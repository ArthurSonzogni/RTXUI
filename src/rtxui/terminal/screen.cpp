// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/screen.hpp"

#include <array>
#include <cctype>
#include <charconv>
#include <cmath>
#include <functional>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "rtxui/base/clock.hpp"
#include "rtxui/color.hpp"
#include "rtxui/event.hpp"
#include "rtxui/layout/layout_arena.hpp"
#include "rtxui/layout/layout_box.hpp"
#include "rtxui/layout/style.hpp"
#include "rtxui/paint/cell.hpp"
#include "rtxui/refcounted.hpp"
#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <io.h>
#elif defined(__EMSCRIPTEN__)
#include <emscripten.h>
#else
#include <fcntl.h>
#include <sys/select.h>
#include <unistd.h>
#endif

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <optional>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <utility>

#include "rtxui/base/string.hpp"
#include "rtxui/base/task_runner.hpp"
#include "rtxui/component/component_internal.hpp"
#include "rtxui/diagnostic.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/dom/text_element.hpp"
#include "rtxui/headless.hpp"
#include "rtxui/layout/layout.hpp"
#include "rtxui/layout/layout_tree_builder.hpp"
#include "rtxui/layout/physical_fragment.hpp"
#include "rtxui/layout/sticky.hpp"
#include "rtxui/paint/paint.hpp"
#include "rtxui/paint/texture.hpp"
#include "rtxui/style/style.hpp"
#include "rtxui/terminal/input_wait.hpp"
#include "rtxui/terminal/terminal_device.hpp"
#include "rtxui/terminal/terminal_input_parser.hpp"

namespace rtxui {

namespace {

void handle_sigwinch(int /*sig*/) {}

// absolute_x()/absolute_y() reflect an element's scrolled screen position,
// but not whether a scrollable ancestor currently clips it out of view
// (e.g. a textarea scrolled away from its cursor, without moving the
// cursor itself). Used to avoid positioning the native terminal cursor
// outside the box that's actually drawn on screen.
bool IsWithinScrollClip(Element* el) {
  if (!el) {
    return false;
  }
  int x = el->absolute_x();
  int y = el->absolute_y();
  for (Element* ancestor = el->Parent(); ancestor;
       ancestor = ancestor->Parent()) {
    bool clips_x = ancestor->style.overflow_x != Overflow::Visible;
    bool clips_y = ancestor->style.overflow_y != Overflow::Visible;
    if (!clips_x && !clips_y) {
      continue;
    }
    int border = (ancestor->style.border_style != BorderStyle::None) ? 1 : 0;
    int content_x0 =
        ancestor->absolute_x() + border + ancestor->style.padding.left;
    int content_y0 =
        ancestor->absolute_y() + border + ancestor->style.padding.top;
    int content_x1 = ancestor->absolute_x() + ancestor->layout_width() -
                     border - ancestor->style.padding.right;
    int content_y1 = ancestor->absolute_y() + ancestor->layout_height() -
                     border - ancestor->style.padding.bottom;
    if (clips_x && (x < content_x0 || x >= content_x1)) {
      return false;
    }
    if (clips_y && (y < content_y0 || y >= content_y1)) {
      return false;
    }
  }
  return true;
}

std::optional<int> GetEffectiveTabIndex(Element* el) {
  if (!el) {
    return std::nullopt;
  }
  if (el->disabled()) {
    return std::nullopt;
  }
  const auto& attrs = el->Attributes();
  if (attrs.count("tabindex")) {
    std::string_view s = attrs.at("tabindex");
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
      s.remove_prefix(1);
    }
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
      s.remove_suffix(1);
    }
    if (!s.empty()) {
      int val = 0;
      auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
      if (ec == std::errc() && ptr == s.data() + s.size()) {
        return val;
      }
    }
  }
  if (attrs.count("focusable")) {
    std::string val = attrs.at("focusable");
    if (val == "true" || val == "1") {
      return 0;
    }
  }
  std::string_view tag = el->tag();
  if (tag == "input" || tag == "textarea" || tag == "checkbox" ||
      tag == "radio" || tag == "slider" || tag == "button" || tag == "select") {
    return 0;
  }
  // A link, as in a browser: one without an href is just text.
  if (tag == "a" && attrs.count("href")) {
    return 0;
  }
  return std::nullopt;
}

struct FocusableFragment {
  Element* element;
  int x, y, width, height;
};

void CollectFocusableFragments(
    PhysicalFragment* fragment,
    int abs_x,
    int abs_y,
    int viewport_x,
    int viewport_y,
    int viewport_w,
    int viewport_h,
    std::vector<FocusableFragment>& focusable_fragments) {
  if (!fragment) {
    return;
  }

  if (fragment->dom_node) {
    auto tab_index_opt = GetEffectiveTabIndex(fragment->dom_node);
    if (tab_index_opt.has_value() && tab_index_opt.value() >= 0) {
      focusable_fragments.push_back({fragment->dom_node, abs_x, abs_y,
                                     fragment->width, fragment->height});
    }
  }

  int scroll_x_offset = 0;
  int scroll_y_offset = 0;
  int next_viewport_x = viewport_x;
  int next_viewport_y = viewport_y;
  int next_viewport_w = viewport_w;
  int next_viewport_h = viewport_h;
  int border_l = 0, border_t = 0, border_r = 0, border_b = 0;
  int padding_l = 0, padding_t = 0, padding_r = 0, padding_b = 0;
  if (fragment->dom_node) {
    padding_l = fragment->dom_node->style.padding.left;
    padding_t = fragment->dom_node->style.padding.top;
    padding_r = fragment->dom_node->style.padding.right;
    padding_b = fragment->dom_node->style.padding.bottom;
  }
  if (fragment->has_border && fragment->border_style != BorderStyle::None) {
    border_l = 1;
    border_t = 1;
    border_r = 1;
    border_b = 1;
  }

  if (fragment->clips_descendants) {
    scroll_x_offset = fragment->scroll_x;
    scroll_y_offset = fragment->scroll_y;
    next_viewport_x = abs_x + border_l + padding_l;
    next_viewport_y = abs_y + border_t + padding_t;
    next_viewport_w =
        fragment->width - border_l - border_r - padding_l - padding_r;
    next_viewport_h =
        fragment->height - border_t - border_b - padding_t - padding_b;
  }

  for (const auto& child : fragment->children) {
    bool is_fixed =
        (child.fragment && child.fragment->dom_node &&
         child.fragment->dom_node->style.position == PositionType::Fixed);
    bool is_sticky =
        (child.fragment && child.fragment->dom_node &&
         child.fragment->dom_node->style.position == PositionType::Sticky);

    int child_abs_x = is_fixed ? child.x : abs_x + child.x - scroll_x_offset;
    int child_abs_y = is_fixed ? child.y : abs_y + child.y - scroll_y_offset;

    if (is_sticky) {
      int c_border_l = 0, c_border_r = 0, c_border_t = 0, c_border_b = 0;
      int c_padding_l = 0, c_padding_r = 0, c_padding_t = 0, c_padding_b = 0;
      if (fragment->dom_node) {
        c_padding_l = fragment->dom_node->style.padding.left;
        c_padding_r = fragment->dom_node->style.padding.right;
        c_padding_t = fragment->dom_node->style.padding.top;
        c_padding_b = fragment->dom_node->style.padding.bottom;
      }
      if (fragment->has_border && fragment->border_style != BorderStyle::None) {
        c_border_l = 1;
        c_border_r = 1;
        c_border_t = 1;
        c_border_b = 1;
      }

      StickyContext sticky;
      sticky.parent_clips = fragment->clips_descendants;
      sticky.viewport_x =
          fragment->clips_descendants ? next_viewport_x : viewport_x;
      sticky.viewport_y =
          fragment->clips_descendants ? next_viewport_y : viewport_y;
      sticky.viewport_width =
          fragment->clips_descendants ? next_viewport_w : viewport_w;
      sticky.viewport_height =
          fragment->clips_descendants ? next_viewport_h : viewport_h;
      sticky.parent_x = abs_x;
      sticky.parent_y = abs_y;
      sticky.parent_width = fragment->width;
      sticky.parent_height = fragment->height;
      sticky.parent_border_left = c_border_l;
      sticky.parent_border_top = c_border_t;
      sticky.parent_border_right = c_border_r;
      sticky.parent_border_bottom = c_border_b;
      sticky.parent_padding_left = c_padding_l;
      sticky.parent_padding_top = c_padding_t;
      sticky.parent_padding_right = c_padding_r;
      sticky.parent_padding_bottom = c_padding_b;
      ApplyStickyOffset(child.fragment->dom_node->style, child.fragment->width,
                        child.fragment->height, sticky, child_abs_x,
                        child_abs_y);
    }

    CollectFocusableFragments(child.fragment, child_abs_x, child_abs_y,
                              next_viewport_x, next_viewport_y, next_viewport_w,
                              next_viewport_h, focusable_fragments);
  }
}

// Whether the mouse passes through `element`: pointer-events is inherited,
// so the nearest element that sets it decides.
bool IgnoresPointer(const Element* element) {
  for (; element; element = element->Parent()) {
    if (element->style.pointer_events_none) {
      return *element->style.pointer_events_none;
    }
  }
  return false;
}

Element* FindElementAtImpl(PhysicalFragment* fragment,
                           int target_x,
                           int target_y,
                           int abs_x = 0,
                           int abs_y = 0,
                           int accum_scroll_x = 0,
                           int accum_scroll_y = 0,
                           int viewport_x = 0,
                           int viewport_y = 0,
                           int viewport_w = 0,
                           int viewport_h = 0) {
  if (!fragment) {
    return nullptr;
  }
  if (target_x < abs_x || target_y < abs_y ||
      target_x >= abs_x + fragment->width ||
      target_y >= abs_y + fragment->height) {
    return nullptr;
  }

  int scroll_x_offset = 0;
  int scroll_y_offset = 0;
  int next_viewport_x = viewport_x;
  int next_viewport_y = viewport_y;
  int next_viewport_w = viewport_w;
  int next_viewport_h = viewport_h;
  int border_l = 0, border_t = 0, border_r = 0, border_b = 0;
  int padding_l = 0, padding_t = 0, padding_r = 0, padding_b = 0;
  if (fragment->dom_node) {
    padding_l = fragment->dom_node->style.padding.left;
    padding_t = fragment->dom_node->style.padding.top;
    padding_r = fragment->dom_node->style.padding.right;
    padding_b = fragment->dom_node->style.padding.bottom;
  }
  if (fragment->has_border && fragment->border_style != BorderStyle::None) {
    border_l = 1;
    border_t = 1;
    border_r = 1;
    border_b = 1;
  }

  if (fragment->clips_descendants) {
    scroll_x_offset = fragment->scroll_x;
    scroll_y_offset = fragment->scroll_y;
    next_viewport_x = abs_x + border_l + padding_l;
    next_viewport_y = abs_y + border_t + padding_t;
    next_viewport_w =
        fragment->width - border_l - border_r - padding_l - padding_r;
    next_viewport_h =
        fragment->height - border_t - border_b - padding_t - padding_b;
  }

  int next_accum_scroll_x = accum_scroll_x + scroll_x_offset;
  int next_accum_scroll_y = accum_scroll_y + scroll_y_offset;

  auto sorted_children = fragment->children;
  std::stable_sort(
      sorted_children.begin(), sorted_children.end(),
      [](const PhysicalFragment::ChildLink& a,
         const PhysicalFragment::ChildLink& b) {
        bool a_pos = (a.fragment && a.fragment->dom_node &&
                      a.fragment->dom_node->style.PaintsAsPositioned());
        bool b_pos = (b.fragment && b.fragment->dom_node &&
                      b.fragment->dom_node->style.PaintsAsPositioned());
        int az = (a.fragment && a.fragment->dom_node)
                     ? a.fragment->dom_node->style.z_index.value_or(0)
                     : 0;
        int bz = (b.fragment && b.fragment->dom_node)
                     ? b.fragment->dom_node->style.z_index.value_or(0)
                     : 0;
        if (az != bz) {
          return az < bz;
        }
        if (a_pos != b_pos) {
          return !a_pos && b_pos;
        }
        return false;
      });

  for (auto& child : std::views::reverse(sorted_children)) {
    bool is_fixed =
        (child.fragment && child.fragment->dom_node &&
         child.fragment->dom_node->style.position == PositionType::Fixed);
    bool is_sticky =
        (child.fragment && child.fragment->dom_node &&
         child.fragment->dom_node->style.position == PositionType::Sticky);

    int child_abs_x = is_fixed ? child.x : abs_x + child.x - scroll_x_offset;
    int child_abs_y = is_fixed ? child.y : abs_y + child.y - scroll_y_offset;
    int child_accum_scroll_x = next_accum_scroll_x;
    int child_accum_scroll_y = next_accum_scroll_y;

    if (is_fixed) {
      child_accum_scroll_x = 0;
      child_accum_scroll_y = 0;
    } else {
      if (is_sticky) {
        int c_border_l = 0, c_border_r = 0, c_border_t = 0, c_border_b = 0;
        int c_padding_l = 0, c_padding_r = 0, c_padding_t = 0, c_padding_b = 0;
        if (fragment->dom_node) {
          c_padding_l = fragment->dom_node->style.padding.left;
          c_padding_r = fragment->dom_node->style.padding.right;
          c_padding_t = fragment->dom_node->style.padding.top;
          c_padding_b = fragment->dom_node->style.padding.bottom;
        }
        if (fragment->has_border &&
            fragment->border_style != BorderStyle::None) {
          c_border_l = 1;
          c_border_r = 1;
          c_border_t = 1;
          c_border_b = 1;
        }

        StickyContext sticky;
        sticky.parent_clips = fragment->clips_descendants;
        sticky.viewport_x =
            fragment->clips_descendants ? next_viewport_x : viewport_x;
        sticky.viewport_y =
            fragment->clips_descendants ? next_viewport_y : viewport_y;
        sticky.viewport_width =
            fragment->clips_descendants ? next_viewport_w : viewport_w;
        sticky.viewport_height =
            fragment->clips_descendants ? next_viewport_h : viewport_h;
        sticky.parent_x = abs_x;
        sticky.parent_y = abs_y;
        sticky.parent_width = fragment->width;
        sticky.parent_height = fragment->height;
        sticky.parent_border_left = c_border_l;
        sticky.parent_border_top = c_border_t;
        sticky.parent_border_right = c_border_r;
        sticky.parent_border_bottom = c_border_b;
        sticky.parent_padding_left = c_padding_l;
        sticky.parent_padding_top = c_padding_t;
        sticky.parent_padding_right = c_padding_r;
        sticky.parent_padding_bottom = c_padding_b;
        ApplyStickyOffset(child.fragment->dom_node->style,
                          child.fragment->width, child.fragment->height, sticky,
                          child_abs_x, child_abs_y);
      }
    }

    if (auto* found = FindElementAtImpl(
            child.fragment, target_x, target_y, child_abs_x, child_abs_y,
            child_accum_scroll_x, child_accum_scroll_y, next_viewport_x,
            next_viewport_y, next_viewport_w, next_viewport_h)) {
      return found;
    }
  }

  // An element the mouse passes through is no target, but its children may
  // be: what is under it is searched next.
  if (fragment->dom_node && !IgnoresPointer(fragment->dom_node)) {
    return fragment->dom_node;
  }
  return nullptr;
}

// A `position: fixed` fragment is placed against the viewport, so it paints
// wherever it likes regardless of where its ancestors sit. FindElementAtImpl
// culls a subtree against its ancestor's box *before* descending, which makes
// such a fragment unreachable whenever an ancestor's box doesn't cover the
// click -- an overlay whose parent is a zero-height wrapper, say, which is
// exactly the shape of the built-in <dialog>. Collect them up front so they
// can be tested against the viewport directly.
struct FixedFragment {
  PhysicalFragment* fragment;
  // Viewport coordinates. A fragment does not know its own position -- that
  // lives in the parent's ChildLink -- and PhysicalFragment::x/y are never
  // assigned by layout, so they must not be read here.
  int x = 0;
  int y = 0;
};

// Walks the fragment tree accumulating each fragment's absolute origin, so a
// fixed fragment's parent-relative ChildLink offset can be turned into the
// viewport position it actually paints at. This mirrors PaintFragment: a
// fixed child sits at `parent_origin + link + accumulated_scroll`. Taking the
// raw link offset instead put the whole subtree at the wrong place whenever
// the fixed element's parent was not itself at the viewport origin -- e.g.
// the built-in <dialog> inside a padded wrapper, whose contents then hit-test
// several cells away from where they are drawn.
void CollectFixedFragments(PhysicalFragment* fragment,
                           std::vector<FixedFragment>& out,
                           int abs_x = 0,
                           int abs_y = 0,
                           int accum_scroll_x = 0,
                           int accum_scroll_y = 0) {
  if (!fragment) {
    return;
  }
  int scroll_x_offset = fragment->clips_descendants ? fragment->scroll_x : 0;
  int scroll_y_offset = fragment->clips_descendants ? fragment->scroll_y : 0;
  int next_accum_scroll_x = accum_scroll_x + scroll_x_offset;
  int next_accum_scroll_y = accum_scroll_y + scroll_y_offset;

  for (const auto& child : fragment->children) {
    bool is_fixed =
        child.fragment && child.fragment->dom_node &&
        child.fragment->dom_node->style.position == PositionType::Fixed;
    int child_abs_x = abs_x + child.x;
    int child_abs_y = abs_y + child.y;
    int child_accum_scroll_x = next_accum_scroll_x;
    int child_accum_scroll_y = next_accum_scroll_y;
    if (is_fixed) {
      child_abs_x += accum_scroll_x;
      child_abs_y += accum_scroll_y;
      child_accum_scroll_x = 0;
      child_accum_scroll_y = 0;
      out.push_back({child.fragment, child_abs_x, child_abs_y});
    } else {
      child_abs_x -= scroll_x_offset;
      child_abs_y -= scroll_y_offset;
    }
    CollectFixedFragments(child.fragment, out, child_abs_x, child_abs_y,
                          child_accum_scroll_x, child_accum_scroll_y);
  }
}

Element* FindElementAt(PhysicalFragment* fragment, int target_x, int target_y) {
  std::vector<FixedFragment> fixed;
  CollectFixedFragments(fragment, fixed);

  // Topmost first, matching paint order. `display: none` subtrees never reach
  // layout, so a closed overlay contributes no fragment here and cannot
  // swallow clicks meant for the interface underneath it.
  std::stable_sort(fixed.begin(), fixed.end(),
                   [](const FixedFragment& a, const FixedFragment& b) {
                     return a.fragment->dom_node->style.z_index.value_or(0) >
                            b.fragment->dom_node->style.z_index.value_or(0);
                   });

  for (const FixedFragment& candidate : fixed) {
    if (auto* found = FindElementAtImpl(candidate.fragment, target_x, target_y,
                                        candidate.x, candidate.y, 0, 0, 0, 0,
                                        fragment ? fragment->width : 0,
                                        fragment ? fragment->height : 0)) {
      return found;
    }
  }

  return FindElementAtImpl(fragment, target_x, target_y, 0, 0, 0, 0, 0, 0,
                           fragment ? fragment->width : 0,
                           fragment ? fragment->height : 0);
}

PhysicalFragment* FindFragmentForElement(PhysicalFragment* fragment,
                                         Element* element) {
  if (!fragment) {
    return nullptr;
  }
  if (fragment->dom_node == element) {
    return fragment;
  }
  for (const auto& child : fragment->children) {
    if (auto found = FindFragmentForElement(child.fragment, element)) {
      return found;
    }
  }
  return nullptr;
}

PhysicalFragment* FindFirstScrollableFragment(PhysicalFragment* fragment) {
  if (!fragment) {
    return nullptr;
  }
  if (fragment->dom_node &&
      (fragment->dom_node->style.overflow_y == Overflow::Scroll ||
       fragment->dom_node->style.overflow_x == Overflow::Scroll)) {
    return fragment;
  }
  for (const auto& child : fragment->children) {
    if (auto found = FindFirstScrollableFragment(child.fragment)) {
      return found;
    }
  }
  return nullptr;
}

struct FragmentWithPos {
  PhysicalFragment* fragment;
  int abs_x = 0;
  int abs_y = 0;
};

std::optional<FragmentWithPos> FindFragmentForElementWithPos(
    PhysicalFragment* fragment,
    Element* element,
    int abs_x = 0,
    int abs_y = 0,
    int accum_scroll_x = 0,
    int accum_scroll_y = 0,
    int viewport_x = 0,
    int viewport_y = 0,
    int viewport_w = 0,
    int viewport_h = 0) {
  if (!fragment) {
    return std::nullopt;
  }
  if (fragment->dom_node == element) {
    return FragmentWithPos{fragment, abs_x, abs_y};
  }

  int scroll_x_offset = 0;
  int scroll_y_offset = 0;
  int next_viewport_x = viewport_x;
  int next_viewport_y = viewport_y;
  int next_viewport_w = viewport_w;
  int next_viewport_h = viewport_h;
  int border_l = 0, border_t = 0, border_r = 0, border_b = 0;
  int padding_l = 0, padding_t = 0, padding_r = 0, padding_b = 0;
  if (fragment->dom_node) {
    padding_l = fragment->dom_node->style.padding.left;
    padding_t = fragment->dom_node->style.padding.top;
    padding_r = fragment->dom_node->style.padding.right;
    padding_b = fragment->dom_node->style.padding.bottom;
  }
  if (fragment->has_border && fragment->border_style != BorderStyle::None) {
    border_l = 1;
    border_t = 1;
    border_r = 1;
    border_b = 1;
  }

  if (fragment->clips_descendants) {
    scroll_x_offset = fragment->scroll_x;
    scroll_y_offset = fragment->scroll_y;
    next_viewport_x = abs_x + border_l + padding_l;
    next_viewport_y = abs_y + border_t + padding_t;
    next_viewport_w =
        fragment->width - border_l - border_r - padding_l - padding_r;
    next_viewport_h =
        fragment->height - border_t - border_b - padding_t - padding_b;
  }

  int next_accum_scroll_x = accum_scroll_x + scroll_x_offset;
  int next_accum_scroll_y = accum_scroll_y + scroll_y_offset;

  for (const auto& child : fragment->children) {
    bool is_fixed =
        (child.fragment && child.fragment->dom_node &&
         child.fragment->dom_node->style.position == PositionType::Fixed);
    bool is_sticky =
        (child.fragment && child.fragment->dom_node &&
         child.fragment->dom_node->style.position == PositionType::Sticky);

    int child_abs_x = is_fixed ? child.x : abs_x + child.x - scroll_x_offset;
    int child_abs_y = is_fixed ? child.y : abs_y + child.y - scroll_y_offset;
    int child_accum_scroll_x = next_accum_scroll_x;
    int child_accum_scroll_y = next_accum_scroll_y;

    if (is_fixed) {
      child_accum_scroll_x = 0;
      child_accum_scroll_y = 0;
    } else {
      if (is_sticky) {
        int c_border_l = 0, c_border_r = 0, c_border_t = 0, c_border_b = 0;
        int c_padding_l = 0, c_padding_r = 0, c_padding_t = 0, c_padding_b = 0;
        if (fragment->dom_node) {
          c_padding_l = fragment->dom_node->style.padding.left;
          c_padding_r = fragment->dom_node->style.padding.right;
          c_padding_t = fragment->dom_node->style.padding.top;
          c_padding_b = fragment->dom_node->style.padding.bottom;
        }
        if (fragment->has_border &&
            fragment->border_style != BorderStyle::None) {
          c_border_l = 1;
          c_border_r = 1;
          c_border_t = 1;
          c_border_b = 1;
        }

        StickyContext sticky;
        sticky.parent_clips = fragment->clips_descendants;
        sticky.viewport_x =
            fragment->clips_descendants ? next_viewport_x : viewport_x;
        sticky.viewport_y =
            fragment->clips_descendants ? next_viewport_y : viewport_y;
        sticky.viewport_width =
            fragment->clips_descendants ? next_viewport_w : viewport_w;
        sticky.viewport_height =
            fragment->clips_descendants ? next_viewport_h : viewport_h;
        sticky.parent_x = abs_x;
        sticky.parent_y = abs_y;
        sticky.parent_width = fragment->width;
        sticky.parent_height = fragment->height;
        sticky.parent_border_left = c_border_l;
        sticky.parent_border_top = c_border_t;
        sticky.parent_border_right = c_border_r;
        sticky.parent_border_bottom = c_border_b;
        sticky.parent_padding_left = c_padding_l;
        sticky.parent_padding_top = c_padding_t;
        sticky.parent_padding_right = c_padding_r;
        sticky.parent_padding_bottom = c_padding_b;
        ApplyStickyOffset(child.fragment->dom_node->style,
                          child.fragment->width, child.fragment->height, sticky,
                          child_abs_x, child_abs_y);
      }
    }

    if (auto found = FindFragmentForElementWithPos(
            child.fragment, element, child_abs_x, child_abs_y,
            child_accum_scroll_x, child_accum_scroll_y, next_viewport_x,
            next_viewport_y, next_viewport_w, next_viewport_h)) {
      return found;
    }
  }

  return std::nullopt;
}

bool IsCellOnVerticalScrollbar(const PhysicalFragment* frag,
                               int abs_x,
                               int abs_y,
                               int mx,
                               int my,
                               bool* out_on_thumb) {
  if (!frag || !frag->dom_node) {
    return false;
  }
  if (frag->dom_node->style.overflow_y != Overflow::Scroll ||
      frag->dom_node->style.scrollbar_width != ScrollbarWidth::Auto) {
    return false;
  }

  int w = frag->width;
  int h = frag->height;
  bool draw_h_scrollbar =
      (frag->dom_node->style.overflow_x == Overflow::Scroll &&
       frag->dom_node->style.scrollbar_width == ScrollbarWidth::Auto);

  int border_right =
      (frag->has_border && frag->border_style != BorderStyle::None) ? 1 : 0;
  int scrollbar_x = abs_x + w - border_right - 1;

  if (mx != scrollbar_x) {
    return false;
  }

  int track_y_start = abs_y;
  int track_h = h;
  if (frag->has_border && frag->border_style != BorderStyle::None) {
    track_y_start = abs_y + 1;
    track_h = draw_h_scrollbar ? (h - 3) : (h - 2);
  } else {
    track_h = draw_h_scrollbar ? (h - 1) : h;
  }

  if (my < track_y_start || my >= track_y_start + track_h) {
    return false;
  }

  // Determine if it's on the thumb
  int scroll_height = frag->dom_node->scroll_height();
  int padding_vert = frag->dom_node->style.padding.Vert();
  int border_vert =
      (frag->has_border && frag->border_style != BorderStyle::None) ? 1 : 0;
  int viewport_h = std::max(1, h - 2 * border_vert - padding_vert);

  int thumb_h_eighths = (viewport_h * track_h * 8) / std::max(1, scroll_height);
  thumb_h_eighths = std::max(8, thumb_h_eighths);
  thumb_h_eighths = std::min(track_h * 8, thumb_h_eighths);

  int max_scroll = scroll_height - h;
  int thumb_y_eighths =
      (max_scroll > 0) ? static_cast<int>(std::round(
                             frag->visual_scroll_y *
                             ((track_h * 8) - thumb_h_eighths) / max_scroll))
                       : 0;

  int i = my - track_y_start;
  int cell_start = i * 8;
  int cell_end = (i + 1) * 8;

  int start_eighth = std::max(cell_start, thumb_y_eighths);
  int end_eighth = std::min(cell_end, thumb_y_eighths + thumb_h_eighths);

  if (out_on_thumb) {
    *out_on_thumb = (start_eighth < end_eighth);
  }
  return true;
}

bool IsCellOnHorizontalScrollbar(const PhysicalFragment* frag,
                                 int abs_x,
                                 int abs_y,
                                 int mx,
                                 int my,
                                 bool* out_on_thumb) {
  if (!frag || !frag->dom_node) {
    return false;
  }
  if (frag->dom_node->style.overflow_x != Overflow::Scroll ||
      frag->dom_node->style.scrollbar_width != ScrollbarWidth::Auto) {
    return false;
  }

  int w = frag->width;
  int h = frag->height;
  bool draw_v_scrollbar =
      (frag->dom_node->style.overflow_y == Overflow::Scroll &&
       frag->dom_node->style.scrollbar_width == ScrollbarWidth::Auto);

  int border_bottom =
      (frag->has_border && frag->border_style != BorderStyle::None) ? 1 : 0;
  int scrollbar_y = abs_y + h - border_bottom - 1;

  if (my != scrollbar_y) {
    return false;
  }

  int track_x_start = abs_x;
  int track_w = w;
  if (frag->has_border && frag->border_style != BorderStyle::None) {
    track_x_start = abs_x + 1;
    track_w = draw_v_scrollbar ? (w - 3) : (w - 2);
  } else {
    track_w = draw_v_scrollbar ? (w - 1) : w;
  }

  if (mx < track_x_start || mx >= track_x_start + track_w) {
    return false;
  }

  // Determine if it's on the thumb
  int scroll_width = frag->dom_node->scroll_width();
  int padding_horiz = frag->dom_node->style.padding.Horiz();
  int border_horiz =
      (frag->has_border && frag->border_style != BorderStyle::None) ? 2 : 0;
  int viewport_w = std::max(1, w - border_horiz - padding_horiz);

  int thumb_w_eighths = (viewport_w * track_w * 8) / std::max(1, scroll_width);
  thumb_w_eighths = std::max(8, thumb_w_eighths);
  thumb_w_eighths = std::min(track_w * 8, thumb_w_eighths);

  int max_scroll = scroll_width - w;
  int thumb_x_eighths =
      (max_scroll > 0) ? static_cast<int>(std::round(
                             frag->visual_scroll_x *
                             ((track_w * 8) - thumb_w_eighths) / max_scroll))
                       : 0;

  int i = mx - track_x_start;
  int cell_start = i * 8;
  int cell_end = (i + 1) * 8;

  int start_eighth = std::max(cell_start, thumb_x_eighths);
  int end_eighth = std::min(cell_end, thumb_x_eighths + thumb_w_eighths);

  if (out_on_thumb) {
    *out_on_thumb = (start_eighth < end_eighth);
  }
  return true;
}

}  // namespace

// A terminal that exists only in memory, for HeadlessScreen and for
// RTXUI_HEADLESS. Output is discarded: the frame is read back from the last
// texture instead, which is what the output would have drawn.
class HeadlessTerminalDevice : public TerminalDevice {
 public:
  HeadlessTerminalDevice(int width, int height)
      : width_(width), height_(height) {}

  bool IsAtty() override { return true; }
  int Read(char* buf, int len) override {
    int count = std::min(len, static_cast<int>(input_.size()));
    std::copy_n(input_.begin(), count, buf);
    input_.erase(0, count);
    return count;
  }
  void Write(std::string_view /*data*/) override {}
  bool GetSize(int& width, int& height) override {
    width = width_;
    height = height_;
    return true;
  }
  void EnterRawMode(void (* /*sigwinch_handler*/)(int),
                    bool /*alternate_screen*/) override {}
  void ExitRawMode() override {}

  void PushInput(std::string_view data) { input_ += data; }
  bool HasInput() const { return !input_.empty(); }
  void Resize(int width, int height) {
    width_ = width;
    height_ = height;
  }

 private:
  std::string input_;
  int width_;
  int height_;
};

namespace {

// Parses RTXUI_HEADLESS=<width>x<height>.
std::optional<std::pair<int, int>> HeadlessSizeFromEnvironment() {
  const char* value = std::getenv("RTXUI_HEADLESS");
  if (!value) {
    return std::nullopt;
  }
  std::string_view text = value;
  size_t x = text.find('x');
  if (x == std::string_view::npos) {
    return std::nullopt;
  }
  int width = 0;
  int height = 0;
  auto [w_end, w_err] = std::from_chars(text.data(), text.data() + x, width);
  auto [h_end, h_err] =
      std::from_chars(text.data() + x + 1, text.data() + text.size(), height);
  if (w_err != std::errc() || h_err != std::errc() || width <= 0 ||
      height <= 0 || width > 255 || height > 255) {
    return std::nullopt;
  }
  return std::pair{width, height};
}

}  // namespace

class ScreenImpl {
 public:
  ScreenImpl(Ref<ComponentBase> component,
             std::shared_ptr<TerminalDevice> device);
  ~ScreenImpl();

  void Loop();
  void Step();
  // When the next delayed task is due, from the end of the last Step().
  std::chrono::steady_clock::duration next_task_delay_{};
  // A lone Escape is the Escape key once this long has passed without the
  // rest of a sequence following it.
  static constexpr double kEscapeDelayMs = 50;
  double last_input_ms_ = 0;
  // Handles the input the parser was waiting to tell apart, as complete.
  void SettlePendingInput();
  void Exit() { running_ = false; }
  void Dispatch(Event event);
  // `reuse_layout`: when nothing but paint-only animations changed since the
  // last layout (layout_reusable_), keep it and refresh its colors instead.
  void Draw(bool reuse_layout = false);
  void RequestDraw();
  // `reuse_fragments`: lay out again only what changed since the last layout,
  // reusing the rest of its fragments.
  void UpdateLayout(bool reuse_fragments = true);

  void UpdateSize();
  void DigestAndDraw();
  void HandleEvent(Event event);
  // Whether something is moving and needs more frames. `include_infinite`
  // false leaves out the animations that never end.
  bool HasActiveTransitions(bool include_infinite = true);
  bool TickTransitions(double current_time_ms);
  // Runs the `onanimationend` handler of every element whose animation
  // finished since the last call. Returns whether one ran.
  bool DispatchAnimationEnds();
  void ScrollIntoView(Element* element);

  std::string Text() const;
  // The cell at column `x`, row `y` of the last frame drawn, or nothing.
  const Cell* CellAt(int x, int y) const;
  // Processes everything queued on a HeadlessTerminalDevice, then lets the
  // transitions it started finish, so the frame left behind does not depend
  // on timing.
  void Settle(HeadlessTerminalDevice& device);
  void RunHeadless();
  bool headless_ = false;

  bool SpatialNavigate(Event event);
  void SimulateClick(Element* element);
  // Follows `link`, an <a href="#id">: scrolls the element with that id into
  // view. Returns whether there was one.
  bool FollowLink(Element* link);
  void PaintScreen(const PhysicalFragment& root,
                   Texture& texture,
                   const PaintSelection* selection = nullptr) const;

  // Text selection, kept in the document as a browser keeps it: a left drag
  // selects the text from the glyph it started on to the one it is on, in
  // document order, wherever that text is drawn. It is copied on release,
  // and lasts until the next press.
  struct Selection {
    // The left button went down where a selection can start.
    bool armed = false;
    // The mouse has left the cell it went down on since.
    bool dragged = false;
    int press_x = 0, press_y = 0;
    int mouse_x = 0, mouse_y = 0;
    // What a press selects at once: a glyph, a word (a double click) or a
    // block (a triple click). A drag then extends it by as much.
    enum class Unit { Glyph, Word, Block };
    Unit unit = Unit::Glyph;
    // The first and last glyphs of the units at either end. The selection
    // runs from the first start to the last end. No node where there was
    // no text to start or end on.
    TextPosition anchor;
    TextPosition anchor_end;
    TextPosition focus;
    TextPosition focus_end;
    // The `user-select: all` elements at either end, selected whole.
    const Element* anchor_all = nullptr;
    const Element* focus_all = nullptr;
    // The scroll containers the drag started in, innermost first. While the
    // mouse is past the edge of one, it scrolls, and the selection follows.
    std::vector<Ref<ElementHandleTarget>> scrollers;

    bool Shown() const {
      return dragged || unit != Unit::Glyph || anchor_all || focus_all;
    }
  };
  // Whether the selection is drawn: not while a press it started on is
  // taken by a component capturing the mouse, an input selecting its own.
  bool SelectionShown() const {
    return selection_.Shown() &&
           !(selection_.armed && ComponentBase::GetMouseCapturer());
  }
  // Updates the selection for a mouse event. Returns whether that consumed
  // the event.
  bool HandleSelection(const Event::Mouse& mouse);
  // Paints the last layout again, into the texture returned, and records in
  // `glyphs` the glyph of selectable text each cell shows.
  Texture PaintGlyphs(std::vector<TextPosition>& glyphs) const;
  // The glyph a selection ending at cell `x`, `y` ends on: the one there,
  // else the nearest one before it.
  TextPosition GlyphNear(const std::vector<TextPosition>& glyphs,
                         int x,
                         int y) const;
  // The first and last glyphs of the unit around `glyph`, from what
  // PaintGlyphs() recorded: the glyph, the word on screen, or the block.
  std::pair<TextPosition, TextPosition> UnitAround(
      const std::vector<TextPosition>& glyphs,
      const Texture& texture,
      const TextPosition& glyph,
      Selection::Unit unit) const;
  // Consecutive presses on a cell, for double and triple clicks.
  int click_count_ = 0;
  int last_click_x_ = -1;
  int last_click_y_ = -1;
  double last_click_ms_ = 0;
  // Numbers the elements in document order, for Selected(), and drops a
  // selection whose ends are gone from the document.
  void UpdateSelectionOrder();
  bool Selected(const TextPosition& glyph) const;
  // The cells of `scroller` that show its content: left, top, right,
  // bottom, inclusive.
  std::optional<std::array<int, 4>> ScrollerViewport(
      const Element* scroller) const;
  // Moves the selection's end to the glyph at the mouse, or, when the mouse
  // is past the scroll container scrolling toward it, at the nearest cell
  // of that container's content.
  void UpdateSelectionFocus();
  // How many cells, and which way, the mouse is past the edges of
  // `scroller`: the speed it scrolls at. The edge rows and columns of the
  // screen count as past a container reaching them.
  std::pair<int, int> SelectionScrollSpeed(const Element* scroller) const;
  // What the next autoscroll scrolls, on each axis: the innermost of the
  // selection's scroll containers the mouse is past that can still scroll
  // toward it, and by how much. The next one out takes over at its end.
  struct AutoScrollPlan {
    Element* x_scroller = nullptr;
    Element* y_scroller = nullptr;
    int dx = 0;
    int dy = 0;
  };
  AutoScrollPlan PlanAutoScroll() const;
  // Scrolls the selection's scroll containers toward the mouse, then again
  // every kAutoScrollDelay while the drag goes on.
  void ScheduleAutoScroll();
  void AutoScroll();
  bool autoscroll_pending_ = false;
  // The selected text that is drawn, in document order.
  std::string SelectedText() const;
  Selection selection_;
  std::unordered_map<const Element*, int> selection_order_;
  // Whether the last frame drawn shows a selection.
  bool selection_painted_ = false;

  void SetBackgroundColor(Color color) {
    background_color_ = color;
    background_color_set_ = true;
  }
  Color background_color() const { return background_color_; }

  void SetSmoothScrollEnabled(bool enabled) {
    smooth_scroll_enabled_ = enabled;
  }
  bool smooth_scroll_enabled() const { return smooth_scroll_enabled_; }

  Ref<ComponentBase> component_;
  int width_ = 80;
  int height_ = 24;
  // How many rows the frame takes: the terminal's height, or, inline, the
  // application's, up to it.
  int frame_height_ = 24;
  int last_height_ = 0;

  // Inline (Screen::SetInline): the frame is drawn where the cursor is, on
  // the terminal's main screen, its rows below the shell's.
  bool inline_ = false;
  // Writes the frame from the top-left of the last one, after what Print()
  // queued. Leaves the cursor at the frame's top-left.
  void WriteInlineFrame(const Texture& texture, bool repaint);
  // How many rows above the cursor the frame's top row is, after the terminal
  // got `width` wide.
  int InlineRowsAboveCursor(int width) const;
  // Moves the cursor below the frame, for the shell to go on from there.
  void LeaveInline();
  // The terminal row the frame's top row is on, 0-based, as the terminal last
  // reported it. -1 while unknown: the mouse is ignored until then.
  int inline_top_ = -1;
  // Cursor position queries not answered yet. Only the last answer is used:
  // the frame may have moved since the earlier ones.
  int position_queries_ = 0;
  // Where the cursor was left after the frame, from its top-left: an input's
  // caret, or the top-left itself.
  int rest_x_ = 0;
  int rest_y_ = 0;
  // Whether a button went down inside the frame and is not released yet.
  bool inline_pressed_ = false;
  // Whether Loop() is running, in raw mode.
  bool looping_ = false;
  // What Print() queued: written above the frame at the next inline frame,
  // or else once Loop() returns.
  std::string printed_;
  void Print(std::string text);
  void Print(Ref<ComponentBase> component);
  bool has_drawn_ = false;
  bool running_ = true;
  // Set while draining a burst of already-queued events (e.g. a paste,
  // which synthesizes one keyboard event per character all at once, or a
  // fast scroll delivering several wheel ticks) so RequestDraw() only
  // updates the layout and Step() paints once after the last of them --
  // otherwise each event would trigger its own full layout/paint/write
  // cycle, visibly "typing" a paste out one character at a time instead
  // of applying it immediately.
  bool suppress_draw_ = false;
  // A RequestDraw() was deferred by suppress_draw_ and no Draw() has run
  // since.
  bool draw_pending_ = false;
  // Where this screen's layout lives. Declared before the trees allocated in
  // it, so they are destroyed while it still exists.
  LayoutArenas layout_arenas_;
  PhysicalFragment* root_fragment_;
  std::shared_ptr<LayoutBox> root_box_;
  // Whether root_fragment_ still fits the application, but for the colors,
  // opacities and translations animations changed since: set by each layout,
  // cleared by anything else that can change what is drawn -- an event, a task,
  // a resize, or an animation of anything else.
  bool layout_reusable_ = false;
  // The size the last layout was for. Its fragments are reused only at that
  // size: viewport units reach anywhere in the tree.
  int laid_out_width_ = -1;
  int laid_out_height_ = -1;
  // Whether the last layout reused any fragment from the one before it.
  bool reused_fragments_ = false;
  // Whether a layout since the last paint ran anything, rather than reuse the
  // whole of the one before it. Nothing painted can have changed otherwise,
  // but the colors the screen paints over. A burst of input lays out without
  // painting (suppress_draw_), so this spans layouts.
  bool laid_out_since_paint_ = true;
  // The colors last_texture_ was painted over.
  bool painted_background_set_ = false;
  Color painted_background_;
  Color painted_terminal_background_;
  std::unique_ptr<Texture> last_texture_;
  std::shared_ptr<TerminalDevice> device_;
  std::unique_ptr<TerminalInputParser> parser_;
  Element* focused_element_ = nullptr;
  // Set while UpdateLayout() lays out again for virtual lists, so it does so
  // at most once per frame.
  bool relayout_for_virtual_lists_ = false;
  bool smooth_scroll_enabled_ = true;
  // Transparent: the terminal's own background shows through.
  Color background_color_;
  bool background_color_set_ = false;
  // What the terminal said its background is, in reply to the OSC 11 query
  // made on entering raw mode. Transparent until then, and for a terminal
  // that does not answer. Used only while the app has not named one.
  Color terminal_background_;
  bool drag_active_ = false;
  Element* drag_element_ = nullptr;
  bool drag_vertical_ = false;
  int drag_start_mouse_ = 0;
  int drag_start_mouse_pixel_ = 0;
  int drag_start_thumb_pos_ = 0;
  task::TaskRunner task_runner_;
#if defined(_WIN32)
  HANDLE wakeup_event_ = NULL;
#else
  int wakeup_pipe_[2] = {-1, -1};
#endif

  struct RawTerminal {
    ScreenImpl* screen_ = nullptr;
    explicit RawTerminal(ScreenImpl* screen);
    ~RawTerminal();
  };
};

ScreenImpl::ScreenImpl(Ref<ComponentBase> component,
                       std::shared_ptr<TerminalDevice> device)
    : component_(std::move(component)),
      device_(std::move(device)),
      parser_(std::make_unique<TerminalInputParser>()) {
  ScopedLayoutArenas arenas(layout_arenas_);
#if defined(_WIN32)
  wakeup_event_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  task_runner_.SetWakeupCallback([this]() {
    if (wakeup_event_) {
      SetEvent(wakeup_event_);
    }
  });
#elif !defined(__EMSCRIPTEN__)
  if (pipe(wakeup_pipe_) == 0) {
    fcntl(wakeup_pipe_[0], F_SETFL, O_NONBLOCK);
    fcntl(wakeup_pipe_[1], F_SETFL, O_NONBLOCK);
  } else {
    wakeup_pipe_[0] = -1;
    wakeup_pipe_[1] = -1;
  }

  task_runner_.SetWakeupCallback([this]() {
    if (wakeup_pipe_[1] != -1) {
      char val = 'w';
      ssize_t bytes_written = write(wakeup_pipe_[1], &val, 1);
      (void)bytes_written;
    }
  });
#endif

  if (!device_) {
    if (auto size = HeadlessSizeFromEnvironment()) {
      device_ =
          std::make_shared<HeadlessTerminalDevice>(size->first, size->second);
      headless_ = true;
    } else {
      device_ = std::make_shared<SystemTerminalDevice>();
    }
  }
  // Bracketed paste mode makes the terminal wrap pasted content in
  // "\x1b[200~"/"\x1b[201~" markers instead of sending it as if typed, so
  // it can be inserted as literal text (see TerminalInputParser) instead of
  // being misinterpreted as individual keystrokes or escape sequences.
  device_->Write("\x1b[?2004h");
  UpdateSize();
  css::g_terminal_width = width_;
  css::g_terminal_height = height_;
  component_->Mount();
  component_->Digest();
  // A real terminal is drawn on once Loop() has switched it to the alternate
  // screen. A frame written now would land on the normal screen and stay in
  // the scrollback after the application quits. Laying out is enough for
  // events to be hit-tested until then.
  if (dynamic_cast<SystemTerminalDevice*>(device_.get())) {
    UpdateLayout();
  } else {
    Draw();
  }
}

ScreenImpl::~ScreenImpl() {
  ScopedLayoutArenas arenas(layout_arenas_);
  // A worker thread may still post while this is torn down; its wakeup must
  // not reach the pipe or event closed below.
  task_runner_.SetWakeupCallback(nullptr);
  device_->Write("\x1b[?2004l");
#if defined(_WIN32)
  if (wakeup_event_) {
    CloseHandle(wakeup_event_);
    wakeup_event_ = NULL;
  }
#elif !defined(__EMSCRIPTEN__)
  if (wakeup_pipe_[0] != -1) {
    close(wakeup_pipe_[0]);
  }
  if (wakeup_pipe_[1] != -1) {
    close(wakeup_pipe_[1]);
  }
#endif
}

void ScreenImpl::Loop() {
  if (headless_) {
    RunHeadless();
    return;
  }
  {
    RawTerminal raw_terminal(this);
    looping_ = true;
    if (inline_) {
      // Starts on a line of its own. Printing as many spaces as the terminal
      // is wide leaves the cursor at the end of the line it starts at, or on
      // the next one if that was not empty: the carriage return then goes to
      // the start of the first empty line, without one to spare.
      device_->Write("\x1b[?7h" + std::string(width_, ' ') +
                     "\r\x1b[K\x1b[?7l");
    }
    Draw();

    running_ = true;
    while (running_) {
      Step();
    }
    if (inline_) {
      LeaveInline();
    }
    looping_ = false;
  }
  // What Print() wrote while the alternate screen hid the shell.
  if (!printed_.empty()) {
    device_->Write(std::exchange(printed_, ""));
  }
}

void ScreenImpl::Step() {
  ScopedLayoutArenas arenas(layout_arenas_);
#ifdef __EMSCRIPTEN__
  EM_ASM({ window.rtxui_has_active_transitions = $0; }, HasActiveTransitions());
#endif

  bool input_available = true;

#if defined(_WIN32)
  if (dynamic_cast<SystemTerminalDevice*>(device_.get())) {
    DWORD timeout_ms = static_cast<DWORD>(
        InputWait(HasActiveTransitions() || autoscroll_pending_ ||
                      parser_->HasPendingInput(),
                  next_task_delay_)
            .count());
    HANDLE in_handle = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE handles[2] = {in_handle, wakeup_event_};
    DWORD count = (wakeup_event_ != NULL) ? 2 : 1;
    DWORD wait_result =
        WaitForMultipleObjects(count, handles, FALSE, timeout_ms);
    if (wait_result == WAIT_OBJECT_0) {
      input_available = true;
    } else {
      input_available = false;
      UpdateSize();
    }
  }
#elif !defined(__EMSCRIPTEN__)
  if (dynamic_cast<SystemTerminalDevice*>(device_.get())) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    int max_fd = STDIN_FILENO;
    if (wakeup_pipe_[0] != -1) {
      FD_SET(wakeup_pipe_[0], &fds);
      max_fd = std::max(max_fd, wakeup_pipe_[0]);
    }

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = static_cast<suseconds_t>(
        std::chrono::microseconds(InputWait(HasActiveTransitions() ||
                                                autoscroll_pending_ ||
                                                parser_->HasPendingInput(),
                                            next_task_delay_))
            .count());

    int retval = select(max_fd + 1, &fds, nullptr, nullptr, &tv);
    if (retval == 0) {
      input_available = false;
    } else if (retval < 0) {
      if (errno == EINTR) {
        UpdateSize();
      }
      input_available = false;
    } else {
      if (wakeup_pipe_[0] != -1 && FD_ISSET(wakeup_pipe_[0], &fds)) {
        char dummy[128];
        while (read(wakeup_pipe_[0], dummy, sizeof(dummy)) > 0) {
        }
      }
      input_available = FD_ISSET(STDIN_FILENO, &fds);
    }
  }
#endif

  if (input_available) {
    // Read everything already buffered, not one byte per wakeup. With
    // any-event mouse tracking on, moving the mouse produces a dozen bytes
    // per motion event; draining them a byte at a time meant each motion was
    // decoded and repainted on its own, so a drag across a complex interface
    // built up a backlog and hover feedback lagged far behind the cursor.
    char buffer[4096];
    int bytes_read = device_->Read(buffer, sizeof(buffer));
    if (bytes_read > 0) {
      UpdateSize();
      last_input_ms_ = time::GetTimeMs();
      for (int i = 0; i < bytes_read; ++i) {
        parser_->Add(buffer[i]);
      }
      // Several events commonly complete together (a burst of mouse motion
      // or wheel ticks, or the last byte of a paste's end marker
      // synthesizing one keyboard event per pasted character): suppress
      // the per-event draws so the whole batch applies as a single redraw.
      suppress_draw_ = true;
      while (auto event = parser_->GetEvent()) {
        HandleEvent(*event);
      }
      for (auto [x, y] : parser_->TakeCursorPositions()) {
        if (position_queries_ > 0 && --position_queries_ == 0) {
          inline_top_ = y;
        }
      }
      if (auto background = parser_->TakeBackgroundColor()) {
        terminal_background_ = *background;
        if (!background_color_set_) {
          RequestDraw();
        }
      }
      suppress_draw_ = false;
      if (draw_pending_) {
        Draw();
      }
    } else if (bytes_read == 0) {
      running_ = false;
    } else {
      if (bytes_read == -1 && errno == EINTR) {
        UpdateSize();
      }
    }
  }

  if (parser_->HasPendingInput() &&
      time::GetTimeMs() - last_input_ms_ >= kEscapeDelayMs) {
    SettlePendingInput();
  }

  bool executed_task = false;
  next_task_delay_ = task_runner_.RunUntilNextDelayedTask(&executed_task);
  if (executed_task) {
    layout_reusable_ = false;
    // Tasks posted from other threads (or delayed tasks) commonly mutate
    // component state; digest so those changes reach the terminal without
    // waiting for the next input event.
    DigestAndDraw();
  }

  if (TickTransitions(time::GetTimeMs())) {
    Draw(/*reuse_layout=*/true);
  }

  if (HotReloadManager::PollChanges() || !has_drawn_) {
    Draw();
  }
}

bool ScreenImpl::HasActiveTransitions(bool include_infinite) {
  if (!component_ || !component_->Root()) {
    return false;
  }
  std::function<bool(Element*)> CheckActive = [&](Element* element) {
    if (!element) {
      return false;
    }
    if (element->IsAnimating(include_infinite)) {
      return true;
    }
    for (size_t i = 0; i < element->ChildCount(); ++i) {
      if (CheckActive(element->ChildAt(i))) {
        return true;
      }
    }
    return false;
  };
  return CheckActive(component_->Root());
}

bool ScreenImpl::TickTransitions(double current_time_ms) {
  if (!component_ || !component_->Root()) {
    return false;
  }
  // As in CSS, nothing under a `display: none` element animates: it would
  // play out unseen, and keep the screen redrawing for nothing. Its
  // animations start over once it is shown.
  std::function<bool(Element*, bool)> TickAll = [&](Element* element,
                                                    bool hidden) {
    if (!element) {
      return false;
    }
    bool updated = false;
    hidden = hidden || element->style.display_none;
    if (hidden && !element->running_animations.empty()) {
      element->running_animations.clear();
      element->animations_suspended = true;
    } else if (!hidden && element->animations_suspended) {
      element->animations_suspended = false;
      element->UpdateAnimations(current_time_ms);
      layout_reusable_ = false;
      updated = true;
    }
    if (element->IsAnimating() && element->AnimatesLayout()) {
      layout_reusable_ = false;
    }
    updated |= element->TickTransitions(current_time_ms);
    for (size_t i = 0; i < element->ChildCount(); ++i) {
      if (TickAll(element->ChildAt(i), hidden)) {
        updated = true;
      }
    }
    return updated;
  };
  bool updated = TickAll(component_->Root(), false);
  if (DispatchAnimationEnds()) {
    layout_reusable_ = false;
    component_->Digest();
    updated = true;
  }
  return updated;
}

bool ScreenImpl::DispatchAnimationEnds() {
  // Collected first: a handler may change the tree being walked.
  std::vector<Ref<Element>> ended;
  std::function<void(Element*)> Collect = [&](Element* element) {
    if (element->TakeEndedAnimations() &&
        element->GetAttribute("onanimationend")) {
      ended.emplace_back(element);
    }
    for (size_t i = 0; i < element->ChildCount(); ++i) {
      Collect(element->ChildAt(i));
    }
  };
  Collect(component_->Root());

  for (const Ref<Element>& element : ended) {
    const std::string* action = element->GetAttribute("onanimationend");
    if (!action || action->empty()) {
      continue;
    }
    std::string callback_name = *action;
    std::string callback_arg;
    const size_t paren_open = action->find('(');
    if (paren_open != std::string::npos && action->ends_with(')')) {
      callback_name = action->substr(0, paren_open);
      callback_arg =
          action->substr(paren_open + 1, action->size() - paren_open - 2);
    }
    bool executed = false;
    for (ComponentBase* comp = GetAttributeOwnerComponent(element.get());
         comp && !executed; comp = GetParentComponent(comp)) {
      executed = comp->RunCallback(callback_name, callback_arg);
    }
    if (!executed) {
      ReportDiagnostic("handler '" + callback_name +
                       "' is not bound in any enclosing component");
    }
  }
  return !ended.empty();
}

void ScreenImpl::Dispatch(Event event) {
  ScopedLayoutArenas arenas(layout_arenas_);
  HandleEvent(event);
}

void ScreenImpl::HandleEvent(Event event) {
  layout_reusable_ = false;
  css::g_terminal_width = width_;
  css::g_terminal_height = height_;

  if (auto* mouse_ptr = event.get_if<Event::Mouse>()) {
    int cell_w = 0, cell_h = 0;
    if (device_->GetCellPixelSize(cell_w, cell_h) && cell_w > 0 && cell_h > 0) {
      mouse_ptr->is_pixel_precise = true;
      mouse_ptr->pixel_x = mouse_ptr->x;
      mouse_ptr->pixel_y = mouse_ptr->y;

      int raw_x = mouse_ptr->x;
      int raw_y = mouse_ptr->y;

      mouse_ptr->x = ((raw_x - 1) / cell_w) + 1;
      mouse_ptr->y = ((raw_y - 1) / cell_h) + 1;

      mouse_ptr->sub_cell_x = static_cast<float>((raw_x - 1) % cell_w) / cell_w;
      mouse_ptr->sub_cell_y = static_cast<float>((raw_y - 1) % cell_h) / cell_h;
    }
    // Inline, the terminal reports the mouse on its whole screen: from the
    // frame's top row instead. A press or a wheel turn outside the frame is
    // on the shell's lines, not the application's, and so is its release; a
    // drag started inside may go on outside.
    if (inline_) {
      if (inline_top_ < 0) {
        return;
      }
      mouse_ptr->y -= inline_top_;
      mouse_ptr->pixel_y -= inline_top_ * cell_h;
      const bool outside = mouse_ptr->y < 1 || mouse_ptr->y > frame_height_;
      const bool wheel = mouse_ptr->button >= Event::Mouse::WheelUp;
      if (mouse_ptr->motion == Event::Mouse::Motion::Pressed) {
        if (outside) {
          return;
        }
        inline_pressed_ = inline_pressed_ || !wheel;
      } else if (mouse_ptr->motion == Event::Mouse::Motion::Released) {
        if (!inline_pressed_) {
          return;
        }
        inline_pressed_ = false;
      }
    }
  }

  if (auto* mouse = event.get_if<Event::Mouse>();
      mouse && HandleSelection(*mouse)) {
    return;
  }

  // `:focus-visible` follows the last kind of input. Typing after a click
  // brings the focus ring back without moving focus, so nothing else would
  // re-match it.
  if (event.is<Event::Keyboard>()) {
    if (PointerInteraction()) {
      SetPointerInteraction(false);
      component_->ResolveTargetStyles();
    }
  } else if (event.is<Event::Mouse>() && event.get<Event::Mouse>().motion ==
                                             Event::Mouse::Motion::Pressed) {
    SetPointerInteraction(true);
  }

  if (event.is<Event::Mouse>()) {
    auto mouse = event.get<Event::Mouse>();

    bool state_changed = false;
    if (root_fragment_) {
      int tx = mouse.x - 1;
      int ty = mouse.y - 1;
      Element* target_el = nullptr;
      if (auto* capturer = ComponentBase::GetMouseCapturer()) {
        target_el = capturer->Root();
      }
      if (!target_el) {
        target_el = FindElementAt(root_fragment_, tx, ty);
      }

      auto IsAncestorOf = [](const Element* element, const Element* target) {
        for (const Element* curr = target; curr; curr = curr->Parent()) {
          if (curr == element) {
            return true;
          }
        }
        return false;
      };

      if (drag_active_) {
        if (mouse.motion == Event::Mouse::Motion::Released) {
          drag_active_ = false;
          drag_element_ = nullptr;
          state_changed = true;
        } else if (mouse.motion == Event::Mouse::Motion::Moved ||
                   mouse.motion == Event::Mouse::Motion::Pressed) {
          int current_mouse = drag_vertical_ ? mouse.y : mouse.x;
          int delta = current_mouse - drag_start_mouse_;

          int delta_eighths = 0;
          int cell_w = 0, cell_h = 0;
          if (mouse.is_pixel_precise &&
              device_->GetCellPixelSize(cell_w, cell_h) && cell_w > 0 &&
              cell_h > 0) {
            int delta_pixels = drag_vertical_
                                   ? (mouse.pixel_y - drag_start_mouse_pixel_)
                                   : (mouse.pixel_x - drag_start_mouse_pixel_);
            delta_eighths =
                (delta_pixels * 8) / (drag_vertical_ ? cell_h : cell_w);
          } else {
            delta_eighths = delta * 8;
          }

          auto frag_opt =
              FindFragmentForElementWithPos(root_fragment_, drag_element_);
          if (frag_opt) {
            auto frag = frag_opt->fragment;
            int w = frag->width;
            int h = frag->height;
            if (drag_vertical_) {
              int scroll_height = drag_element_->scroll_height();
              int padding_vert = drag_element_->style.padding.Vert();
              int border_vert =
                  (frag->has_border && frag->border_style != BorderStyle::None)
                      ? 1
                      : 0;
              int viewport_h = std::max(1, h - 2 * border_vert - padding_vert);
              int track_h = h;
              bool draw_h_scrollbar =
                  (drag_element_->style.overflow_x == Overflow::Scroll &&
                   drag_element_->style.scrollbar_width ==
                       ScrollbarWidth::Auto);
              if (frag->has_border && frag->border_style != BorderStyle::None) {
                track_h = draw_h_scrollbar ? (h - 3) : (h - 2);
              } else {
                track_h = draw_h_scrollbar ? (h - 1) : h;
              }

              int thumb_h_eighths =
                  (viewport_h * track_h * 8) / std::max(1, scroll_height);
              thumb_h_eighths = std::max(8, thumb_h_eighths);
              thumb_h_eighths = std::min(track_h * 8, thumb_h_eighths);

              int max_scroll = scroll_height - h;
              if (max_scroll > 0 && ((track_h * 8) - thumb_h_eighths) > 0) {
                int new_thumb_y_eighths = drag_start_thumb_pos_ + delta_eighths;
                new_thumb_y_eighths = std::clamp(
                    new_thumb_y_eighths, 0, (track_h * 8) - thumb_h_eighths);
                int new_scroll_y =
                    std::round(static_cast<double>(new_thumb_y_eighths) *
                               max_scroll / ((track_h * 8) - thumb_h_eighths));
                if (new_scroll_y != drag_element_->target_scroll_y()) {
                  drag_element_->set_scroll_y(new_scroll_y, false);
                  state_changed = true;
                }
              }
            } else {
              int scroll_width = drag_element_->scroll_width();
              int padding_horiz = drag_element_->style.padding.Horiz();
              int border_horiz =
                  (frag->has_border && frag->border_style != BorderStyle::None)
                      ? 2
                      : 0;
              int viewport_w = std::max(1, w - border_horiz - padding_horiz);
              int track_w = w;
              bool draw_v_scrollbar =
                  (drag_element_->style.overflow_y == Overflow::Scroll &&
                   drag_element_->style.scrollbar_width ==
                       ScrollbarWidth::Auto);
              if (frag->has_border && frag->border_style != BorderStyle::None) {
                track_w = draw_v_scrollbar ? (w - 3) : (w - 2);
              } else {
                track_w = draw_v_scrollbar ? (w - 1) : w;
              }

              int thumb_w_eighths =
                  (viewport_w * track_w * 8) / std::max(1, scroll_width);
              thumb_w_eighths = std::max(8, thumb_w_eighths);
              thumb_w_eighths = std::min(track_w * 8, thumb_w_eighths);

              int max_scroll = scroll_width - w;
              if (max_scroll > 0 && ((track_w * 8) - thumb_w_eighths) > 0) {
                int new_thumb_x_eighths = drag_start_thumb_pos_ + delta_eighths;
                new_thumb_x_eighths = std::clamp(
                    new_thumb_x_eighths, 0, (track_w * 8) - thumb_w_eighths);
                int new_scroll_x =
                    std::round(static_cast<double>(new_thumb_x_eighths) *
                               max_scroll / ((track_w * 8) - thumb_w_eighths));
                if (new_scroll_x != drag_element_->target_scroll_x()) {
                  drag_element_->set_scroll_x(new_scroll_x, false);
                  state_changed = true;
                }
              }
            }
          }
        }
      }

      if (component_->Root()) {
        component_->Root()->Visit([&](Element& el) {
          if (mouse.motion == Event::Mouse::Motion::Pressed) {
            bool should_be_active = IsAncestorOf(&el, target_el);
            if (el.active() != should_be_active) {
              el.set_active(should_be_active);
              state_changed = true;
            }
          } else if (mouse.motion == Event::Mouse::Motion::Released) {
            if (el.active()) {
              el.set_active(false);
              state_changed = true;
            }
          }

          bool should_be_hovered = IsAncestorOf(&el, target_el);
          if (el.hovered() != should_be_hovered) {
            el.set_hovered(should_be_hovered);
            state_changed = true;
          }

          bool scrollbar_hovered = false;
          bool scrollbar_thumb_hovered = false;
          bool scrollbar_active = false;
          bool scrollbar_thumb_active = false;

          if (drag_active_ && drag_element_ == &el) {
            scrollbar_hovered = true;
            scrollbar_thumb_hovered = true;
            scrollbar_active = true;
            scrollbar_thumb_active = true;
          } else if (!drag_active_) {
            auto frag_opt = FindFragmentForElementWithPos(root_fragment_, &el);
            if (frag_opt) {
              bool on_v_thumb = false;
              bool on_h_thumb = false;
              bool on_v_scrollbar = IsCellOnVerticalScrollbar(
                  frag_opt->fragment, frag_opt->abs_x, frag_opt->abs_y, tx, ty,
                  &on_v_thumb);
              bool on_h_scrollbar = IsCellOnHorizontalScrollbar(
                  frag_opt->fragment, frag_opt->abs_x, frag_opt->abs_y, tx, ty,
                  &on_h_thumb);

              if (on_v_scrollbar || on_h_scrollbar) {
                scrollbar_hovered = true;
              }
              if ((on_v_scrollbar && on_v_thumb) ||
                  (on_h_scrollbar && on_h_thumb)) {
                scrollbar_thumb_hovered = true;
              }
            }
          }

          if (el.scrollbar_hovered() != scrollbar_hovered) {
            el.set_scrollbar_hovered(scrollbar_hovered);
            state_changed = true;
          }
          if (el.scrollbar_thumb_hovered() != scrollbar_thumb_hovered) {
            el.set_scrollbar_thumb_hovered(scrollbar_thumb_hovered);
            state_changed = true;
          }
          if (el.scrollbar_active() != scrollbar_active) {
            el.set_scrollbar_active(scrollbar_active);
            state_changed = true;
          }
          if (el.scrollbar_thumb_active() != scrollbar_thumb_active) {
            el.set_scrollbar_thumb_active(scrollbar_thumb_active);
            state_changed = true;
          }
        });
      }
    }

    if (state_changed) {
      const bool tree_changed = component_->Digest();
      component_->ResolveTargetStyles();
      // When more input is already queued, only the last event of the burst
      // needs to paint -- but only if the tree survived unchanged. Draw() is
      // what rebuilds root_fragment_, and every later event in the burst
      // hit-tests against it, so skipping it after reconciliation destroyed
      // elements leaves those raw Element pointers dangling. The batching this
      // gives up is the case it was never for: a burst is mouse motion, which
      // changes hover state and no tree.
      if (suppress_draw_) {
        // Same rule as RequestDraw: the paint can wait for the end of the
        // burst, the layout cannot, or the rest of the burst hit-tests against
        // fragments pointing at destroyed elements.
        if (tree_changed) {
          UpdateLayout();
        }
        draw_pending_ = true;
      } else {
        Draw();
      }
    }

    if (!ComponentBase::GetMouseCapturer() &&
        mouse.motion == Event::Mouse::Motion::Pressed &&
        (mouse.button == Event::Mouse::Button::Left ||
         mouse.button == Event::Mouse::Button::Right)) {
      if (root_fragment_) {
        int tx = mouse.x - 1;
        int ty = mouse.y - 1;
        if (auto* clicked_element = FindElementAt(root_fragment_, tx, ty)) {
          // A disabled element must not gain focus or fire click handlers
          // (matches real browsers: a click on a disabled control doesn't
          // even dispatch, so it can't bubble to an ancestor's onclick
          // either). It still falls through to the generic OnEvent
          // dispatch below, which TextInputBase::OnEventShared already
          // no-ops for a disabled input/textarea.
          bool clicked_is_disabled = clicked_element->disabled();

          // Check scrollbar click first
          Element* curr = clicked_element;
          Element* scrollbar_element = nullptr;
          bool is_vertical = false;
          bool is_on_thumb = false;
          while (curr) {
            auto frag_opt = FindFragmentForElementWithPos(root_fragment_, curr);
            if (frag_opt) {
              bool on_thumb = false;
              if (IsCellOnVerticalScrollbar(frag_opt->fragment, frag_opt->abs_x,
                                            frag_opt->abs_y, tx, ty,
                                            &on_thumb)) {
                scrollbar_element = curr;
                is_vertical = true;
                is_on_thumb = on_thumb;
                break;
              }
              if (IsCellOnHorizontalScrollbar(frag_opt->fragment,
                                              frag_opt->abs_x, frag_opt->abs_y,
                                              tx, ty, &on_thumb)) {
                scrollbar_element = curr;
                is_vertical = false;
                is_on_thumb = on_thumb;
                break;
              }
            }
            curr = curr->Parent();
          }
          if (scrollbar_element) {
          }

          bool handled = false;
          if (scrollbar_element) {
            handled = true;
            if (is_on_thumb && mouse.button == Event::Mouse::Button::Left) {
              // Start drag!
              drag_active_ = true;
              drag_element_ = scrollbar_element;
              drag_vertical_ = is_vertical;
              drag_start_mouse_ = is_vertical ? mouse.y : mouse.x;
              drag_start_mouse_pixel_ =
                  is_vertical ? mouse.pixel_y : mouse.pixel_x;

              auto frag_opt = FindFragmentForElementWithPos(root_fragment_,
                                                            scrollbar_element);
              if (frag_opt) {
                auto frag = frag_opt->fragment;
                int w = frag->width;
                int h = frag->height;
                if (is_vertical) {
                  int scroll_height = scrollbar_element->scroll_height();
                  int padding_vert = scrollbar_element->style.padding.Vert();
                  int border_vert = (frag->has_border &&
                                     frag->border_style != BorderStyle::None)
                                        ? 1
                                        : 0;
                  int viewport_h =
                      std::max(1, h - 2 * border_vert - padding_vert);
                  int track_h = h;
                  bool draw_h_scrollbar =
                      (scrollbar_element->style.overflow_x ==
                           Overflow::Scroll &&
                       scrollbar_element->style.scrollbar_width ==
                           ScrollbarWidth::Auto);
                  if (frag->has_border &&
                      frag->border_style != BorderStyle::None) {
                    track_h = draw_h_scrollbar ? (h - 3) : (h - 2);
                  } else {
                    track_h = draw_h_scrollbar ? (h - 1) : h;
                  }

                  int thumb_h_eighths =
                      (viewport_h * track_h * 8) / std::max(1, scroll_height);
                  thumb_h_eighths = std::max(8, thumb_h_eighths);
                  thumb_h_eighths = std::min(track_h * 8, thumb_h_eighths);

                  int max_scroll = scroll_height - h;
                  int thumb_y_eighths =
                      (max_scroll > 0)
                          ? static_cast<int>(std::round(
                                frag->visual_scroll_y *
                                ((track_h * 8) - thumb_h_eighths) / max_scroll))
                          : 0;

                  drag_start_thumb_pos_ = thumb_y_eighths;
                } else {
                  int scroll_width = scrollbar_element->scroll_width();
                  int padding_horiz = scrollbar_element->style.padding.Horiz();
                  int border_horiz = (frag->has_border &&
                                      frag->border_style != BorderStyle::None)
                                         ? 2
                                         : 0;
                  int viewport_w =
                      std::max(1, w - border_horiz - padding_horiz);
                  int track_w = w;
                  bool draw_v_scrollbar =
                      (scrollbar_element->style.overflow_y ==
                           Overflow::Scroll &&
                       scrollbar_element->style.scrollbar_width ==
                           ScrollbarWidth::Auto);
                  if (frag->has_border &&
                      frag->border_style != BorderStyle::None) {
                    track_w = draw_v_scrollbar ? (w - 3) : (w - 2);
                  } else {
                    track_w = draw_v_scrollbar ? (w - 1) : w;
                  }

                  int thumb_w_eighths =
                      (viewport_w * track_w * 8) / std::max(1, scroll_width);
                  thumb_w_eighths = std::max(8, thumb_w_eighths);
                  thumb_w_eighths = std::min(track_w * 8, thumb_w_eighths);

                  int max_scroll = scroll_width - w;
                  int thumb_x_eighths =
                      (max_scroll > 0)
                          ? static_cast<int>(std::round(
                                frag->visual_scroll_x *
                                ((track_w * 8) - thumb_w_eighths) / max_scroll))
                          : 0;

                  drag_start_thumb_pos_ = thumb_x_eighths;
                }
              }
              frag_opt.reset();  // Release before Draw().

              // Update the styles and redraw since active states changed
              component_->ResolveTargetStyles();
              RequestDraw();
            } else if (!is_on_thumb &&
                       mouse.button == Event::Mouse::Button::Left) {
              // Track click: Page Up/Down or Page Left/Right
              auto frag_opt = FindFragmentForElementWithPos(root_fragment_,
                                                            scrollbar_element);
              if (frag_opt) {
                auto frag = frag_opt->fragment;
                int w = frag->width;
                int h = frag->height;
                if (is_vertical) {
                  int scroll_height = scrollbar_element->scroll_height();
                  int padding_vert = scrollbar_element->style.padding.Vert();
                  int border_vert = (frag->has_border &&
                                     frag->border_style != BorderStyle::None)
                                        ? 1
                                        : 0;
                  int viewport_h =
                      std::max(1, h - 2 * border_vert - padding_vert);
                  int track_y_start = frag_opt->abs_y;
                  int track_h = h;
                  bool draw_h_scrollbar =
                      (scrollbar_element->style.overflow_x ==
                           Overflow::Scroll &&
                       scrollbar_element->style.scrollbar_width ==
                           ScrollbarWidth::Auto);
                  if (frag->has_border &&
                      frag->border_style != BorderStyle::None) {
                    track_y_start = frag_opt->abs_y + 1;
                    track_h = draw_h_scrollbar ? (h - 3) : (h - 2);
                  } else {
                    track_h = draw_h_scrollbar ? (h - 1) : h;
                  }

                  int thumb_h_eighths =
                      (viewport_h * track_h * 8) / std::max(1, scroll_height);
                  thumb_h_eighths = std::max(8, thumb_h_eighths);
                  thumb_h_eighths = std::min(track_h * 8, thumb_h_eighths);

                  int max_scroll = scroll_height - h;
                  int thumb_y_eighths =
                      (max_scroll > 0)
                          ? static_cast<int>(std::round(
                                frag->visual_scroll_y *
                                ((track_h * 8) - thumb_h_eighths) / max_scroll))
                          : 0;

                  int click_y_eighths = (ty - track_y_start) * 8;
                  int current_scroll = scrollbar_element->target_scroll_y();
                  int speed = viewport_h;
                  if (click_y_eighths < thumb_y_eighths) {
                    scrollbar_element->set_scroll_y(
                        std::max(0, current_scroll - speed), false);
                  } else {
                    scrollbar_element->set_scroll_y(
                        std::min(max_scroll, current_scroll + speed), false);
                  }
                } else {
                  int scroll_width = scrollbar_element->scroll_width();
                  int padding_horiz = scrollbar_element->style.padding.Horiz();
                  int border_horiz = (frag->has_border &&
                                      frag->border_style != BorderStyle::None)
                                         ? 2
                                         : 0;
                  int viewport_w =
                      std::max(1, w - border_horiz - padding_horiz);
                  int track_x_start = frag_opt->abs_x;
                  int track_w = w;
                  bool draw_v_scrollbar =
                      (scrollbar_element->style.overflow_y ==
                           Overflow::Scroll &&
                       scrollbar_element->style.scrollbar_width ==
                           ScrollbarWidth::Auto);
                  if (frag->has_border &&
                      frag->border_style != BorderStyle::None) {
                    track_x_start = frag_opt->abs_x + 1;
                    track_w = draw_v_scrollbar ? (w - 3) : (w - 2);
                  } else {
                    track_w = draw_v_scrollbar ? (w - 1) : w;
                  }

                  int thumb_w_eighths =
                      (viewport_w * track_w * 8) / std::max(1, scroll_width);
                  thumb_w_eighths = std::max(8, thumb_w_eighths);
                  thumb_w_eighths = std::min(track_w * 8, thumb_w_eighths);

                  int max_scroll = scroll_width - w;
                  int thumb_x_eighths =
                      (max_scroll > 0)
                          ? static_cast<int>(std::round(
                                frag->visual_scroll_x *
                                ((track_w * 8) - thumb_w_eighths) / max_scroll))
                          : 0;

                  int click_x_eighths = (tx - track_x_start) * 8;
                  int current_scroll = scrollbar_element->target_scroll_x();
                  int speed = viewport_w;
                  if (click_x_eighths < thumb_x_eighths) {
                    scrollbar_element->set_scroll_x(
                        std::max(0, current_scroll - speed), false);
                  } else {
                    scrollbar_element->set_scroll_x(
                        std::min(max_scroll, current_scroll + speed), false);
                  }
                }
                frag = nullptr;    // Release before Draw().
                frag_opt.reset();  // Release before Draw().
                RequestDraw();
              }
            }
          }

          if (handled) {
            return;
          }

          if (!clicked_is_disabled) {
            // The focus goes to the focusable element the click landed in,
            // as in a browser: a click on a button's label focuses the
            // button, not the label's text. A click on nothing focusable
            // focuses what it landed on, which is where keys then go.
            Element* focus_target = clicked_element;
            for (Element* el = clicked_element; el; el = el->Parent()) {
              if (GetEffectiveTabIndex(el)) {
                focus_target = el;
                break;
              }
            }
            bool focus_changed = (focused_element_ != focus_target);
            if (component_->Root()) {
              component_->Root()->Visit(
                  [](Element& el) { el.set_focused(false); });
            }
            focused_element_ = focus_target;
            focused_element_->set_focused(true);
            // No ScrollIntoView() here: a mouse click always lands on an
            // already-visible cell, unlike Tab/arrow-key focus movement
            // (which does call it, see above/below) where the newly focused
            // element may be off-screen. ScrollIntoView() computes an
            // element's position from its fragment's unscrolled content-space
            // bounding box, so for an element taller than its scrollable
            // ancestor's viewport (e.g. a textarea's text content, clicked
            // while scrolled down) it would always resolve to that box's top
            // edge and scroll back to it -- clobbering the scroll offset the
            // click's own row/column math is about to read.
            if (focus_changed) {
              component_->ResolveTargetStyles();
              RequestDraw();
            }
          }
          std::vector<std::string> attr_keys;
          if (mouse.button == Event::Mouse::Button::Left) {
            attr_keys = {"onclick", "@click.left", "@click"};
          } else {
            attr_keys = {"oncontextmenu", "@click.right"};
          }

          curr = clicked_element;
          handled = false;
          while (curr && !clicked_is_disabled) {
            if (FollowLink(curr)) {
              handled = true;
              break;
            }

            std::string action;
            const auto& attrs = curr->Attributes();
            for (const auto& key : attr_keys) {
              if (attrs.count(key)) {
                action = attrs.at(key);
                break;
              }
            }

            if (!action.empty()) {
              std::string callback_name = action;
              std::string callback_arg = "";

              size_t paren_open = action.find('(');
              if (paren_open != std::string::npos && action.ends_with(')')) {
                callback_name = action.substr(0, paren_open);
                callback_arg = action.substr(paren_open + 1,
                                             action.size() - paren_open - 2);
              }

              ComponentBase* comp = GetAttributeOwnerComponent(curr);
              bool executed = false;
              while (comp) {
                if (comp->RunCallback(callback_name, callback_arg)) {
                  DigestAndDraw();
                  handled = true;
                  executed = true;
                  break;
                }
                comp = GetParentComponent(comp);
              }
              if (!executed) {
                ReportDiagnostic("handler '" + callback_name +
                                 "' is not bound in any enclosing component");
              }
              if (executed) {
                break;
              }
            }
            curr = curr->Parent();
          }
          if (handled) {
            return;
          }
        }
      }
    } else if (!ComponentBase::GetMouseCapturer() &&
               (mouse.button == Event::Mouse::Button::WheelUp ||
                mouse.button == Event::Mouse::Button::WheelDown ||
                mouse.button == Event::Mouse::Button::WheelLeft ||
                mouse.button == Event::Mouse::Button::WheelRight)) {
      if (root_fragment_) {
        int tx = mouse.x - 1;
        int ty = mouse.y - 1;
        // Collect scroll info from fragments first, then release them before
        // calling Draw() to avoid use-after-arena-reset (Draw() resets the
        // arena while fragment shared_ptrs might still be in scope).
        struct ScrollAction {
          Element* element = nullptr;
          bool is_vertical = false;
          int new_value = 0;
        };
        ScrollAction action;

        // What the wheel is over, the mouse passing through elements with
        // pointer-events: none, then the first of it and its ancestors that
        // scrolls.
        if (Element* target = FindElementAt(root_fragment_, tx, ty)) {
          Element* curr = target;
          while (curr) {
            bool is_horizontal_wheel =
                (mouse.button == Event::Mouse::Button::WheelLeft ||
                 mouse.button == Event::Mouse::Button::WheelRight);
            if (curr->style.overflow_y == Overflow::Scroll &&
                !is_horizontal_wheel) {
              auto frag = FindFragmentForElement(root_fragment_, curr);
              if (frag) {
                int scroll_height = curr->scroll_height();
                int max_scroll = std::max(0, scroll_height - frag->height);
                int curr_y = curr->target_scroll_y();
                int speed = curr->style.scroll_speed_y;
                frag = nullptr;  // Release before Draw().

                int new_y = curr_y;
                if (mouse.button == Event::Mouse::Button::WheelUp) {
                  new_y = std::max(0, curr_y - speed);
                } else {
                  new_y = std::min(max_scroll, curr_y + speed);
                }

                if (new_y != curr_y) {
                  action = {curr, true, new_y};
                  break;
                }
              }
            } else if (curr->style.overflow_x == Overflow::Scroll) {
              auto frag = FindFragmentForElement(root_fragment_, curr);
              if (frag) {
                int scroll_width = curr->scroll_width();
                int max_scroll = std::max(0, scroll_width - frag->width);
                int curr_x = curr->target_scroll_x();
                int speed = curr->style.scroll_speed_x;
                frag = nullptr;  // Release before Draw().

                int new_x = curr_x;
                if (mouse.button == Event::Mouse::Button::WheelLeft ||
                    mouse.button == Event::Mouse::Button::WheelUp) {
                  new_x = std::max(0, curr_x - speed);
                } else {
                  new_x = std::min(max_scroll, curr_x + speed);
                }

                if (new_x != curr_x) {
                  action = {curr, false, new_x};
                  break;
                }
              }
            }
            curr = curr->Parent();
          }
        }

        if (action.element) {
          if (action.is_vertical) {
            action.element->set_scroll_y(action.new_value, false);
          } else {
            action.element->set_scroll_x(action.new_value, false);
          }
          RequestDraw();
          return;
        }
      }
    }
  }

  bool event_handled = false;
  if (event.is<Event::Mouse>() && ComponentBase::GetMouseCapturer()) {
    event_handled = ComponentBase::GetMouseCapturer()->OnEvent(event);
  } else {
    event_handled = component_->OnEvent(event);
  }

  if (event_handled) {
    DigestAndDraw();
    return;
  }

  if (event == Event::Tab() || event == Event::TabReverse()) {
    std::vector<Element*> document_order;
    std::function<void(Element*)> CollectAll = [&](Element* el) {
      if (!el || el->style.display_none) {
        return;
      }
      document_order.push_back(el);
      for (const auto& child : el->children()) {
        CollectAll(child.get());
      }
    };
    if (component_->Root()) {
      CollectAll(component_->Root());
    }

    struct FocusEntry {
      Element* element;
      int tabindex;
      int document_index;
    };

    std::vector<FocusEntry> navigable;
    for (int i = 0; i < static_cast<int>(document_order.size()); ++i) {
      Element* el = document_order[i];
      auto tab_index_opt = GetEffectiveTabIndex(el);
      if (tab_index_opt.has_value() && tab_index_opt.value() >= 0) {
        navigable.push_back({el, tab_index_opt.value(), i});
      }
    }

    if (!navigable.empty()) {
      std::sort(navigable.begin(), navigable.end(),
                [](const FocusEntry& a, const FocusEntry& b) {
                  if (a.tabindex > 0 && b.tabindex > 0) {
                    if (a.tabindex != b.tabindex) {
                      return a.tabindex < b.tabindex;
                    }
                    return a.document_index < b.document_index;
                  }
                  if (a.tabindex > 0) {
                    return true;
                  }
                  if (b.tabindex > 0) {
                    return false;
                  }
                  return a.document_index < b.document_index;
                });

      int curr_idx = -1;
      for (int i = 0; i < static_cast<int>(navigable.size()); ++i) {
        if (navigable[i].element->focused()) {
          curr_idx = i;
          break;
        }
      }

      int next_idx = 0;
      if (event == Event::Tab()) {
        next_idx = (curr_idx == -1) ? 0 : (curr_idx + 1) % navigable.size();
      } else {
        next_idx = (curr_idx == -1)
                       ? (static_cast<int>(navigable.size()) - 1)
                       : (curr_idx - 1 + navigable.size()) % navigable.size();
      }

      if (component_->Root()) {
        component_->Root()->Visit([](Element& el) { el.set_focused(false); });
      }
      navigable[next_idx].element->set_focused(true);
      focused_element_ = navigable[next_idx].element;
      component_->ResolveTargetStyles();
      ScrollIntoView(focused_element_);
      RequestDraw();
      return;
    }
  }

  if (event == Event::Return() ||
      (event.is<Event::Keyboard>() &&
       event.get<Event::Keyboard>().codepoint == 32)) {
    if (focused_element_) {
      SimulateClick(focused_element_);
      return;
    }
  }

  if (event == Event::ArrowUp() || event == Event::ArrowDown() ||
      event == Event::ArrowLeft() || event == Event::ArrowRight() ||
      event == Event::h() || event == Event::j() || event == Event::k() ||
      event == Event::l()) {
    if (SpatialNavigate(event)) {
      return;
    }
  }

  if (event == Event::ArrowUp() || event == Event::ArrowDown() ||
      event == Event::PageUp() || event == Event::PageDown() ||
      event == Event::Home() || event == Event::End() ||
      event == Event::ArrowLeft() || event == Event::ArrowRight()) {
    bool is_horiz =
        (event == Event::ArrowLeft() || event == Event::ArrowRight());
    if (focused_element_) {
      Element* curr = focused_element_;
      while (curr) {
        if (is_horiz) {
          if (curr->style.overflow_x == Overflow::Scroll) {
            auto scroll_frag = FindFragmentForElement(root_fragment_, curr);
            if (scroll_frag) {
              int scroll_width = curr->scroll_width();
              int max_scroll = std::max(0, scroll_width - scroll_frag->width);
              int curr_x = curr->target_scroll_x();
              int speed = curr->style.scroll_speed_x;
              scroll_frag = nullptr;  // Release before Draw().

              int delta = (event == Event::ArrowLeft()) ? -speed : speed;
              int new_x = std::clamp(curr_x + delta, 0, max_scroll);
              if (new_x != curr_x) {
                curr->set_scroll_x(new_x, false);
                RequestDraw();
                return;
              }
            }
          }
        } else {
          if (curr->style.overflow_y == Overflow::Scroll) {
            auto scroll_frag = FindFragmentForElement(root_fragment_, curr);
            if (scroll_frag) {
              int scroll_height = curr->scroll_height();
              int max_scroll = std::max(0, scroll_height - scroll_frag->height);
              int curr_y = curr->target_scroll_y();
              int speed = curr->style.scroll_speed_y;

              int delta = 0;
              if (event == Event::ArrowUp()) {
                delta = -speed;
              } else if (event == Event::ArrowDown()) {
                delta = speed;
              } else if (event == Event::PageUp()) {
                delta = -scroll_frag->height;
              } else if (event == Event::PageDown()) {
                delta = scroll_frag->height;
              }
              scroll_frag = nullptr;  // Release before Draw().

              int new_y;
              if (event == Event::Home()) {
                new_y = 0;
              } else if (event == Event::End()) {
                new_y = max_scroll;
              } else {
                new_y = std::clamp(curr_y + delta, 0, max_scroll);
              }
              if (new_y != curr_y) {
                curr->set_scroll_y(new_y, false);
                RequestDraw();
                return;
              }
            }
          }
        }
        curr = curr->Parent();
      }
    }

    if (auto scroll_frag = FindFirstScrollableFragment(root_fragment_)) {
      if (is_horiz) {
        if (scroll_frag->dom_node->style.overflow_x == Overflow::Scroll) {
          int scroll_width = scroll_frag->dom_node->scroll_width();
          int max_scroll = std::max(0, scroll_width - scroll_frag->width);
          int curr_x = scroll_frag->dom_node->target_scroll_x();
          int speed = scroll_frag->dom_node->style.scroll_speed_x;
          Element* el = scroll_frag->dom_node;
          scroll_frag = nullptr;  // Release before Draw().

          int delta = (event == Event::ArrowLeft()) ? -speed : speed;
          int new_x = std::clamp(curr_x + delta, 0, max_scroll);
          if (new_x != curr_x) {
            el->set_scroll_x(new_x, false);
            RequestDraw();
            return;
          }
        }
      } else {
        if (scroll_frag->dom_node->style.overflow_y == Overflow::Scroll) {
          int scroll_height = scroll_frag->dom_node->scroll_height();
          int max_scroll = std::max(0, scroll_height - scroll_frag->height);
          int curr_y = scroll_frag->dom_node->target_scroll_y();
          int speed = scroll_frag->dom_node->style.scroll_speed_y;

          int delta = 0;
          if (event == Event::ArrowUp()) {
            delta = -speed;
          } else if (event == Event::ArrowDown()) {
            delta = speed;
          } else if (event == Event::PageUp()) {
            delta = -scroll_frag->height;
          } else if (event == Event::PageDown()) {
            delta = scroll_frag->height;
          }
          Element* el = scroll_frag->dom_node;
          scroll_frag = nullptr;  // Release before Draw().

          int new_y;
          if (event == Event::Home()) {
            new_y = 0;
          } else if (event == Event::End()) {
            new_y = max_scroll;
          } else {
            new_y = std::clamp(curr_y + delta, 0, max_scroll);
          }
          if (new_y != curr_y) {
            el->set_scroll_y(new_y, false);
            RequestDraw();
            return;
          }
        }
      }
    }
  }

  if (event == Event::Escape() || event == Event::CtrlC()) {
    running_ = false;
    return;
  }
}

// The half of Draw() that every event needs, including one whose paint is
// batched away. Hit-testing reads raw Element pointers out of root_fragment_,
// so letting it go stale across a reconciliation is a use-after-free rather
// than merely a wrong answer -- reconciliation destroys the elements it
// replaces, and the fragments still point at them.
namespace {

// How far down `fragment`, at row `y`, draws: its own rows, and those of
// what overflows it, like a popup below its last row.
int InkBottom(const PhysicalFragment& fragment, int y) {
  int bottom = y + fragment.height;
  if (fragment.clips_descendants) {
    return bottom;
  }
  for (const auto& child : fragment.children) {
    bottom = std::max(bottom, InkBottom(*child.fragment, y + child.y));
  }
  return bottom;
}

}  // namespace

void ScreenImpl::UpdateLayout(bool reuse_fragments) {
  css::g_terminal_width = width_;
  css::g_terminal_height = height_;

  // Anything that changed the DOM since the last frame gets its styles
  // resolved here, rather than at each of the places that can change it. A
  // Digest() override is the awkward case -- <tabs> and <select> re-tag
  // elements from theirs, which runs after Render() already resolved styles --
  // but drawing is the one thing every path has in common, so this is the
  // point where nothing can bypass it. Elements whose resolved styles are
  // still valid are skipped, so an unchanged tree costs a single walk.
  component_->ResolveStyles();
  // A :has() rule depends on elements other than the one it styles. Those are
  // its own component's, which change when that component renders -- but not
  // only then: <tabs> and <select> re-tag the panes and options their user
  // wrote from their own Digest(). The pseudo-class pass is where :has()
  // rules are matched, so whenever styles changed it runs over the whole tree.
  if (TakeBaseStylesResolved() &&
      ComponentInternals::UsesRelationalSelectors(*component_)) {
    component_->ResolveTargetStyles();
  }

  auto root = component_->Root();

  // An element that appeared with `autofocus` takes the focus, provided it is
  // still in the document.
  if (Ref<Element> autofocus = TakePendingAutofocus(); autofocus && root) {
    const Element* top = autofocus.get();
    while (top->Parent()) {
      top = top->Parent();
    }
    if (top == root) {
      root->Visit([](Element& el) { el.set_focused(false); });
      autofocus->set_focused(true);
      component_->ResolveTargetStyles();
    }
  }

  focused_element_ = nullptr;
  if (root && Element::FocusedCount() > 0) {
    std::function<void(Element*)> FindFocused = [&](Element* el) {
      if (!el) {
        return;
      }
      if (el->focused()) {
        focused_element_ = el;
        return;
      }
      for (size_t i = 0; i < el->ChildCount(); ++i) {
        FindFocused(el->ChildAt(i));
        if (focused_element_) {
          return;
        }
      }
    };
    FindFocused(root);
  }

  // The last tree stays alive until this one is laid out: the fragments
  // reused from it are copied, and its text fragments view its boxes' text.
  // Its fragments are in the other arena, which ResetLayoutArena() keeps; its
  // boxes are held here.
  auto previous_box = std::move(root_box_);
  ResetLayoutArena();
  layout_arenas_.reuse_fragments = reuse_fragments &&
                                   width_ == laid_out_width_ &&
                                   height_ == laid_out_height_;
  laid_out_width_ = width_;
  laid_out_height_ = height_;
  const int reuse_count = LayoutReuseCount();
  const int run_count = LayoutRunCount();

  auto root_box = LayoutTreeBuilder::Build(root, {}, /*may_defer=*/true);

  // Inline, the application is as tall as its content, as a web page is;
  // the terminal's height is only the most it is shown at.
  LayoutConstraints viewport = {
      {width_, MeasureMode::Exactly},
      {height_, inline_ ? MeasureMode::AtMost : MeasureMode::Exactly},
  };

  PhysicalFragment* root_fragment = nullptr;
  if (root_box) {
    root_fragment = RunLayout({root_box.get()}, viewport);
    ApplyTranslate(*root_fragment);
  }
  root_fragment_ = root_fragment;
  root_box_ = root_box;
  frame_height_ = !inline_ ? height_
                  : root_fragment_
                      ? std::clamp(InkBottom(*root_fragment_, 0), 0, height_)
                      : 0;
  reused_fragments_ = LayoutReuseCount() != reuse_count;
  if (!root_fragment_ || !reused_fragments_ || LayoutRunCount() != run_count) {
    laid_out_since_paint_ = true;
  }
  // Before laying out again below, which resets the arena they are in.
  previous_box = nullptr;

  // A `<for virtual="">` renders the items around what its scroll container
  // shows. Layout is what sizes and scrolls that container, so once laid out,
  // the loops it has moved past render again -- Digest() finds them -- and
  // the frame is laid out once more with their new items.
  if (!relayout_for_virtual_lists_ &&
      ComponentInternals::HasVirtualLists(*component_) &&
      component_->Digest()) {
    relayout_for_virtual_lists_ = true;
    UpdateLayout(reuse_fragments);
    relayout_for_virtual_lists_ = false;
  }
  layout_reusable_ = true;
}

void ScreenImpl::Draw(bool reuse_layout) {
  ScopedLayoutArenas arenas(layout_arenas_);
  draw_pending_ = false;
  // A DOM changed from outside the loop (a test calling Digest()) shows as
  // stale styles.
  reuse_layout = reuse_layout && layout_reusable_ && root_fragment_ &&
                 !component_->StylesNeedResolve();
  if (reuse_layout) {
    RefreshPaint(*root_fragment_);
    ApplyTranslate(*root_fragment_);
  } else {
    UpdateLayout();
  }

  auto root = component_->Root();
  const auto& root_fragment = root_fragment_;

  // A layout reused whole paints what the last frame painted.
  const bool repaint = reuse_layout || laid_out_since_paint_ ||
                       !last_texture_ || last_texture_->width() != width_ ||
                       last_texture_->height() != frame_height_ ||
                       painted_background_set_ != background_color_set_ ||
                       painted_background_ != background_color_ ||
                       painted_terminal_background_ != terminal_background_ ||
                       selection_painted_ || SelectionShown();
  // The selection, in reverse video over the text it takes.
  PaintSelection selection;
  if (SelectionShown()) {
    UpdateSelectionOrder();
    selection.selected = [this](const TextPosition& glyph) {
      return Selected(glyph);
    };
  }
  const PaintSelection* shown_selection =
      SelectionShown() ? &selection : nullptr;
  Texture texture = repaint ? Texture(width_, frame_height_) : *last_texture_;
  if (repaint && root_fragment) {
    PaintScreen(*root_fragment, texture, shown_selection);
  }
  laid_out_since_paint_ = false;
  painted_background_set_ = background_color_set_;
  painted_background_ = background_color_;
  painted_terminal_background_ = terminal_background_;

  // RTXUI_VERIFY_LAYOUT_REUSE (set for the tests) lays out again anyway,
  // from scratch, and stops if that draws anything else.
  if ((reuse_layout || reused_fragments_) &&
      std::getenv("RTXUI_VERIFY_LAYOUT_REUSE")) {
    UpdateLayout(/*reuse_fragments=*/false);
    Texture fresh(width_, frame_height_);
    if (root_fragment_) {
      PaintScreen(*root_fragment_, fresh, shown_selection);
    }
    if (fresh.Render() != texture.Render()) {
      std::cerr << "rtxui: the reused layout drew\n"
                << texture.Render() << "\ninstead of\n"
                << fresh.Render() << "\n";
      std::abort();
    }
  }

  selection_painted_ = SelectionShown();

  std::string new_output;
  bool use_diff = has_drawn_ && last_texture_ &&
                  last_texture_->width() == width_ &&
                  last_texture_->height() == height_;
  if (inline_) {
    device_->Write("\x1b[?2026h");
    WriteInlineFrame(texture, repaint);
  } else {
    if (use_diff) {
      new_output = repaint ? texture.RenderDiff(*last_texture_)
                           : texture.RenderUnchanged();
    } else {
      new_output = texture.Render();
    }

    device_->Write("\x1b[?2026h");

    if (has_drawn_) {
      if (last_height_ > 0) {
        device_->Write("\r\x1b[" + std::to_string(last_height_) + "A");
      }
    } else {
      device_->Write("\x1b[2J\x1b[H");
    }

    device_->Write(new_output);
  }

  Element* cursor_element = nullptr;
  if (root) {
    std::function<void(Element*)> FindCursor = [&](Element* el) {
      if (!el) {
        return;
      }
      if (std::find(el->classes().begin(), el->classes().end(),
                    "cursor-focused") != el->classes().end()) {
        cursor_element = el;
        return;
      }
      for (size_t i = 0; i < el->ChildCount(); ++i) {
        FindCursor(el->ChildAt(i));
        if (cursor_element) {
          return;
        }
      }
    };
    // Only a focused input shows its cursor, so it is under the focused
    // element: no need to walk the whole tree every frame.
    FindCursor(focused_element_);
  }

  if (inline_) {
    // From the frame's top-left, where WriteInlineFrame() left the cursor.
    const int cx = cursor_element ? cursor_element->absolute_x() : -1;
    const int cy = cursor_element ? cursor_element->absolute_y() : -1;
    if (cursor_element && IsWithinScrollClip(cursor_element) && cx >= 0 &&
        cx < width_ && cy >= 0 && cy < frame_height_) {
      std::string move = "\x1b[?25h\x1b[5 q";
      if (cy > 0) {
        move += "\x1b[" + std::to_string(cy) + "B";
      }
      if (cx > 0) {
        move += "\x1b[" + std::to_string(cx) + "C";
      }
      device_->Write(move);
      rest_x_ = cx;
      rest_y_ = cy;
    } else {
      device_->Write("\x1b[?25l");
      rest_x_ = 0;
      rest_y_ = 0;
    }
  } else if (cursor_element && IsWithinScrollClip(cursor_element)) {
    int cx = cursor_element->absolute_x() + 1;
    int cy = cursor_element->absolute_y() + 1;
    device_->Write("\x1b[?25h\x1b[5 q\x1b[" + std::to_string(cy) + ";" +
                   std::to_string(cx) + "H");
  } else {
    device_->Write("\x1b[?25l");
  }

  // OSC 52: sets the system clipboard via the terminal itself, so it works
  // over SSH without any host clipboard tool dependency. Most terminals
  // allow this "write" direction; reading the clipboard back this way is
  // commonly disabled for security, which is why paste instead relies on
  // bracketed paste mode.
  if (auto text = ComponentBase::TakePendingClipboardWrite()) {
    device_->Write("\x1b]52;c;" + Base64Encode(*text) + "\x07");
  }

  device_->Write("\x1b[?2026l");

  if (use_diff) {
    last_height_ = height_ - 1;
  } else {
    last_height_ = 0;
    for (char ch : new_output) {
      if (ch == '\n') {
        last_height_++;
      }
    }
  }
  last_texture_ = std::make_unique<Texture>(std::move(texture));
  has_drawn_ = true;
}

void ScreenImpl::UpdateSize() {
  ScopedLayoutArenas arenas(layout_arenas_);
  int new_width = width_;
  int new_height = height_;
  if (device_->GetSize(new_width, new_height)) {
    if (new_width != width_ || new_height != height_) {
      layout_reusable_ = false;
      width_ = new_width;
      height_ = new_height;
      selection_ = {};
      css::g_terminal_width = width_;
      css::g_terminal_height = height_;
      if (has_drawn_ && inline_) {
        // The terminal may have rewrapped the frame's rows, and moved the
        // cursor with them. From its top-left again, the frame is drawn
        // whole, over what is left of the last one.
        const int above = InlineRowsAboveCursor(new_width);
        device_->Write(above > 0 ? "\r\x1b[" + std::to_string(above) + "A"
                                 : "\r");
        rest_x_ = 0;
        rest_y_ = 0;
        last_texture_.reset();
        inline_top_ = -1;
        component_->Render();
        Draw();
      } else if (has_drawn_) {
        device_->Write("\x1b[2J\x1b[H");
        last_height_ = 0;
        component_->Render();
        Draw();
      }
    }
  }
}

namespace {

// Whether the terminal rewraps its lines when it gets narrower, keeping the
// cursor on the glyph it was on, as most terminals do. xterm, st and the
// Linux console cut them instead.
bool TerminalRewrapsLines() {
  if (std::getenv("TMUX")) {
    return true;
  }
  if (std::getenv("XTERM_VERSION")) {
    return false;
  }
  const std::string_view term = std::getenv("TERM") ? std::getenv("TERM") : "";
  return term != "linux" && term != "st" && !term.starts_with("st-");
}

// `text` with every line break a carriage return and a line feed, ending
// with one: the terminal is in raw mode.
std::string AsLines(std::string_view text) {
  std::string lines;
  for (char c : text) {
    if (c == '\n') {
      lines += '\r';
    }
    lines += c;
  }
  if (lines.empty() || lines.back() != '\n') {
    lines += "\r\n";
  }
  return lines;
}

}  // namespace

void ScreenImpl::WriteInlineFrame(const Texture& texture, bool repaint) {
  // To the top-left of the last frame.
  std::string out = "\r";
  if (rest_y_ > 0) {
    out += "\x1b[" + std::to_string(rest_y_) + "A";
  }
  const int height = texture.height();
  const bool grew = !last_texture_ || height > last_texture_->height();
  // Only on a terminal Loop() runs on: else, what Print() queued is for
  // Loop() to write when it returns, or for a headless run to print.
  const bool print = looping_ && !printed_.empty();
  if (has_drawn_ && last_texture_ && !print &&
      last_texture_->width() == texture.width() &&
      last_texture_->height() == height) {
    if (height > 0) {
      out += repaint ? texture.RenderDiff(*last_texture_)
                     : texture.RenderUnchanged();
    }
  } else {
    // Erases the last frame, and anything the terminal left below it.
    out += "\x1b[J";
    if (print) {
      // Wrapped by the terminal, as any other output, to stay readable
      // whatever its width becomes.
      out += "\x1b[?7h" + AsLines(printed_) + "\x1b[?7l";
      printed_.clear();
    }
    out += texture.Render();
  }
  // Back to the top-left: the frame's rows all start on the line they are
  // drawn on, so the terminal keeps the cursor there whatever it rewraps.
  if (height > 1) {
    out += "\x1b[" + std::to_string(height - 1) + "A";
  }
  out += "\r";
  // The frame scrolled the terminal if it went past its bottom. The terminal
  // says where it is now.
  if (grew || inline_top_ < 0) {
    out += "\x1b[6n";
    ++position_queries_;
    inline_top_ = -1;
  }
  device_->Write(out);
}

int ScreenImpl::InlineRowsAboveCursor(int width) const {
  if (!last_texture_ || width >= last_texture_->width() ||
      !TerminalRewrapsLines()) {
    return rest_y_;
  }
  // Each row above, and the cursor's own up to it, now takes as many rows
  // as it has glyphs for.
  int rows = 0;
  for (int y = 0; y < rest_y_; ++y) {
    rows += std::max(1, (last_texture_->RowEnd(y) + width - 1) / width);
  }
  return rows + rest_x_ / width;
}

void ScreenImpl::LeaveInline() {
  std::string out = "\r";
  if (rest_y_ > 0) {
    out += "\x1b[" + std::to_string(rest_y_) + "A";
  }
  // Each line feed scrolls the terminal if it is at its bottom.
  out += std::string(last_texture_ ? last_texture_->height() : 0, '\n');
  device_->Write(out);
}

void ScreenImpl::Print(std::string text) {
  printed_ += text;
  if (printed_.empty() || printed_.back() != '\n') {
    printed_ += '\n';
  }
  if (inline_ && looping_) {
    RequestDraw();
  }
}

void ScreenImpl::Print(Ref<ComponentBase> component) {
  // Laid out alone, as tall as it is, as an inline frame.
  constexpr int kMaxRows = 1000;
  const int terminal_width = css::g_terminal_width;
  const int terminal_height = css::g_terminal_height;
  std::string text;
  {
    ScreenImpl alone(
        std::move(component),
        std::make_shared<HeadlessTerminalDevice>(width_, kMaxRows));
    ScopedLayoutArenas arenas(alone.layout_arenas_);
    alone.inline_ = true;
    alone.UpdateLayout(/*reuse_fragments=*/false);
    Texture texture(width_, alone.frame_height_);
    if (alone.root_fragment_) {
      alone.PaintScreen(*alone.root_fragment_, texture);
    }
    if (headless_) {
      alone.last_texture_ = std::make_unique<Texture>(std::move(texture));
      text = alone.Text();
    } else {
      text = texture.Render();
    }
  }
  css::g_terminal_width = terminal_width;
  css::g_terminal_height = terminal_height;
  Print(std::move(text));
}

void ScreenImpl::DigestAndDraw() {
  if (!component_->Digest()) {
    return;
  }
  RequestDraw();
}

void ScreenImpl::RequestDraw() {
  // Batching still skips the paint and the terminal write -- a pasted line
  // must not repaint once per character -- but never the layout, because the
  // next event of the burst is hit-tested against root_fragment_.
  if (suppress_draw_) {
    UpdateLayout();
    draw_pending_ = true;
  } else {
    Draw();
  }
}

void ScreenImpl::ScrollIntoView(Element* element) {
  if (!element || !root_fragment_) {
    return;
  }

  struct FragmentNode {
    PhysicalFragment* fragment;
    int x_rel_parent = 0;
    int y_rel_parent = 0;
  };

  std::vector<FragmentNode> path;

  std::function<bool(PhysicalFragment*)> find_path =
      [&](PhysicalFragment* frag) -> bool {
    if (!frag) {
      return false;
    }
    if (frag->dom_node == element) {
      return true;
    }
    for (const auto& child : frag->children) {
      path.push_back({child.fragment, child.x, child.y});
      if (find_path(child.fragment)) {
        return true;
      }
      path.pop_back();
    }
    return false;
  };

  path.push_back({root_fragment_, 0, 0});
  if (!find_path(root_fragment_)) {
    return;
  }

  // The last node in path is the fragment for element
  int target_left = 0;
  int target_top = 0;
  int target_right = path.back().fragment->width;
  int target_bottom = path.back().fragment->height;

  for (size_t i = path.size() - 1; i > 0; --i) {
    const auto& curr = path[i];
    auto& parent = path[i - 1];

    target_left += curr.x_rel_parent;
    target_right += curr.x_rel_parent;
    target_top += curr.y_rel_parent;
    target_bottom += curr.y_rel_parent;

    if (parent.fragment->dom_node) {
      bool parent_scrollable_y =
          (parent.fragment->dom_node->style.overflow_y == Overflow::Scroll);
      bool parent_scrollable_x =
          (parent.fragment->dom_node->style.overflow_x == Overflow::Scroll);

      if (parent_scrollable_y) {
        int curr_scroll_y = parent.fragment->dom_node->target_scroll_y();
        int new_scroll_y = curr_scroll_y;

        int border_t = 0, border_b = 0, padding_t = 0, padding_b = 0;
        if (parent.fragment->dom_node) {
          padding_t = parent.fragment->dom_node->style.padding.top;
          padding_b = parent.fragment->dom_node->style.padding.bottom;
        }
        if (parent.fragment->has_border &&
            parent.fragment->border_style != BorderStyle::None) {
          border_t = 1;
          border_b = 1;
        }
        int scrollbar_h = 0;
        if (parent.fragment->dom_node &&
            parent.fragment->dom_node->style.overflow_x == Overflow::Scroll &&
            parent.fragment->dom_node->style.scrollbar_width ==
                ScrollbarWidth::Auto) {
          scrollbar_h = 1;
        }
        int viewport_h =
            std::max(1, parent.fragment->height - border_t - padding_t -
                            border_b - padding_b - scrollbar_h);

        // Adjust target coordinates to be relative to the content area,
        // and also include the target element's margins.
        int margin_t = element->style.margin.top;
        int margin_b = element->style.margin.bottom;
        int rel_target_top = target_top - border_t - padding_t - margin_t;
        int rel_target_bottom = target_bottom - border_t - padding_t + margin_b;

        int content_target_top = target_top - border_t - padding_t;

        if (rel_target_top < curr_scroll_y) {
          new_scroll_y = rel_target_top;
        } else if (rel_target_bottom > curr_scroll_y + viewport_h) {
          new_scroll_y = rel_target_bottom - viewport_h;
          if (content_target_top < new_scroll_y) {
            new_scroll_y = content_target_top;
          }
        }

        int scroll_h = parent.fragment->dom_node->scroll_height();
        int max_scroll_y = std::max(0, scroll_h - parent.fragment->height);
        new_scroll_y = std::clamp(new_scroll_y, 0, max_scroll_y);

        if (new_scroll_y != curr_scroll_y) {
          parent.fragment->dom_node->set_scroll_y(new_scroll_y,
                                                  smooth_scroll_enabled_);
          parent.fragment->scroll_y = parent.fragment->dom_node->scroll_y();
        }
      }

      if (parent_scrollable_x) {
        int curr_scroll_x = parent.fragment->dom_node->target_scroll_x();
        int new_scroll_x = curr_scroll_x;

        int border_l = 0, border_r = 0, padding_l = 0, padding_r = 0;
        if (parent.fragment->dom_node) {
          padding_l = parent.fragment->dom_node->style.padding.left;
          padding_r = parent.fragment->dom_node->style.padding.right;
        }
        if (parent.fragment->has_border &&
            parent.fragment->border_style != BorderStyle::None) {
          border_l = 1;
          border_r = 1;
        }
        int scrollbar_w = 0;
        if (parent.fragment->dom_node &&
            parent.fragment->dom_node->style.overflow_y == Overflow::Scroll &&
            parent.fragment->dom_node->style.scrollbar_width ==
                ScrollbarWidth::Auto) {
          scrollbar_w = 1;
        }
        int viewport_w =
            std::max(1, parent.fragment->width - border_l - padding_l -
                            border_r - padding_r - scrollbar_w);

        // Adjust target coordinates to be relative to the content area,
        // and also include the target element's margins.
        int margin_l = element->style.margin.left;
        int margin_r = element->style.margin.right;
        int rel_target_left = target_left - border_l - padding_l - margin_l;
        int rel_target_right = target_right - border_l - padding_l + margin_r;

        int content_target_left = target_left - border_l - padding_l;

        if (rel_target_left < curr_scroll_x) {
          new_scroll_x = rel_target_left;
        } else if (rel_target_right > curr_scroll_x + viewport_w) {
          new_scroll_x = rel_target_right - viewport_w;
          if (content_target_left < new_scroll_x) {
            new_scroll_x = content_target_left;
          }
        }

        int scroll_w = parent.fragment->dom_node->scroll_width();
        int max_scroll_x = std::max(0, scroll_w - parent.fragment->width);
        new_scroll_x = std::clamp(new_scroll_x, 0, max_scroll_x);

        if (new_scroll_x != curr_scroll_x) {
          parent.fragment->dom_node->set_scroll_x(new_scroll_x,
                                                  smooth_scroll_enabled_);
          parent.fragment->scroll_x = parent.fragment->dom_node->scroll_x();
        }
      }
    }

    target_left -= parent.fragment->scroll_x;
    target_right -= parent.fragment->scroll_x;
    target_top -= parent.fragment->scroll_y;
    target_bottom -= parent.fragment->scroll_y;
  }
}

bool ScreenImpl::SpatialNavigate(Event event) {
  enum class Direction { Up, Down, Left, Right } dir;
  if (event == Event::ArrowUp() || event == Event::k()) {
    dir = Direction::Up;
  } else if (event == Event::ArrowDown() || event == Event::j()) {
    dir = Direction::Down;
  } else if (event == Event::ArrowLeft() || event == Event::h()) {
    dir = Direction::Left;
  } else if (event == Event::ArrowRight() || event == Event::l()) {
    dir = Direction::Right;
  } else {
    return false;
  }

  std::vector<FocusableFragment> focusable_fragments;
  // The root viewport is the screen itself.
  CollectFocusableFragments(root_fragment_, 0, 0, 0, 0, width_, frame_height_,
                            focusable_fragments);

  if (focusable_fragments.empty()) {
    return false;
  }

  int cur_x = 0, cur_y = 0, cur_w = 0, cur_h = 0;
  bool start_from_element = false;
  if (focused_element_) {
    for (const auto& f : focusable_fragments) {
      if (f.element == focused_element_) {
        cur_x = f.x;
        cur_y = f.y;
        cur_w = f.width;
        cur_h = f.height;
        start_from_element = true;
        break;
      }
    }
  }

  if (!start_from_element) {
    // If no element focused, start from outside the screen depending on
    // direction
    switch (dir) {
      case Direction::Down:
        cur_x = 0;
        cur_y = -1;
        cur_w = width_;
        cur_h = 0;
        break;
      case Direction::Up:
        cur_x = 0;
        cur_y = frame_height_;
        cur_w = width_;
        cur_h = 0;
        break;
      case Direction::Right:
        cur_x = -1;
        cur_y = 0;
        cur_w = 0;
        cur_h = frame_height_;
        break;
      case Direction::Left:
        cur_x = width_;
        cur_y = 0;
        cur_w = 0;
        cur_h = frame_height_;
        break;
    }
  }

  int cur_cx = cur_x + cur_w / 2;
  int cur_cy = cur_y + cur_h / 2;

  FocusableFragment* best = nullptr;
  long long best_score = -1;

  for (auto& cand : focusable_fragments) {
    if (cand.element == focused_element_) {
      continue;
    }

    int cand_cx = cand.x + cand.width / 2;
    int cand_cy = cand.y + cand.height / 2;

    long long d_primary = 0;
    long long d_secondary = 0;

    switch (dir) {
      case Direction::Left:
        d_primary = cur_cx - cand_cx;
        d_secondary = std::abs(cur_cy - cand_cy);
        break;
      case Direction::Right:
        d_primary = cand_cx - cur_cx;
        d_secondary = std::abs(cur_cy - cand_cy);
        break;
      case Direction::Up:
        d_primary = cur_cy - cand_cy;
        d_secondary = std::abs(cur_cx - cand_cx);
        break;
      case Direction::Down:
        d_primary = cand_cy - cur_cy;
        d_secondary = std::abs(cur_cx - cand_cx);
        break;
    }

    if (d_primary <= 0) {
      continue;
    }

    // Spatial navigation distance metric: primary distance squared + secondary
    // distance squared * 2
    long long score = d_primary * d_primary + d_secondary * d_secondary * 2;
    if (best == nullptr || score < best_score) {
      best = &cand;
      best_score = score;
    }
  }

  if (best) {
    if (component_->Root()) {
      component_->Root()->Visit([](Element& el) { el.set_focused(false); });
    }
    best->element->set_focused(true);
    focused_element_ = best->element;
    component_->ResolveTargetStyles();
    ScrollIntoView(focused_element_);
    RequestDraw();
    return true;
  }

  return false;
}
// Paints the frame against the screen background: the app's, or else the
// terminal's own when it reported one. The terminal's is only borrowed for the
// arithmetic -- what a translucent color or a reversed border cell comes out
// as over it. Wherever it would merely show, the cell is left at the
// terminal's default background, so a translucent or themed terminal still
// shows through, as it does when its color is unknown.
void ScreenImpl::PaintScreen(const PhysicalFragment& root,
                             Texture& texture,
                             const PaintSelection* selection) const {
  if (background_color_set_ || terminal_background_.a == 0) {
    Paint(&root, texture, 0, 0, background_color_, selection);
    return;
  }
  Paint(&root, texture, 0, 0, terminal_background_, selection);
  for (int y = 0; y < texture.height(); ++y) {
    for (int x = 0; x < texture.width(); ++x) {
      Cell& cell = texture[x, y];
      if (cell.background_color == terminal_background_) {
        cell.background_color = Color();
      }
    }
  }
}

namespace {
// How soon a press must follow the last on its cell to make a double or
// triple click.
constexpr double kMultiClickDelayMs = 500;

// How often a selection drag past the edge of its scroll container scrolls it.
constexpr std::chrono::milliseconds kAutoScrollDelay{50};

// The `user-select: all` element `element` is in, if any.
const Element* UserSelectAll(const Element* element) {
  for (; element; element = element->Parent()) {
    if (element->style.user_select) {
      return *element->style.user_select == UserSelect::All ? element : nullptr;
    }
  }
  return nullptr;
}

// The box `element` is laid out in: its parent, a <slot> being no box.
const Element* LayoutParent(const Element* element) {
  const Element* parent = element->Parent();
  while (parent && parent->is_slot()) {
    parent = parent->Parent();
  }
  return parent;
}

// Whether `element` lays out as a block of its own: no inline box, or a
// flex or grid item, whatever its `display`.
bool IsBlockBox(const Element* element) {
  const Element* parent = LayoutParent(element);
  const bool inline_box =
      element->style.display_outside == DisplayOutside::Inline &&
      element->style.display_inside == DisplayInside::Flow;
  const bool item =
      parent && (parent->style.display_inside == DisplayInside::Flex ||
                 parent->style.display_inside == DisplayInside::Grid);
  return !inline_box || item;
}

// Whether `element` holds lines of text, rather than blocks: it is text, or
// none of the boxes in it is a block.
bool IsLineBox(const Element* element) {
  if (element->is_text()) {
    return true;
  }
  return std::ranges::none_of(element->children(), [](const auto& child) {
    return child->is_slot() ? !IsLineBox(child.get()) : IsBlockBox(child.get());
  });
}

// `glyph`, the text drawn for `position`, as its text element has it:
// before text-transform changed its case. Where the laid-out text no longer
// lines up with the element's, whitespace collapsed say, as drawn.
std::string_view AsWritten(const TextPosition& position,
                           std::string_view glyph) {
  if (!position.node->is_text()) {
    return glyph;
  }
  const std::string& text =
      static_cast<const TextElement*>(position.node)->text();
  if (position.offset + glyph.size() > text.size()) {
    return glyph;
  }
  const std::string_view written =
      std::string_view(text).substr(position.offset, glyph.size());
  auto lower = [](char c) {
    return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c;
  };
  return std::ranges::equal(written, glyph, {}, lower, lower) ? written : glyph;
}

bool IsInside(const Element* element, const Element* ancestor) {
  for (; element; element = element->Parent()) {
    if (element == ancestor) {
      return true;
    }
  }
  return false;
}
}  // namespace

Texture ScreenImpl::PaintGlyphs(std::vector<TextPosition>& glyphs) const {
  Texture texture(width_, frame_height_);
  PaintSelection selection;
  selection.glyphs = &glyphs;
  if (root_fragment_) {
    PaintScreen(*root_fragment_, texture, &selection);
  } else {
    glyphs.assign(static_cast<size_t>(width_) * frame_height_, {});
  }
  return texture;
}

TextPosition ScreenImpl::GlyphNear(const std::vector<TextPosition>& glyphs,
                                   int x,
                                   int y) const {
  if (frame_height_ <= 0 || width_ <= 0) {
    return {};
  }
  x = std::clamp(x, 0, width_ - 1);
  y = std::clamp(y, 0, frame_height_ - 1);
  auto at = [&](int cx, int cy) -> const TextPosition& {
    return glyphs[cx + (cy * width_)];
  };
  if (at(x, y).node) {
    return at(x, y);
  }
  struct Cell {
    int x = 0, y = 0;
  };
  // The cells showing text inside `scope`, all of them for none. A screen
  // shows the text of few elements, so each is placed once.
  auto cells_in = [&](const Element* scope) {
    std::unordered_map<const Element*, bool> inside;
    std::vector<Cell> cells;
    for (int cy = 0; cy < frame_height_; ++cy) {
      for (int cx = 0; cx < width_; ++cx) {
        const Element* node = at(cx, cy).node;
        if (!node) {
          continue;
        }
        auto [it, added] = inside.try_emplace(node, false);
        if (added) {
          it->second = !scope || IsInside(node, scope);
        }
        if (it->second) {
          cells.push_back({cx, cy});
        }
      }
    }
    return cells;
  };

  // The text of the element under the mouse, as a browser takes it: a point
  // beside a column's text is in that column, not in the one next to it. An
  // element showing none defers to its parent.
  const Element* scope = FindElementAt(root_fragment_, x, y);
  while (scope && scope->is_slot()) {
    scope = scope->Parent();
  }
  std::vector<Cell> cells = cells_in(scope);
  while (cells.empty() && scope) {
    scope = LayoutParent(scope);
    cells = cells_in(scope);
  }
  if (cells.empty()) {
    return {};
  }
  // Then, down to a box of lines of text, the child of the box the point is
  // nearest: in the direction the box lays its children out, as they are
  // read, then across it.
  while (scope && !IsLineBox(scope)) {
    struct Child {
      const Element* element = nullptr;
      int left = 0, top = 0, right = 0, bottom = 0;
    };
    std::vector<Child> children;
    for (const Cell& cell : cells) {
      const Element* child = at(cell.x, cell.y).node;
      while (child && LayoutParent(child) != scope) {
        child = child->Parent();
      }
      // Not laid out in `scope` after all: its lines are all there is.
      if (!child) {
        children.clear();
        break;
      }
      auto it = std::ranges::find(children, child, &Child::element);
      if (it == children.end()) {
        children.push_back({child, cell.x, cell.y, cell.x, cell.y});
        continue;
      }
      it->left = std::min(it->left, cell.x);
      it->right = std::max(it->right, cell.x);
      it->bottom = std::max(it->bottom, cell.y);
    }
    const bool row = scope->style.display_inside == DisplayInside::Flex &&
                     (scope->style.flex_direction == Direction::Row ||
                      scope->style.flex_direction == Direction::RowReverse);
    const bool grid = scope->style.display_inside == DisplayInside::Grid;
    auto distance = [&](const Child& child) {
      const int dx = std::max({child.left - x, x - child.right, 0});
      const int dy = std::max({child.top - y, y - child.bottom, 0});
      return grid  ? std::pair(dx + dy, 0)
             : row ? std::pair(dx, dy)
                   : std::pair(dy, dx);
    };
    if (children.empty()) {
      break;
    }
    const Child& nearest = *std::ranges::min_element(children, {}, distance);
    scope = nearest.element;
    std::erase_if(cells, [&](const Cell& cell) {
      return !IsInside(at(cell.x, cell.y).node, scope);
    });
  }
  // In it, the end past its last line, the start before its first, else the
  // line the mouse is on, or the end of the one above it.
  const int first_row = cells.front().y;
  const int last_row = cells.back().y;
  if (y < first_row) {
    return at(cells.front().x, cells.front().y);
  }
  if (y > last_row) {
    return at(cells.back().x, cells.back().y);
  }
  std::optional<Cell> before;
  std::optional<Cell> after;
  for (const Cell& cell : cells) {
    if (cell.y < y || (cell.y == y && cell.x <= x)) {
      before = cell;
    } else if (!after) {
      after = cell;
    }
  }
  // On the mouse's row, the glyph before it, else the one after it.
  const Cell& glyph =
      before && (before->y == y || !after || after->y != y) ? *before : *after;
  return at(glyph.x, glyph.y);
}

std::pair<TextPosition, TextPosition> ScreenImpl::UnitAround(
    const std::vector<TextPosition>& glyphs,
    const Texture& texture,
    const TextPosition& glyph,
    Selection::Unit unit) const {
  if (!glyph.node || unit == Selection::Unit::Glyph) {
    return {glyph, glyph};
  }
  if (unit == Selection::Unit::Block) {
    // From the start of the first text in the block to the end of the last.
    const Element* block = glyph.node;
    while (block->Parent() && !IsBlockBox(block)) {
      block = block->Parent();
    }
    const Element* first = nullptr;
    const Element* last = nullptr;
    const_cast<Element*>(block)->Visit([&](Element& element) {
      if (element.is_text()) {
        first = first ? first : &element;
        last = &element;
      }
    });
    return {{first, 0}, {last, UINT32_MAX}};
  }
  // A word: the glyphs of its text on its row that are of its kind, letters
  // and digits, spaces or punctuation.
  auto at = [&](int cx, int cy) -> const TextPosition& {
    return glyphs[cx + (cy * width_)];
  };
  int x = -1;
  int y = -1;
  for (int cy = 0; cy < frame_height_ && x < 0; ++cy) {
    for (int cx = 0; cx < width_; ++cx) {
      if (at(cx, cy).node == glyph.node && at(cx, cy).offset == glyph.offset) {
        x = cx;
        y = cy;
        break;
      }
    }
  }
  if (x < 0) {
    return {glyph, glyph};
  }
  auto kind = [&](int cx) {
    const std::string& c = texture[cx, y].character;
    const unsigned char first = c.empty() ? ' ' : c[0];
    if (first == ' ') {
      return 0;
    }
    return std::isalnum(first) || first == '_' || first >= 0x80 ? 1 : 2;
  };
  const int word = kind(x);
  // Whether the glyph at `cx` is of the word, the second half of a wide one
  // aside.
  auto of_word = [&](int cx) {
    return at(cx, y).node == glyph.node && kind(cx) == word;
  };
  int left = x;
  for (int cx = x - 1; cx >= 0; --cx) {
    if (texture[cx, y].is_continuation) {
      continue;
    }
    if (!of_word(cx)) {
      break;
    }
    left = cx;
  }
  int right = x;
  for (int cx = x + 1; cx < width_; ++cx) {
    if (texture[cx, y].is_continuation) {
      continue;
    }
    if (!of_word(cx)) {
      break;
    }
    right = cx;
  }
  return {at(left, y), at(right, y)};
}

void ScreenImpl::UpdateSelectionOrder() {
  selection_order_.clear();
  if (Element* root = component_->Root()) {
    int index = 0;
    root->Visit(
        [&](Element& element) { selection_order_[&element] = index++; });
  }
  // The pointers are only compared, never read, until found in the document.
  auto gone = [&](const Element* element) {
    return element && !selection_order_.contains(element);
  };
  if (gone(selection_.anchor.node) || gone(selection_.focus.node) ||
      gone(selection_.anchor_all) || gone(selection_.focus_all)) {
    selection_ = {};
  }
}

bool ScreenImpl::Selected(const TextPosition& glyph) const {
  auto order = [&](const Element* element) {
    auto it = selection_order_.find(element);
    return it == selection_order_.end() ? -1 : it->second;
  };
  const int node = order(glyph.node);
  if (node < 0) {
    return false;
  }
  if ((selection_.anchor_all && IsInside(glyph.node, selection_.anchor_all)) ||
      (selection_.focus_all && IsInside(glyph.node, selection_.focus_all))) {
    return true;
  }
  if ((!selection_.dragged && selection_.unit == Selection::Unit::Glyph) ||
      !selection_.anchor.node || !selection_.focus.node) {
    return false;
  }
  auto key = [&](const TextPosition& position) {
    return std::pair(order(position.node), position.offset);
  };
  const auto first = std::min(key(selection_.anchor), key(selection_.focus));
  const auto last =
      std::max(key(selection_.anchor_end), key(selection_.focus_end));
  const auto here = std::pair(node, glyph.offset);
  return first <= here && here <= last;
}

bool ScreenImpl::HandleSelection(const Event::Mouse& mouse) {
  if (mouse.button != Event::Mouse::Button::Left || !root_fragment_) {
    return false;
  }
  const int x = mouse.x - 1;
  const int y = mouse.y - 1;
  switch (mouse.motion) {
    case Event::Mouse::Motion::Pressed: {
      // Presses on one cell in quick succession: a word, then its block.
      const double now = time::GetTimeMs();
      click_count_ = x == last_click_x_ && y == last_click_y_ &&
                             now - last_click_ms_ < kMultiClickDelayMs
                         ? (click_count_ % 3) + 1
                         : 1;
      last_click_x_ = x;
      last_click_y_ = y;
      last_click_ms_ = now;
      // Shift and a click moves the end of the selection there.
      const bool extend =
          mouse.modifier.shift && click_count_ == 1 && selection_.anchor.node;
      if (selection_.Shown() && !extend) {
        RequestDraw();
      }
      const Selection previous = std::exchange(selection_, {});
      // A press on what handles clicks, a link or a `user-select: none`
      // element selects nothing: dragging from a button is no selection.
      Element* target = FindElementAt(root_fragment_, x, y);
      for (const Element* el = target; el; el = el->Parent()) {
        if (el->GetAttribute("onclick") || el->GetAttribute("@click") ||
            el->GetAttribute("@click.left") ||
            (el->tag() == "a" && el->GetAttribute("href"))) {
          return false;
        }
      }
      for (const Element* el = target; el; el = el->Parent()) {
        if (el->style.user_select) {
          if (*el->style.user_select == UserSelect::None) {
            return false;
          }
          break;
        }
      }
      if (!target) {
        return false;
      }
      if (extend) {
        selection_ = previous;
        selection_.armed = true;
        selection_.dragged = true;
        selection_.press_x = selection_.mouse_x = x;
        selection_.press_y = selection_.mouse_y = y;
        UpdateSelectionFocus();
        RequestDraw();
        return false;
      }
      std::vector<TextPosition> glyphs;
      const Texture texture = PaintGlyphs(glyphs);
      selection_.armed = true;
      selection_.press_x = selection_.mouse_x = x;
      selection_.press_y = selection_.mouse_y = y;
      selection_.unit = click_count_ == 3   ? Selection::Unit::Block
                        : click_count_ == 2 ? Selection::Unit::Word
                                            : Selection::Unit::Glyph;
      std::tie(selection_.anchor, selection_.anchor_end) =
          UnitAround(glyphs, texture, GlyphNear(glyphs, x, y), selection_.unit);
      selection_.focus = selection_.anchor;
      selection_.focus_end = selection_.anchor_end;
      selection_.anchor_all = UserSelectAll(target);
      for (Element* el = target; el; el = el->Parent()) {
        if (el->style.overflow_y == Overflow::Scroll ||
            el->style.overflow_x == Overflow::Scroll) {
          selection_.scrollers.push_back(el->HandleTarget());
        }
      }
      if (selection_.Shown()) {
        RequestDraw();
      }
      return false;
    }
    case Event::Mouse::Motion::Moved: {
      // A press an input or a scrollbar took drags them instead.
      if (!selection_.armed || drag_active_ ||
          ComponentBase::GetMouseCapturer()) {
        return false;
      }
      const bool moved =
          selection_.dragged
              ? x != selection_.mouse_x || y != selection_.mouse_y
              : x != selection_.press_x || y != selection_.press_y;
      if (moved) {
        selection_.dragged = true;
        selection_.mouse_x = x;
        selection_.mouse_y = y;
        UpdateSelectionFocus();
        ScheduleAutoScroll();
        RequestDraw();
      }
      return true;
    }
    case Event::Mouse::Motion::Released: {
      if (!selection_.armed) {
        return false;
      }
      selection_.armed = false;
      // A press an input took was a click in it: its own selection.
      if (ComponentBase::GetMouseCapturer()) {
        selection_ = {};
        return false;
      }
      if (selection_.Shown()) {
        UpdateSelectionOrder();
        if (std::string text = SelectedText(); !text.empty()) {
          component_->SetClipboard(text);
          RequestDraw();
        }
      }
      return false;
    }
  }
  return false;
}

std::optional<std::array<int, 4>> ScreenImpl::ScrollerViewport(
    const Element* scroller) const {
  if (!scroller || !root_fragment_) {
    return std::nullopt;
  }
  auto found = FindFragmentForElementWithPos(root_fragment_,
                                             const_cast<Element*>(scroller));
  if (!found) {
    return std::nullopt;
  }
  const PhysicalFragment& fragment = *found->fragment;
  const int border =
      fragment.has_border && fragment.border_style != BorderStyle::None ? 1 : 0;
  const Spacing& padding = scroller->style.padding;
  return std::array<int, 4>{
      found->abs_x + border + padding.left, found->abs_y + border + padding.top,
      found->abs_x + fragment.width - border - padding.right - 1,
      found->abs_y + fragment.height - border - padding.bottom - 1};
}

void ScreenImpl::UpdateSelectionFocus() {
  int x = selection_.mouse_x;
  int y = selection_.mouse_y;
  auto clamp_into = [&](const std::array<int, 4>& viewport) {
    const auto [left, top, right, bottom] = viewport;
    x = std::clamp(x, left, std::max(left, right));
    y = std::clamp(y, top, std::max(top, bottom));
  };
  const AutoScrollPlan plan = PlanAutoScroll();
  if (Element* scrolling =
          plan.y_scroller ? plan.y_scroller : plan.x_scroller) {
    // The edge of what scrolls into view.
    if (auto viewport = ScrollerViewport(scrolling)) {
      clamp_into(*viewport);
    }
  } else {
    // None can scroll further: past all of them, the nearest cell of the
    // outermost; in one, wherever the mouse is.
    std::optional<std::array<int, 4>> outermost;
    bool inside = false;
    for (const auto& handle : selection_.scrollers) {
      if (auto viewport = ScrollerViewport(handle->element)) {
        const auto [left, top, right, bottom] = *viewport;
        inside |= x >= left && x <= right && y >= top && y <= bottom;
        outermost = viewport;
      }
    }
    if (outermost && !inside) {
      clamp_into(*outermost);
    }
  }
  std::vector<TextPosition> glyphs;
  const Texture texture = PaintGlyphs(glyphs);
  std::tie(selection_.focus, selection_.focus_end) =
      UnitAround(glyphs, texture, GlyphNear(glyphs, x, y), selection_.unit);
  selection_.focus_all = UserSelectAll(FindElementAt(root_fragment_, x, y));
}

std::pair<int, int> ScreenImpl::SelectionScrollSpeed(
    const Element* scroller) const {
  auto viewport = ScrollerViewport(scroller);
  if (!viewport) {
    return {0, 0};
  }
  const auto [left, top, right, bottom] = *viewport;
  auto past = [](int mouse, int first, int last, int screen_last) {
    if (mouse < first) {
      return mouse - first;
    }
    if (mouse > last) {
      return mouse - last;
    }
    if (mouse <= 0 && first <= 0) {
      return -1;
    }
    if (mouse >= screen_last && last >= screen_last) {
      return 1;
    }
    return 0;
  };
  return {past(selection_.mouse_x, left, right, width_ - 1),
          past(selection_.mouse_y, top, bottom, frame_height_ - 1)};
}

ScreenImpl::AutoScrollPlan ScreenImpl::PlanAutoScroll() const {
  AutoScrollPlan plan;
  for (const auto& handle : selection_.scrollers) {
    Element* scroller = handle->element;
    auto found = scroller
                     ? FindFragmentForElementWithPos(root_fragment_, scroller)
                     : std::nullopt;
    if (!found) {
      continue;
    }
    const auto [speed_x, speed_y] = SelectionScrollSpeed(scroller);
    // Whether `scroll` can still move by `speed` within [0, max].
    auto can = [](int speed, int scroll, int max) {
      return (speed < 0 && scroll > 0) || (speed > 0 && scroll < max);
    };
    const int max_y =
        std::max(0, scroller->scroll_height() - found->fragment->height);
    if (!plan.y_scroller && scroller->style.overflow_y == Overflow::Scroll &&
        can(speed_y, scroller->target_scroll_y(), max_y)) {
      plan.y_scroller = scroller;
      plan.dy = std::clamp(scroller->target_scroll_y() + speed_y, 0, max_y) -
                scroller->target_scroll_y();
    }
    const int max_x =
        std::max(0, scroller->scroll_width() - found->fragment->width);
    if (!plan.x_scroller && scroller->style.overflow_x == Overflow::Scroll &&
        can(speed_x, scroller->target_scroll_x(), max_x)) {
      plan.x_scroller = scroller;
      plan.dx = std::clamp(scroller->target_scroll_x() + speed_x, 0, max_x) -
                scroller->target_scroll_x();
    }
  }
  return plan;
}

void ScreenImpl::ScheduleAutoScroll() {
  if (autoscroll_pending_) {
    return;
  }
  const AutoScrollPlan plan = PlanAutoScroll();
  if (!plan.x_scroller && !plan.y_scroller) {
    return;
  }
  autoscroll_pending_ = true;
  // The same TaskRunner runs it, so it never outlives the screen.
  task_runner_.PostDelayedTask([this] { AutoScroll(); }, kAutoScrollDelay);
}

void ScreenImpl::AutoScroll() {
  autoscroll_pending_ = false;
  if (!selection_.armed || !selection_.dragged) {
    return;
  }
  const AutoScrollPlan plan = PlanAutoScroll();
  if (!plan.x_scroller && !plan.y_scroller) {
    return;
  }
  if (plan.y_scroller) {
    plan.y_scroller->set_scroll_y(plan.y_scroller->target_scroll_y() + plan.dy,
                                  false);
  }
  if (plan.x_scroller) {
    plan.x_scroller->set_scroll_x(plan.x_scroller->target_scroll_x() + plan.dx,
                                  false);
  }
  // The text now under the mouse is what the selection extends to.
  UpdateLayout();
  UpdateSelectionFocus();
  RequestDraw();
  ScheduleAutoScroll();
}

std::string ScreenImpl::SelectedText() const {
  struct Glyph {
    int order = 0;
    TextPosition position;
    int x = 0, y = 0, width = 0;
    std::string_view text;
  };
  std::vector<Glyph> selected;
  // The laid out text, not the drawn one: what is scrolled out of view is in
  // the selection too. Positions are in the content, scrolling ignored.
  auto visit = [&](auto& self, const PhysicalFragment* fragment, int x,
                   int y) -> void {
    if (fragment->visibility == Visibility::Hidden) {
      return;
    }
    if (fragment->is_text && fragment->text_node &&
        TextSelectable(fragment->text_node)) {
      int column = 0;
      for (const Grapheme& g : Graphemes(fragment->text_content)) {
        if (g.width == 0) {
          continue;
        }
        const TextPosition position{
            fragment->text_node,
            fragment->text_offset +
                static_cast<uint32_t>(g.text.data() -
                                      fragment->text_content.data()),
            fragment->after_wrap && column == 0};
        if (Selected(position)) {
          selected.push_back({selection_order_.at(position.node), position,
                              x + column, y, g.width, g.text});
        }
        column += g.width;
      }
    }
    for (const auto& child : fragment->children) {
      self(self, child.fragment, x + child.x, y + child.y);
    }
  };
  if (root_fragment_) {
    visit(visit, root_fragment_, 0, 0);
  }
  std::ranges::stable_sort(selected, {}, [](const Glyph& glyph) {
    return std::pair(glyph.order, glyph.position.offset);
  });

  std::string text;
  const Glyph* previous = nullptr;
  for (const Glyph& glyph : selected) {
    const int previous_end = previous ? previous->x + previous->width : 0;
    if (previous && glyph.y == previous->y && glyph.x >= previous_end) {
      // Further on the row, past a gap: a border between table cells, or
      // text left out, say. One space stands for it, unless one is there.
      if (glyph.x > previous_end && !text.ends_with(' ') && glyph.text != " ") {
        text += ' ';
      }
    } else if (previous) {
      // On another row. A line a soft wrap began continues the text before
      // it; anything else starts a line of its own.
      text.erase(text.find_last_not_of(' ') + 1);
      if (glyph.position.after_wrap) {
        text += ' ';
      } else {
        text += glyph.y > previous->y + 1 ? "\n\n" : "\n";
      }
    }
    text += AsWritten(glyph.position, glyph.text);
    previous = &glyph;
  }
  text.erase(text.find_last_not_of(' ') + 1);
  return text;
}

bool ScreenImpl::FollowLink(Element* link) {
  if (link->tag() != "a") {
    return false;
  }
  const std::string* href = link->GetAttribute("href");
  if (!href || !href->starts_with("#") || href->size() < 2) {
    return false;
  }
  const std::string_view target_id = std::string_view(*href).substr(1);
  Element* target = nullptr;
  if (component_->Root()) {
    component_->Root()->Visit([&](Element& el) {
      if (el.id == target_id) {
        target = &el;
      }
    });
  }
  if (!target) {
    return false;
  }
  ScrollIntoView(target);
  RequestDraw();
  return true;
}

void ScreenImpl::SimulateClick(Element* element) {
  if (!element) {
    return;
  }
  std::vector<std::string> attr_keys = {"onclick", "@click.left", "@click"};
  Element* curr = element;
  while (curr) {
    if (FollowLink(curr)) {
      return;
    }
    const auto& attrs = curr->Attributes();
    for (const auto& key : attr_keys) {
      if (attrs.count(key)) {
        std::string action = attrs.at(key);
        if (!action.empty()) {
          std::string callback_name = action;
          std::string callback_arg = "";

          size_t paren_open = action.find('(');
          if (paren_open != std::string::npos && action.ends_with(')')) {
            callback_name = action.substr(0, paren_open);
            callback_arg =
                action.substr(paren_open + 1, action.size() - paren_open - 2);
          }

          ComponentBase* comp = GetAttributeOwnerComponent(curr);
          bool executed = false;
          while (comp) {
            if (comp->RunCallback(callback_name, callback_arg)) {
              DigestAndDraw();
              executed = true;
              break;
            }
            comp = GetParentComponent(comp);
          }
          if (!executed) {
            ReportDiagnostic("handler '" + callback_name +
                             "' is not bound in any enclosing component");
          }
          if (executed) {
            return;
          }
        }
      }
    }
    curr = curr->Parent();
  }
}

// --- RawTerminal RAII Implementation ---

ScreenImpl::RawTerminal::RawTerminal(ScreenImpl* screen) : screen_(screen) {
  if (screen_ && screen_->device_) {
    if (screen_->device_->IsAtty()) {
      screen_->device_->EnterRawMode(handle_sigwinch,
                                     /*alternate_screen=*/!screen_->inline_);
      // Ask for the terminal's background color (OSC 11). The reply comes
      // back as input, whenever the terminal sends it, and one that does not
      // answer ignores the question.
      if (!screen_->background_color_set_) {
        screen_->device_->Write("\x1b]11;?\x1b\\");
      }
    }
    screen_->has_drawn_ = false;
    screen_->last_texture_.reset();
    screen_->inline_top_ = -1;
    screen_->position_queries_ = 0;
    screen_->rest_x_ = 0;
    screen_->rest_y_ = 0;
  }
}

ScreenImpl::RawTerminal::~RawTerminal() {
  if (screen_ && screen_->device_ && screen_->device_->IsAtty()) {
    screen_->device_->ExitRawMode();
  }
}

Screen::Screen(Ref<ComponentBase> component,
               std::shared_ptr<TerminalDevice> device)
    : impl_(std::make_unique<ScreenImpl>(std::move(component),
                                         std::move(device))) {}

Screen::~Screen() = default;

void Screen::Loop() {
  impl_->Loop();
}

void Screen::Step() {
  impl_->Step();
}

void Screen::SetInline(bool enabled) {
  ScopedLayoutArenas arenas(impl_->layout_arenas_);
  // On a device that is no terminal, the screen drew from its constructor:
  // it draws again, at its new height.
  const bool drawn = impl_->has_drawn_;
  impl_->inline_ = enabled;
  impl_->has_drawn_ = false;
  impl_->last_texture_.reset();
  if (drawn) {
    impl_->Draw();
  } else {
    impl_->UpdateLayout(/*reuse_fragments=*/false);
  }
}

bool Screen::is_inline() const {
  return impl_->inline_;
}

void Screen::Print(std::string_view text) {
  impl_->Print(std::string(text));
}

void Screen::Print(Ref<ComponentBase> component) {
  impl_->Print(std::move(component));
}

void Screen::Exit() {
  impl_->Exit();
}

void Screen::Dispatch(Event event) {
  impl_->Dispatch(event);
}

void Screen::Draw() {
  impl_->Draw();
}

void Screen::SetSmoothScrollEnabled(bool enabled) {
  impl_->SetSmoothScrollEnabled(enabled);
}

bool Screen::smooth_scroll_enabled() const {
  return impl_->smooth_scroll_enabled();
}

void Screen::SetBackgroundColor(Color color) {
  impl_->SetBackgroundColor(color);
}

Color Screen::background_color() const {
  return impl_->background_color();
}

std::string Screen::Text() const {
  return impl_->Text();
}

std::string ScreenImpl::Text() const {
  std::string text;
  if (!last_texture_) {
    return text;
  }
  for (int y = 0; y < last_texture_->height(); ++y) {
    std::string line;
    for (int x = 0; x < last_texture_->width(); ++x) {
      const Cell& cell = (*last_texture_)[x, y];
      if (cell.is_continuation) {
        continue;
      }
      line += cell.character.empty() ? " " : cell.character;
    }
    line.erase(line.find_last_not_of(' ') + 1);
    text += line;
    text += '\n';
  }
  return text;
}

const Cell* ScreenImpl::CellAt(int x, int y) const {
  if (!last_texture_ || x < 0 || y < 0 || x >= last_texture_->width() ||
      y >= last_texture_->height()) {
    return nullptr;
  }
  return &(*last_texture_)[x, y];
}

void ScreenImpl::SettlePendingInput() {
  parser_->Timeout(static_cast<int>(kEscapeDelayMs));
  suppress_draw_ = true;
  while (auto event = parser_->GetEvent()) {
    HandleEvent(*event);
  }
  suppress_draw_ = false;
  if (draw_pending_) {
    Draw();
  }
}

void ScreenImpl::Settle(HeadlessTerminalDevice& device) {
  ScopedLayoutArenas arenas(layout_arenas_);
  while (device.HasInput()) {
    Step();
  }
  // Nothing more comes: a lone Escape is the key.
  if (parser_->HasPendingInput()) {
    SettlePendingInput();
  }
  // An animation that never ends is not waited for: the frame is as settled
  // as it gets. Bounded too, so a long one cannot hang a test.
  const double deadline = time::GetTimeMs() + 2000;
  bool ticked = false;
  while (HasActiveTransitions(/*include_infinite=*/false) &&
         time::GetTimeMs() < deadline) {
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
    ticked |= TickTransitions(time::GetTimeMs());
  }
  // An animation can also end while styles are resolved, without a tick.
  DispatchAnimationEnds();
  // Drawn even when nothing else changed: the last frame drawn may be one
  // from the middle of an animation.
  if (component_->Digest() || ticked) {
    RequestDraw();
  }
}

void ScreenImpl::RunHeadless() {
  auto& device = static_cast<HeadlessTerminalDevice&>(*device_);
#if defined(_WIN32)
  const bool piped = !_isatty(_fileno(stdin));
#elif defined(__EMSCRIPTEN__)
  const bool piped = false;  // A browser has no stdin to pipe.
#else
  const bool piped = !isatty(STDIN_FILENO);
#endif
  if (piped) {
    std::string input{std::istreambuf_iterator<char>(std::cin),
                      std::istreambuf_iterator<char>()};
    device.PushInput(input);
  }
  Settle(device);
  // What a terminal would show: the lines printed, then the last frame.
  std::cout << std::exchange(printed_, "") << Text() << std::flush;
}

HeadlessScreen::HeadlessScreen(Ref<ComponentBase> app, int width, int height)
    : device_(std::make_shared<HeadlessTerminalDevice>(width, height)),
      screen_(std::make_unique<Screen>(std::move(app), device_)) {
  screen_->impl_->Settle(*device_);
}

HeadlessScreen::~HeadlessScreen() = default;

void HeadlessScreen::Input(std::string_view bytes) {
  device_->PushInput(bytes);
  screen_->impl_->Settle(*device_);
}

void HeadlessScreen::Click(int x, int y) {
  const std::string column = std::to_string(x + 1);
  const std::string row = std::to_string(y + 1);
  Input("\x1b[<0;" + column + ";" + row + "M" + "\x1b[<0;" + column + ";" +
        row + "m");
}

void HeadlessScreen::Resize(int width, int height) {
  device_->Resize(width, height);
  screen_->impl_->UpdateSize();
  screen_->impl_->Settle(*device_);
}

std::string HeadlessScreen::Text() const {
  return screen_->Text();
}

namespace {
// TimelineScreen's clock. Not zero: some code reads a time of 0 as "never".
constexpr double kTimelineStart = 1000.0;
double g_timeline_now = kTimelineStart;
double TimelineNow() {
  return g_timeline_now;
}
}  // namespace

TimelineScreen::TimelineScreen(Ref<ComponentBase> app, int width, int height)
    : device_(std::make_shared<HeadlessTerminalDevice>(width, height)) {
  g_timeline_now = kTimelineStart;
  time::SetCustomClock(&TimelineNow);
  screen_ = std::make_unique<Screen>(std::move(app), device_);
  screen_->Draw();
}

TimelineScreen::~TimelineScreen() {
  screen_.reset();
  time::SetCustomClock(nullptr);
}

std::string TimelineScreen::Advance(double milliseconds) {
  g_timeline_now += milliseconds;
  screen_->impl_->Step();
  return Text();
}

std::vector<std::string> TimelineScreen::Record(double milliseconds,
                                                double frame_milliseconds) {
  std::vector<std::string> frames = {Text()};
  for (double elapsed = 0; elapsed < milliseconds;
       elapsed += frame_milliseconds) {
    std::string frame = Advance(frame_milliseconds);
    if (frame != frames.back()) {
      frames.push_back(std::move(frame));
    }
  }
  return frames;
}

void TimelineScreen::Input(std::string_view bytes) {
  device_->PushInput(bytes);
  Advance(0);
}

void TimelineScreen::Click(int x, int y) {
  const std::string column = std::to_string(x + 1);
  const std::string row = std::to_string(y + 1);
  Input("\x1b[<0;" + column + ";" + row + "M" + "\x1b[<0;" + column + ";" +
        row + "m");
}

void TimelineScreen::Resize(int width, int height) {
  device_->Resize(width, height);
  screen_->impl_->UpdateSize();
  Advance(0);
}

std::string TimelineScreen::Text() const {
  return screen_->Text();
}

Color TimelineScreen::BackgroundAt(int x, int y) const {
  const Cell* cell = screen_->impl_->CellAt(x, y);
  return cell ? cell->background_color : Color();
}

Color TimelineScreen::ForegroundAt(int x, int y) const {
  const Cell* cell = screen_->impl_->CellAt(x, y);
  return cell ? cell->foreground_color : Color();
}

double TimelineScreen::Elapsed() const {
  return g_timeline_now - kTimelineStart;
}

std::string RenderToString(Ref<ComponentBase> app, int width, int height) {
  return HeadlessScreen(std::move(app), width, height).Text();
}

}  // namespace rtxui
