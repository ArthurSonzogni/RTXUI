#include "rtxui/layout/layout.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <memory>
#include <numeric>
#include <sstream>
#include <vector>

#include "rtxui/core/string.hpp"
#include "rtxui/layout/layout_arena.hpp"
#include "rtxui/layout/layout_box.hpp"
#include "rtxui/layout/physical_fragment.hpp"
#include "rtxui/layout/style.hpp"

namespace rtxui {

thread_local LayoutArena g_layout_arena;

void ResetLayoutArena() {
  g_layout_arena.Reset();
}

// Helper: allocate a PhysicalFragment in the arena using std::allocate_shared
// so that both the control block and the object reside in the arena.
template <typename... Args>
std::shared_ptr<PhysicalFragment> MakeArenaFragment(Args&&... args) {
  return std::allocate_shared<PhysicalFragment,
                              LayoutArenaAllocator<PhysicalFragment>>(
      LayoutArenaAllocator<PhysicalFragment>(), std::forward<Args>(args)...);
}

std::string_view TruncateWithEllipsis(std::string_view text, int limit) {
  if (limit <= 3) {
    if (limit <= 0) {
      return "";
    }
    if (limit == 1) {
      return ".";
    }
    if (limit == 2) {
      return "..";
    }
    return "...";
  }

  int target_width = limit - 3;
  int current_width = 0;
  size_t truncate_byte_index = 0;

  for (const Grapheme& g : Graphemes(text)) {
    if (current_width + g.width > target_width) {
      break;
    }
    current_width += g.width;
    truncate_byte_index =
        static_cast<size_t>(g.text.data() + g.text.size() - text.data());
  }

  size_t result_size = truncate_byte_index + 3;
  void* ptr = g_layout_arena.Allocate(result_size, 1);
  char* dst = static_cast<char*>(ptr);
  std::memcpy(dst, text.data(), truncate_byte_index);
  std::memcpy(dst + truncate_byte_index, "...", 3);
  return std::string_view(dst, result_size);
}

// --- Forward Declarations ---
std::shared_ptr<PhysicalFragment> LayoutBlockFlow(LayoutInputNode node,
                                                  LayoutConstraints constraints,
                                                  LayoutContext context);
std::shared_ptr<PhysicalFragment> LayoutInlineFlow(
    LayoutInputNode node,
    LayoutConstraints constraints,
    LayoutContext context);
std::shared_ptr<PhysicalFragment> LayoutFlex(LayoutInputNode node,
                                             LayoutConstraints constraints,
                                             LayoutContext context);
std::shared_ptr<PhysicalFragment> LayoutTable(LayoutInputNode node,
                                              LayoutConstraints constraints,
                                              LayoutContext context);

/**
 * Resolves a StyleLength against a parent dimension.
 * Supports Fixed and Percentage units. Returns -1 for 'Auto'.
 */
int ResolveSize(const Length& length, int parent_size) {
  if (length.unit == Unit::Percent && parent_size >= 0) {
    return (length.value * parent_size) / 100;
  }
  if (length.unit == Unit::Cells) {
    return length.value;
  }
  return -1;  // Represents 'Auto'
}

void AdjustOutOfFlowCoordinates(PhysicalFragment* frag,
                                int shift_x,
                                int shift_y) {
  if (shift_x == 0 && shift_y == 0) {
    return;
  }
  for (auto& child : frag->children) {
    if (child.fragment) {
      if (child.fragment->dom_node &&
          (child.fragment->dom_node->style.position == PositionType::Absolute ||
           child.fragment->dom_node->style.position == PositionType::Fixed)) {
        child.x -= shift_x;
        child.y -= shift_y;
      } else {
        AdjustOutOfFlowCoordinates(child.fragment.get(), shift_x, shift_y);
      }
    }
  }
}

// --- Dispatcher ---
std::shared_ptr<PhysicalFragment> RunLayout(LayoutInputNode node,
                                            LayoutConstraints constraints,
                                            LayoutContext context) {
  auto* box = node.box;
  if (!box) {
    throw std::runtime_error("LayoutInputNode has null LayoutBox.");
  }

  if (box->algorithm == LayoutBox::Algorithm::Text) {
    auto wrapper_box =
        std::allocate_shared<LayoutBox, LayoutArenaAllocator<LayoutBox>>(
            LayoutArenaAllocator<LayoutBox>());
    wrapper_box->is_anonymous = true;
    wrapper_box->algorithm = LayoutBox::Algorithm::InlineFlow;
    wrapper_box->children.push_back(
        std::shared_ptr<LayoutBox>(box, [](LayoutBox*) {}));
    wrapper_box->style = box->style;
    wrapper_box->style.display_outside = DisplayOutside::Inline;
    return LayoutInlineFlow({wrapper_box.get()}, constraints, context);
  }

  if (context.viewport_w == 80 && context.viewport_h == 24) {
    context.viewport_w = constraints.width.value;
    context.viewport_h = constraints.height.value;
  }

  switch (node.box->algorithm) {
    case LayoutBox::Algorithm::BlockFlow:
      return LayoutBlockFlow(node, constraints, context);
    case LayoutBox::Algorithm::InlineFlow:
      return LayoutInlineFlow(node, constraints, context);
    case LayoutBox::Algorithm::Flex:
      return LayoutFlex(node, constraints, context);
    case LayoutBox::Algorithm::Table:
      return LayoutTable(node, constraints, context);
    case LayoutBox::Algorithm::Text:
      throw std::runtime_error(
          "Text nodes should not be laid out directly. Use InlineFlow.");
  }
  return nullptr;
}

LayoutContext CreateChildContext(LayoutBox* parent,
                                 int parent_w,
                                 int parent_h,
                                 int child_flow_x,
                                 int child_flow_y,
                                 const LayoutContext& parent_context) {
  LayoutContext child_context = parent_context;
  child_context.viewport_offset_x += child_flow_x;
  child_context.viewport_offset_y += child_flow_y;

  if (parent->style.position != PositionType::Static) {
    child_context.npa_w = parent_w;
    child_context.npa_h = parent_h;
    child_context.npa_offset_x = child_flow_x;
    child_context.npa_offset_y = child_flow_y;
  } else {
    child_context.npa_offset_x += child_flow_x;
    child_context.npa_offset_y += child_flow_y;
  }
  return child_context;
}

void LayoutOutOfFlowChildren(LayoutBox* parent,
                             std::shared_ptr<PhysicalFragment>& fragment,
                             const LayoutContext& parent_context) {
  for (auto& child_box : parent->children) {
    if (child_box->style.position == PositionType::Absolute ||
        child_box->style.position == PositionType::Fixed) {
      bool is_fixed = child_box->style.position == PositionType::Fixed;
      int container_w =
          is_fixed ? parent_context.viewport_w : parent_context.npa_w;
      int container_h =
          is_fixed ? parent_context.viewport_h : parent_context.npa_h;

      LayoutConstraints child_c;
      int child_w = ResolveSize(child_box->style.width, container_w);
      int child_h = ResolveSize(child_box->style.height, container_h);

      child_c.width = {
          child_w != -1 ? child_w : container_w,
          child_w != -1 ? MeasureMode::Exactly : MeasureMode::AtMost};
      child_c.height = {
          child_h != -1 ? child_h : container_h,
          child_h != -1 ? MeasureMode::Exactly : MeasureMode::AtMost};

      LayoutContext child_context = parent_context;
      if (child_box->style.position != PositionType::Static) {
        child_context.npa_w = child_w != -1 ? child_w : container_w;
        child_context.npa_h = child_h != -1 ? child_h : container_h;
        child_context.npa_offset_x = 0;
        child_context.npa_offset_y = 0;
      }

      LayoutInputNode child_input = {child_box.get()};
      std::shared_ptr<LayoutBox> wrapper_box = nullptr;
      if (child_box->algorithm == LayoutBox::Algorithm::Text) {
        wrapper_box =
            std::allocate_shared<LayoutBox, LayoutArenaAllocator<LayoutBox>>(
                LayoutArenaAllocator<LayoutBox>());
        wrapper_box->is_anonymous = true;
        wrapper_box->algorithm = LayoutBox::Algorithm::InlineFlow;
        wrapper_box->children.push_back(child_box);
        wrapper_box->style = child_box->style;
        wrapper_box->style.display_outside = DisplayOutside::Inline;
        child_input.box = wrapper_box.get();
      }
      auto child_frag = RunLayout(child_input, child_c, child_context);

      int x = 0;
      int y = 0;

      if (child_box->style.left.unit != Unit::Auto) {
        x = child_box->style.left.Resolve(container_w);
      } else if (child_box->style.right.unit != Unit::Auto) {
        x = container_w - child_box->style.right.Resolve(container_w) -
            child_frag->width;
      }

      if (child_box->style.top.unit != Unit::Auto) {
        y = child_box->style.top.Resolve(container_h);
      } else if (child_box->style.bottom.unit != Unit::Auto) {
        y = container_h - child_box->style.bottom.Resolve(container_h) -
            child_frag->height;
      }

      x += child_box->style.margin.left;
      y += child_box->style.margin.top;

      int relative_to_parent_x = 0;
      int relative_to_parent_y = 0;

      if (is_fixed) {
        relative_to_parent_x = x - parent_context.viewport_offset_x;
        relative_to_parent_y = y - parent_context.viewport_offset_y;
      } else {
        relative_to_parent_x = x - parent_context.npa_offset_x;
        relative_to_parent_y = y - parent_context.npa_offset_y;
      }

      fragment->children.push_back(
          {child_frag, relative_to_parent_x, relative_to_parent_y});
    }
  }
}

// --- Block Layout ---
std::shared_ptr<PhysicalFragment> LayoutBlockFlow(LayoutInputNode node,
                                                  LayoutConstraints constraints,
                                                  LayoutContext context) {
  auto* box = node.box;
  int avail_width = constraints.width.value;

  int width = 0;
  bool is_auto_width = false;

  if (constraints.width.mode == MeasureMode::Exactly) {
    width = avail_width;
  } else {
    int resolved = ResolveSize(box->style.width, avail_width);
    if (resolved != -1) {
      width = resolved;
    } else {
      is_auto_width = true;
      width = (constraints.width.mode == MeasureMode::Undefined)
                  ? 0
                  : std::max(0, avail_width - box->style.margin.Horiz());
    }
  }

  // Apply max-width constraint
  int max_width_resolved = ResolveSize(box->style.max_width, avail_width);
  if (max_width_resolved != -1 && width > max_width_resolved) {
    width = max_width_resolved;
    is_auto_width = false;
  }

  bool has_v_scrollbar = (box->style.overflow_y == Overflow::Scroll &&
                          box->style.scrollbar_width == ScrollbarWidth::Auto);
  bool has_h_scrollbar = (box->style.overflow_x == Overflow::Scroll &&
                          box->style.scrollbar_width == ScrollbarWidth::Auto);
  int scrollbar_spacing_x = has_v_scrollbar ? 1 : 0;

  int content_width_limit =
      is_auto_width && constraints.width.mode == MeasureMode::Undefined
          ? 10000
          : std::max(0, width - box->style.padding.Horiz() -
                            box->style.border.Horiz() - scrollbar_spacing_x);

  int child_width_limit = content_width_limit;
  if (box->style.overflow_x == Overflow::Scroll) {
    child_width_limit = 10000;
  }

  auto fragment = MakeArenaFragment(width, 0);
  fragment->children.reserve(box->children.size());
  fragment->dom_node = box->dom_node;
  fragment->visibility = box->style.visibility;
  fragment->background_color = box->style.background_color;
  fragment->foreground_color = box->style.foreground_color;
  fragment->opacity = box->style.opacity;
  fragment->bold = box->style.bold;
  fragment->underlined = box->style.underlined;
  fragment->underlined_double = box->style.underlined_double;
  fragment->strikethrough = box->style.strikethrough;
  fragment->blink = box->style.blink;
  fragment->border_style = box->style.border_style;
  fragment->border_color_top = box->style.border_color_top;
  fragment->border_color_right = box->style.border_color_right;
  fragment->border_color_bottom = box->style.border_color_bottom;
  fragment->border_color_left = box->style.border_color_left;
  if ((box->style.border.Horiz() > 0 || box->style.border.Vert() > 0) &&
      box->style.border_style != BorderStyle::None) {
    fragment->has_border = true;
  }

  int cur_y = box->style.border.top + box->style.padding.top;
  int cur_x = box->style.border.left + box->style.padding.left;
  int max_child_width = 0;
  int prev_margin_bottom = 0;
  bool is_first_child = true;

  int parent_resolved_height = -1;
  if (constraints.height.mode == MeasureMode::Exactly) {
    parent_resolved_height = constraints.height.value;
  } else {
    int resolved = ResolveSize(box->style.height, constraints.height.value);
    if (resolved != -1) {
      parent_resolved_height = resolved;
    }
  }

  int child_height_limit = 0;
  if (parent_resolved_height != -1) {
    child_height_limit =
        std::max(0, parent_resolved_height - box->style.padding.Vert() -
                        box->style.border.Vert());
  } else if (constraints.height.mode == MeasureMode::AtMost) {
    child_height_limit =
        std::max(0, constraints.height.value - box->style.padding.Vert() -
                        box->style.border.Vert());
  }

  for (auto& child_box : box->children) {
    if (child_box->style.position == PositionType::Absolute ||
        child_box->style.position == PositionType::Fixed) {
      continue;
    }

    LayoutConstraints child_c;
    child_c.width = {
        child_width_limit - child_box->style.margin.Horiz(),
        MeasureMode::AtMost,
    };
    child_c.height = {
        std::max(0, child_height_limit - child_box->style.margin.Vert()),
        MeasureMode::Undefined,
    };

    int margin_top = child_box->style.margin.top;
    int margin_bottom = child_box->style.margin.bottom;

    int collapsed_margin =
        is_first_child ? margin_top : std::max(prev_margin_bottom, margin_top);

    int cx = cur_x;
    int cy = cur_y + collapsed_margin;

    LayoutContext child_context =
        CreateChildContext(box, width, 0, cx, cy, context);
    LayoutInputNode child_input = {child_box.get()};
    std::shared_ptr<LayoutBox> wrapper_box = nullptr;
    if (child_box->algorithm == LayoutBox::Algorithm::Text) {
      wrapper_box =
          std::allocate_shared<LayoutBox, LayoutArenaAllocator<LayoutBox>>(
              LayoutArenaAllocator<LayoutBox>());
      wrapper_box->is_anonymous = true;
      wrapper_box->algorithm = LayoutBox::Algorithm::InlineFlow;
      wrapper_box->children.push_back(child_box);
      wrapper_box->style = child_box->style;
      wrapper_box->style.display_outside = DisplayOutside::Inline;
      child_input.box = wrapper_box.get();
    }
    auto child_frag = RunLayout(child_input, child_c, child_context);

    int rx = cx;
    int shift = 0;
    if (child_box->style.margin_left_auto &&
        child_box->style.margin_right_auto) {
      shift = std::max(0, child_width_limit - child_frag->width) / 2;
    } else if (child_box->style.margin_left_auto) {
      shift = std::max(0, child_width_limit - child_frag->width);
    } else {
      shift = child_box->style.margin.left;
    }
    rx += shift;
    int ry = cy;

    if (shift != child_box->style.margin.left) {
      AdjustOutOfFlowCoordinates(child_frag.get(),
                                 shift - child_box->style.margin.left, 0);
    }

    if (child_box->style.position == PositionType::Relative) {
      if (child_box->style.left.unit != Unit::Auto) {
        rx += child_box->style.left.Resolve(width);
      } else if (child_box->style.right.unit != Unit::Auto) {
        rx -= child_box->style.right.Resolve(width);
      }
      if (child_box->style.top.unit != Unit::Auto) {
        ry += child_box->style.top.Resolve(
            0);  // unresolved container height is 0
      } else if (child_box->style.bottom.unit != Unit::Auto) {
        ry -= child_box->style.bottom.Resolve(0);
      }
    }

    fragment->children.push_back({
        child_frag,
        rx,
        ry,
    });

    cur_y += collapsed_margin + child_frag->height;
    prev_margin_bottom = margin_bottom;
    is_first_child = false;

    max_child_width = std::max(
        max_child_width, child_frag->width + child_box->style.margin.Horiz());
  }

  cur_y += prev_margin_bottom;

  cur_y += box->style.border.bottom + box->style.padding.bottom;

  if (is_auto_width && constraints.width.mode == MeasureMode::Undefined) {
    fragment->width = max_child_width + box->style.padding.Horiz() +
                      box->style.border.Horiz();
  }

  // Cap width by max-width if needed
  {
    int max_w = ResolveSize(box->style.max_width, avail_width);
    if (max_w != -1 && fragment->width > max_w) {
      fragment->width = max_w;
    }
  }

  // Apply min-width constraint
  {
    int min_w = ResolveSize(box->style.min_width, avail_width);
    if (min_w != -1 && fragment->width < min_w) {
      fragment->width = min_w;
    }
  }

  if (constraints.height.mode == MeasureMode::Exactly) {
    fragment->height = constraints.height.value;
  } else {
    int resolved_h = ResolveSize(box->style.height, constraints.height.value);
    fragment->height = (resolved_h != -1) ? resolved_h : cur_y;
  }

  // Cap height by max-height if needed
  {
    int max_h = ResolveSize(box->style.max_height, constraints.height.value);
    if (max_h != -1 && fragment->height > max_h) {
      fragment->height = max_h;
    }
  }

  // Apply min-height constraint
  {
    int min_h = ResolveSize(box->style.min_height, constraints.height.value);
    if (min_h != -1 && fragment->height < min_h) {
      fragment->height = min_h;
    }
  }

  LayoutOutOfFlowChildren(box, fragment, context);

  if (box->style.overflow_y != Overflow::Visible ||
      box->style.overflow_x != Overflow::Visible) {
    fragment->clips_descendants = true;
  }

  if (box->dom_node) {
    int total_scroll_height = cur_y;
    int total_scroll_width = max_child_width + box->style.padding.Horiz() +
                             box->style.border.Horiz();

    for (const auto& child_link : fragment->children) {
      if (child_link.fragment && child_link.fragment->dom_node) {
        int child_bottom =
            child_link.y +
            (child_link.fragment->clips_descendants
                 ? child_link.fragment->height
                 : child_link.fragment->dom_node->scroll_height());
        int parent_bottom_needed =
            child_bottom + box->style.padding.bottom + box->style.border.bottom;
        total_scroll_height =
            std::max(total_scroll_height, parent_bottom_needed);

        int child_right = child_link.x +
                          (child_link.fragment->clips_descendants
                               ? child_link.fragment->width
                               : child_link.fragment->dom_node->scroll_width());
        int parent_right_needed =
            child_right + box->style.padding.right + box->style.border.right;
        total_scroll_width = std::max(total_scroll_width, parent_right_needed);
      }
    }

    if (!context.is_measurement) {
      box->dom_node->set_layout_width(fragment->width);
      box->dom_node->set_layout_height(fragment->height);
      box->dom_node->set_scroll_height(total_scroll_height);
      int max_scroll = std::max(0, total_scroll_height - fragment->height);
      box->dom_node->ClampScrollY(max_scroll);
      fragment->scroll_y = box->dom_node->scroll_y();
      fragment->visual_scroll_y = box->dom_node->visual_scroll_y();

      box->dom_node->set_scroll_width(total_scroll_width);
      int max_scroll_x = std::max(0, total_scroll_width - fragment->width);
      box->dom_node->ClampScrollX(max_scroll_x);
      fragment->scroll_x = box->dom_node->scroll_x();
      fragment->visual_scroll_x = box->dom_node->visual_scroll_x();
    } else {
      int max_scroll = std::max(0, total_scroll_height - fragment->height);
      fragment->scroll_y = std::clamp(box->dom_node->scroll_y(), 0, max_scroll);
      fragment->visual_scroll_y =
          std::clamp(box->dom_node->visual_scroll_y(), 0.f,
                     static_cast<float>(max_scroll));

      int max_scroll_x = std::max(0, total_scroll_width - fragment->width);
      fragment->scroll_x =
          std::clamp(box->dom_node->scroll_x(), 0, max_scroll_x);
      fragment->visual_scroll_x =
          std::clamp(box->dom_node->visual_scroll_x(), 0.f,
                     static_cast<float>(max_scroll_x));
    }
  }

  return fragment;
}

// --- Inline Layout ---
struct TextStyle {
  std::optional<Color> background_color;
  std::optional<Color> foreground_color;
  std::optional<bool> bold;
  std::optional<bool> underlined;
  std::optional<bool> underlined_double;
  std::optional<bool> strikethrough;
  std::optional<bool> blink;
};

std::shared_ptr<PhysicalFragment> LayoutInlineFlow(
    LayoutInputNode node,
    LayoutConstraints constraints,
    LayoutContext context) {
  auto* box = node.box;
  int avail_width = (constraints.width.mode == MeasureMode::Undefined)
                        ? 10000
                        : constraints.width.value;

  int width = (constraints.width.mode == MeasureMode::Exactly)
                  ? avail_width
                  : ResolveSize(box->style.width, avail_width);
  bool is_fixed_width =
      (width != -1) || (box->is_anonymous && avail_width < 10000);
  if (width == -1) {
    width = avail_width;
  }

  int content_width_limit = std::max(
      0, width - box->style.padding.Horiz() - box->style.border.Horiz());
  auto container_frag = MakeArenaFragment(width, 0);
  // Optimization: Estimate and pre-reserve the children vector capacity of the
  // physical fragment. Combined with LayoutArenaAllocator, this completely
  // eliminates heap allocations/copies during layout. Yields ~7% speedup in
  // Layout/Paint.
  size_t estimated_children = 0;
  for (const auto& child : box->children) {
    if (child->is_text) {
      estimated_children += std::max<size_t>(1, child->text_data.size() / 6);
    } else {
      estimated_children += 1;
    }
  }
  container_frag->children.reserve(estimated_children + 8);
  container_frag->dom_node = box->dom_node;
  container_frag->visibility = box->style.visibility;
  container_frag->background_color = box->style.background_color;
  container_frag->foreground_color = box->style.foreground_color;
  container_frag->opacity = box->style.opacity;
  container_frag->bold = box->style.bold;
  container_frag->underlined = box->style.underlined;
  container_frag->underlined_double = box->style.underlined_double;
  container_frag->strikethrough = box->style.strikethrough;
  container_frag->blink = box->style.blink;
  container_frag->border_style = box->style.border_style;
  container_frag->border_color_top = box->style.border_color_top;
  container_frag->border_color_right = box->style.border_color_right;
  container_frag->border_color_bottom = box->style.border_color_bottom;
  container_frag->border_color_left = box->style.border_color_left;
  if ((box->style.border.Horiz() > 0 || box->style.border.Vert() > 0) &&
      box->style.border_style != BorderStyle::None) {
    container_frag->has_border = true;
  }

  int cursor_x = 0;
  int cursor_y = box->style.padding.top + box->style.border.top;
  int line_height = 1;
  int max_line_width = 0;

  struct LineInfo {
    size_t start_index;
    size_t end_index;
    int occupied_width;
  };
  std::vector<LineInfo> lines;
  size_t line_start_index = 0;

  auto commit_line = [&]() {
    max_line_width = std::max(max_line_width, cursor_x);
    lines.push_back(
        {line_start_index, container_frag->children.size(), cursor_x});
    cursor_x = 0;
    cursor_y += line_height;
    line_height = 1;
    line_start_index = container_frag->children.size();
  };

  auto process_text_in_flow = [&](std::string_view text, Element* dom_node,
                                  const TextStyle& style) {
    if (box->style.text_overflow == TextOverflow::Ellipsis &&
        box->style.white_space == WhiteSpace::Nowrap) {
      int avail = content_width_limit - cursor_x;
      int text_w = 0;
      for (const Grapheme& g : Graphemes(text)) {
        text_w += g.width;
      }
      if (cursor_x + text_w > content_width_limit) {
        text = TruncateWithEllipsis(text, avail);
      }
    }
    size_t byte_start = 0;
    int col_start = 0;
    size_t last_space_byte = 0;
    int last_space_col = 0;
    bool have_last_space = false;

    auto emit_frag = [&](size_t byte_end, int col_width) {
      if (byte_end == byte_start && col_width == 0 && byte_end != text.size()) {
        return;
      }
      auto text_frag = MakeArenaFragment(col_width, 1);
      text_frag->dom_node = dom_node;
      text_frag->is_text = true;
      text_frag->text_content = text.substr(byte_start, byte_end - byte_start);
      auto* dst = reinterpret_cast<TextStyle*>(&text_frag->background_color);
      *dst = style;
      container_frag->children.push_back(
          {text_frag,
           box->style.padding.left + box->style.border.left + cursor_x,
           cursor_y});
      cursor_x += col_width;
    };

    int cur_col = col_start;

    // Optimization: Fast path for pure ASCII strings in text flow to bypass
    // the GraphemeIterator object creation and grapheme boundary checks.
    // Yields ~6% speedup in Layout/Paint.
    bool is_pure_ascii = true;
    for (char c : text) {
      if (static_cast<unsigned char>(c) >= 128) {
        is_pure_ascii = false;
        break;
      }
    }

    if (is_pure_ascii) {
      size_t i = 0;
      while (i < text.size()) {
        char c = text[i];
        bool is_newline = (c == '\n' || c == '\r');
        size_t byte_end = i + 1;
        int g_width = (c >= 32 && c < 127) ? 1 : 0;

        if (is_newline) {
          if (c == '\r' && i + 1 < text.size() && text[i + 1] == '\n') {
            byte_end = i + 2;
          }
          size_t frag_byte_end = i;
          emit_frag(frag_byte_end, cur_col - col_start);
          byte_start = byte_end;
          col_start = cur_col + g_width;
          cur_col = col_start;
          have_last_space = false;
          commit_line();
          i = byte_end;
          continue;
        }

        if (c == ' ') {
          last_space_byte = i;
          last_space_col = cur_col - col_start;
          have_last_space = true;
        }

        if (box->style.white_space == WhiteSpace::Nowrap) {
          cur_col += g_width;
        } else if (cursor_x + (cur_col - col_start) + g_width >
                   content_width_limit) {
          if (have_last_space) {
            emit_frag(last_space_byte, last_space_col);
            byte_start = last_space_byte + 1;
            col_start = last_space_col + 1;
            cur_col = col_start;
            have_last_space = false;
            commit_line();
            byte_end = i + 1;
            cur_col += g_width;
          } else if (cursor_x > 0) {
            commit_line();
            cur_col += g_width;
          } else {
            cur_col += g_width;
            size_t next_byte = i + 1;
            emit_frag(next_byte, cur_col - col_start);
            byte_start = next_byte;
            col_start = cur_col;
            commit_line();
          }
        } else {
          cur_col += g_width;
        }
        i = byte_end;
      }

      if (byte_start < text.size()) {
        emit_frag(text.size(), cur_col - col_start);
      }
      return;
    }

    for (const Grapheme& g : Graphemes(text)) {
      size_t byte_end =
          static_cast<size_t>(g.text.data() + g.text.size() - text.data());

      bool is_newline = (g.text == "\n" || g.text == "\r\n" || g.text == "\r");
      if (is_newline) {
        size_t frag_byte_end = static_cast<size_t>(g.text.data() - text.data());
        emit_frag(frag_byte_end, cur_col - col_start);
        byte_start = byte_end;
        col_start = cur_col + g.width;
        cur_col = col_start;
        have_last_space = false;
        commit_line();
        continue;
      }

      if (g.text.size() == 1 && g.text[0] == ' ') {
        last_space_byte = static_cast<size_t>(g.text.data() - text.data());
        last_space_col = cur_col - col_start;
        have_last_space = true;
      }

      if (box->style.white_space == WhiteSpace::Nowrap) {
        cur_col += g.width;
      } else if (cursor_x + (cur_col - col_start) + g.width >
                 content_width_limit) {
        if (have_last_space) {
          emit_frag(last_space_byte, last_space_col);
          byte_start = last_space_byte + 1;
          col_start = last_space_col + 1;
          cur_col = col_start;
          have_last_space = false;
          commit_line();
          byte_end =
              static_cast<size_t>(g.text.data() + g.text.size() - text.data());
          cur_col += g.width;
        } else if (cursor_x > 0) {
          commit_line();
          cur_col += g.width;
        } else {
          cur_col += g.width;
          size_t next_byte =
              static_cast<size_t>(g.text.data() + g.text.size() - text.data());
          emit_frag(next_byte, cur_col - col_start);
          byte_start = next_byte;
          col_start = cur_col;
          commit_line();
        }
      } else {
        cur_col += g.width;
      }
    }

    if (byte_start < text.size()) {
      emit_frag(text.size(), cur_col - col_start);
    }
  };

  auto place_opaque_box = [&](LayoutBox* elem) {
    if (elem->style.position == PositionType::Absolute ||
        elem->style.position == PositionType::Fixed) {
      return;
    }
    int m_left = elem->style.margin.left;
    int m_right = elem->style.margin.right;
    int m_top = elem->style.margin.top;
    int m_bottom = elem->style.margin.bottom;
    int child_m_horiz = m_left + m_right;
    int child_m_vert = m_top + m_bottom;

    int child_height_limit = 0;
    if (constraints.height.mode == MeasureMode::Exactly ||
        constraints.height.mode == MeasureMode::AtMost) {
      child_height_limit =
          std::max(0, constraints.height.value - box->style.padding.Vert() -
                          box->style.border.Vert());
    }

    LayoutConstraints child_c = {
        {content_width_limit - child_m_horiz, MeasureMode::AtMost},
        {std::max(0, child_height_limit - child_m_vert),
         MeasureMode::Undefined}};

    int cx =
        box->style.padding.left + box->style.border.left + cursor_x + m_left;
    int cy = cursor_y + m_top;

    LayoutContext child_context =
        CreateChildContext(box, width, 0, cx, cy, context);
    LayoutInputNode child_input = {elem};
    std::shared_ptr<LayoutBox> wrapper_box = nullptr;
    if (elem->algorithm == LayoutBox::Algorithm::Text) {
      wrapper_box =
          std::allocate_shared<LayoutBox, LayoutArenaAllocator<LayoutBox>>(
              LayoutArenaAllocator<LayoutBox>());
      wrapper_box->is_anonymous = true;
      wrapper_box->algorithm = LayoutBox::Algorithm::InlineFlow;
      wrapper_box->children.push_back(
          std::shared_ptr<LayoutBox>(elem, [](LayoutBox*) {}));
      wrapper_box->style = elem->style;
      wrapper_box->style.display_outside = DisplayOutside::Inline;
      child_input.box = wrapper_box.get();
    }
    auto child_frag = RunLayout(child_input, child_c, child_context);

    if (box->style.white_space != WhiteSpace::Nowrap &&
        cursor_x + child_frag->width + child_m_horiz > content_width_limit &&
        cursor_x > 0) {
      commit_line();
      cx = box->style.padding.left + box->style.border.left + cursor_x + m_left;
      cy = cursor_y + m_top;
      child_context = CreateChildContext(box, width, 0, cx, cy, context);
    }

    int rx = cx;
    int ry = cy;

    if (elem->style.position == PositionType::Relative) {
      if (elem->style.left.unit != Unit::Auto) {
        rx += elem->style.left.Resolve(width);
      } else if (elem->style.right.unit != Unit::Auto) {
        rx -= elem->style.right.Resolve(width);
      }
      if (elem->style.top.unit != Unit::Auto) {
        ry += elem->style.top.Resolve(0);
      } else if (elem->style.bottom.unit != Unit::Auto) {
        ry -= elem->style.bottom.Resolve(0);
      }
    }

    container_frag->children.push_back({child_frag, rx, ry});
    line_height = std::max(line_height, child_frag->height + child_m_vert);
    cursor_x += child_frag->width + child_m_horiz;
  };

  for (auto& child : box->children) {
    if (child->style.position == PositionType::Absolute ||
        child->style.position == PositionType::Fixed) {
      continue;
    }
    if (child->is_text) {
      process_text_in_flow(
          child->text_data, child->dom_node,
          {child->style.background_color, child->style.foreground_color,
           child->style.bold, child->style.underlined,
           child->style.underlined_double, child->style.strikethrough,
           child->style.blink});
    } else if (child->style.display_outside == DisplayOutside::Inline &&
               child->style.display_inside == DisplayInside::Flow &&
               child->algorithm != LayoutBox::Algorithm::Table &&
               child->style.border.Horiz() == 0 &&
               child->style.border.Vert() == 0 &&
               child->style.margin.Horiz() == 0 &&
               child->style.margin.Vert() == 0 &&
               child->style.padding.Horiz() == 0 &&
               child->style.padding.Vert() == 0) {
      for (auto& grandchild : child->children) {
        if (grandchild->is_text) {
          process_text_in_flow(
              grandchild->text_data, child->dom_node,
              {child->style.background_color, child->style.foreground_color,
               child->style.bold, child->style.underlined,
               child->style.underlined_double, child->style.strikethrough,
               child->style.blink});
        } else {
          place_opaque_box(grandchild.get());
        }
      }
    } else {
      place_opaque_box(child.get());
    }
  }

  commit_line();
  container_frag->height =
      cursor_y + box->style.padding.bottom + box->style.border.bottom;
  if (!is_fixed_width) {
    container_frag->width =
        max_line_width + box->style.padding.Horiz() + box->style.border.Horiz();
  }

  // Cap width by max-width if needed
  {
    int max_w = ResolveSize(box->style.max_width, avail_width);
    if (max_w != -1 && container_frag->width > max_w) {
      container_frag->width = max_w;
    }
  }

  // Cap height by max-height if needed
  {
    int max_h = ResolveSize(box->style.max_height, constraints.height.value);
    if (max_h != -1 && container_frag->height > max_h) {
      container_frag->height = max_h;
    }
  }

  LayoutOutOfFlowChildren(box, container_frag, context);

  if (box->dom_node && !context.is_measurement) {
    box->dom_node->set_layout_width(container_frag->width);
    box->dom_node->set_layout_height(container_frag->height);
  }

  int final_content_width =
      is_fixed_width ? content_width_limit : max_line_width;

  if (box->style.text_align.has_value() &&
      box->style.text_align != TextAlign::Left && final_content_width > 0) {
    for (const auto& line : lines) {
      int remaining_space = final_content_width - line.occupied_width;
      if (remaining_space > 0) {
        int shift = 0;
        if (box->style.text_align == TextAlign::Right) {
          shift = remaining_space;
        } else if (box->style.text_align == TextAlign::Center) {
          shift = remaining_space / 2;
        }
        if (shift > 0) {
          for (size_t i = line.start_index; i < line.end_index; ++i) {
            container_frag->children[i].x += shift;
          }
        }
      }
    }
  }

  return container_frag;
}

/**
 * LayoutFlex: A W3C-aligned Flexbox implementation for TUI.
 * Handles flex-direction, weighted flex-shrink, flex-grow, and basis.
 */
std::shared_ptr<PhysicalFragment> LayoutFlex(LayoutInputNode node,
                                             LayoutConstraints constraints,
                                             LayoutContext context) {
  auto* box = node.box;
  bool is_row = box->style.flex_direction == Direction::Row ||
                box->style.flex_direction == Direction::RowReverse;
  bool is_reverse = box->style.flex_direction == Direction::RowReverse ||
                    box->style.flex_direction == Direction::ColumnReverse;

  int parent_w = constraints.width.value;
  int parent_h = constraints.height.value;

  int my_width = (constraints.width.mode == MeasureMode::Exactly)
                     ? parent_w
                     : ResolveSize(box->style.width, parent_w);
  int my_height = (constraints.height.mode == MeasureMode::Exactly)
                      ? parent_h
                      : ResolveSize(box->style.height, parent_h);

  // Apply max-width constraint
  int max_width_resolved = ResolveSize(box->style.max_width, parent_w);
  if (max_width_resolved != -1 && my_width > max_width_resolved) {
    my_width = max_width_resolved;
  }

  // Apply min-width constraint
  int min_width_resolved = ResolveSize(box->style.min_width, parent_w);
  if (min_width_resolved != -1 && my_width < min_width_resolved) {
    my_width = min_width_resolved;
  }

  // Apply max-height constraint
  int max_height_resolved = ResolveSize(box->style.max_height, parent_h);
  if (max_height_resolved != -1 && my_height > max_height_resolved) {
    my_height = max_height_resolved;
  }

  // Apply min-height constraint
  int min_height_resolved = ResolveSize(box->style.min_height, parent_h);
  if (min_height_resolved != -1 && my_height < min_height_resolved) {
    my_height = min_height_resolved;
  }

  bool auto_width = (my_width == -1);
  bool auto_height = (my_height == -1);

  if (auto_width) {
    my_width = parent_w;
  }
  if (auto_height) {
    my_height = parent_h;
  }

  bool has_v_scrollbar = (box->style.overflow_y == Overflow::Scroll &&
                          box->style.scrollbar_width == ScrollbarWidth::Auto);
  bool has_h_scrollbar = (box->style.overflow_x == Overflow::Scroll &&
                          box->style.scrollbar_width == ScrollbarWidth::Auto);
  int scrollbar_spacing_x = has_v_scrollbar ? 1 : 0;
  int scrollbar_spacing_y = has_h_scrollbar ? 1 : 0;

  int content_w =
      std::max(0, my_width - box->style.border.Horiz() -
                      box->style.padding.Horiz() - scrollbar_spacing_x);
  int content_h =
      std::max(0, my_height - box->style.border.Vert() -
                      box->style.padding.Vert() - scrollbar_spacing_y);

  struct FlexItem {
    std::shared_ptr<LayoutBox> child_ptr;
    std::shared_ptr<PhysicalFragment> fragment;
    int main_base_size;
    int main_resolved_size;
    float grow;
    float shrink;
    int cross_size;
  };

  struct FlexLine {
    std::vector<FlexItem> items;
    int total_main_base = 0;
    int total_main_resolved = 0;
    float total_grow = 0.0f;
    float total_shrink_scaled = 0.0f;
    int cross_size = 0;
  };

  std::vector<FlexItem> items;

  // Pass 1: Determine Flex Base Sizes
  int resolved_gap =
      ResolveSize(is_row ? box->style.column_gap : box->style.row_gap,
                  is_row ? content_w : content_h);
  for (auto& child : box->children) {
    if (child->style.position == PositionType::Absolute ||
        child->style.position == PositionType::Fixed) {
      continue;
    }
    int basis = is_row ? ResolveSize(child->style.width, content_w)
                       : ResolveSize(child->style.height, content_h);

    LayoutConstraints child_c;
    if (is_row) {
      child_c.width = {basis != -1 ? basis : 0, basis != -1
                                                    ? MeasureMode::Exactly
                                                    : MeasureMode::Undefined};
      if (auto_height && constraints.height.mode == MeasureMode::Undefined) {
        child_c.height = {content_h, MeasureMode::Undefined};
      } else {
        int max_h =
            (box->style.overflow_y == Overflow::Scroll) ? 10000 : content_h;
        child_c.height = {max_h, MeasureMode::AtMost};
      }
    } else {
      if (auto_width && constraints.width.mode == MeasureMode::Undefined) {
        child_c.width = {content_w, MeasureMode::Undefined};
      } else {
        int max_w =
            (box->style.overflow_x == Overflow::Scroll) ? 10000 : content_w;
        child_c.width = {max_w, MeasureMode::AtMost};
      }
      child_c.height = {basis != -1 ? basis : 0, basis != -1
                                                     ? MeasureMode::Exactly
                                                     : MeasureMode::Undefined};
    }

    LayoutInputNode child_input = {child.get()};
    std::shared_ptr<LayoutBox> wrapper_box = nullptr;
    if (child->algorithm == LayoutBox::Algorithm::Text) {
      wrapper_box =
          std::allocate_shared<LayoutBox, LayoutArenaAllocator<LayoutBox>>(
              LayoutArenaAllocator<LayoutBox>());
      wrapper_box->is_anonymous = true;
      wrapper_box->algorithm = LayoutBox::Algorithm::InlineFlow;
      wrapper_box->children.push_back(child);
      wrapper_box->style = child->style;
      wrapper_box->style.display_outside = DisplayOutside::Inline;
      child_input.box = wrapper_box.get();
    }

    LayoutContext child_context = context;
    child_context.is_measurement = true;
    auto frag = RunLayout(child_input, child_c, child_context);
    int m_margin =
        is_row ? child->style.margin.Horiz() : child->style.margin.Vert();
    int main_size = (is_row ? frag->width : frag->height) + m_margin;
    int cross_size = is_row ? (frag->height + child->style.margin.Vert())
                            : (frag->width + child->style.margin.Horiz());

    items.push_back({child, frag, main_size, main_size, child->style.flex_grow,
                     child->style.flex_shrink, cross_size});
  }

  // Pass 2: Group items into Flex Lines and resolve flexible lengths
  std::vector<FlexLine> lines;
  bool main_is_indefinite =
      is_row
          ? (constraints.width.mode == MeasureMode::Undefined && auto_width)
          : (constraints.height.mode == MeasureMode::Undefined && auto_height);
  bool is_wrap = box->style.flex_wrap == FlexWrap::Wrap ||
                 box->style.flex_wrap == FlexWrap::WrapReverse;
  int container_main = is_row ? content_w : content_h;

  if (!is_wrap || main_is_indefinite || items.empty()) {
    FlexLine line;
    line.items = std::move(items);
    lines.push_back(std::move(line));
  } else {
    FlexLine current_line;
    for (auto& item : items) {
      int item_needed = item.main_base_size;
      if (!current_line.items.empty()) {
        item_needed += resolved_gap;
      }
      if (!current_line.items.empty() &&
          current_line.total_main_base + item_needed > container_main) {
        lines.push_back(std::move(current_line));
        current_line = FlexLine();
      }
      if (current_line.items.empty()) {
        current_line.total_main_base = item.main_base_size;
      } else {
        current_line.total_main_base += resolved_gap + item.main_base_size;
      }
      current_line.items.push_back(std::move(item));
    }
    if (!current_line.items.empty()) {
      lines.push_back(std::move(current_line));
    }
  }

  // Resolve flexible lengths for each line independently
  for (auto& line : lines) {
    line.total_main_base = 0;
    line.total_grow = 0.0f;
    line.total_shrink_scaled = 0.0f;
    bool is_first = true;
    for (const auto& item : line.items) {
      if (!is_first) {
        line.total_main_base += resolved_gap;
      }
      is_first = false;
      line.total_main_base += item.main_base_size;
      line.total_grow += item.grow;
      line.total_shrink_scaled += (item.main_base_size * item.shrink);
    }

    int free_space =
        main_is_indefinite ? 0 : (container_main - line.total_main_base);

    if (free_space > 0 && line.total_grow > 0) {
      int total_allocated = 0;
      float current_grow_sum = 0.0f;
      int items_to_grow = 0;
      for (const auto& item : line.items) {
        if (item.grow > 0) {
          items_to_grow++;
        }
      }
      int grown_count = 0;
      for (auto& item : line.items) {
        if (item.grow > 0) {
          grown_count++;
          int next_cumulative = 0;
          if (grown_count == items_to_grow) {
            next_cumulative = free_space;
          } else {
            current_grow_sum += item.grow;
            next_cumulative = static_cast<int>((free_space * current_grow_sum) /
                                               line.total_grow);
          }
          int extra = next_cumulative - total_allocated;
          total_allocated = next_cumulative;
          item.main_resolved_size += extra;
        }
      }
    } else if (free_space < 0 && line.total_shrink_scaled > 0) {
      bool allow_overflow =
          (is_row && box->style.overflow_x == Overflow::Scroll) ||
          (!is_row && box->style.overflow_y == Overflow::Scroll);
      if (!allow_overflow) {
        int total_shrunk = 0;
        float current_shrink_scaled_sum = 0.0f;
        int items_to_shrink = 0;
        for (const auto& item : line.items) {
          if (item.shrink > 0) {
            items_to_shrink++;
          }
        }
        int shrunk_count = 0;
        for (auto& item : line.items) {
          if (item.shrink > 0) {
            shrunk_count++;
            int next_cumulative = 0;
            if (shrunk_count == items_to_shrink) {
              next_cumulative = free_space;
            } else {
              current_shrink_scaled_sum += (item.main_base_size * item.shrink);
              next_cumulative =
                  static_cast<int>((free_space * current_shrink_scaled_sum) /
                                   line.total_shrink_scaled);
            }
            int shrink_amount = next_cumulative - total_shrunk;
            total_shrunk = next_cumulative;
            item.main_resolved_size += shrink_amount;
          }
        }
      }
    }

    // Measure each item using its main resolved size to get final cross size
    line.cross_size = 0;
    for (auto& item : line.items) {
      LayoutConstraints final_c;
      int m_horiz = item.child_ptr->style.margin.Horiz();
      int m_vert = item.child_ptr->style.margin.Vert();

      if (is_row) {
        final_c.width = {item.main_resolved_size - m_horiz,
                         MeasureMode::Exactly};
        if (auto_height && constraints.height.mode == MeasureMode::Undefined) {
          final_c.height = {content_h, MeasureMode::Undefined};
        } else {
          int max_h =
              (box->style.overflow_y == Overflow::Scroll) ? 10000 : content_h;
          final_c.height = {max_h, MeasureMode::AtMost};
        }
      } else {
        if (auto_width && constraints.width.mode == MeasureMode::Undefined) {
          final_c.width = {content_w, MeasureMode::Undefined};
        } else {
          int max_w =
              (box->style.overflow_x == Overflow::Scroll) ? 10000 : content_w;
          final_c.width = {max_w, MeasureMode::AtMost};
        }
        final_c.height = {item.main_resolved_size - m_vert,
                          MeasureMode::Exactly};
      }

      LayoutContext child_context = context;
      child_context.is_measurement = true;
      LayoutInputNode child_input = {item.child_ptr.get()};
      std::shared_ptr<LayoutBox> wrapper_box = nullptr;
      if (item.child_ptr->algorithm == LayoutBox::Algorithm::Text) {
        wrapper_box =
            std::allocate_shared<LayoutBox, LayoutArenaAllocator<LayoutBox>>(
                LayoutArenaAllocator<LayoutBox>());
        wrapper_box->is_anonymous = true;
        wrapper_box->algorithm = LayoutBox::Algorithm::InlineFlow;
        wrapper_box->children.push_back(item.child_ptr);
        wrapper_box->style = item.child_ptr->style;
        wrapper_box->style.display_outside = DisplayOutside::Inline;
        child_input.box = wrapper_box.get();
      }

      item.fragment = RunLayout(child_input, final_c, child_context);
      item.cross_size = is_row ? (item.fragment->height + m_vert)
                               : (item.fragment->width + m_horiz);
      line.cross_size = std::max(line.cross_size, item.cross_size);
    }

    // Recompute total main resolved for the line
    line.total_main_resolved = 0;
    is_first = true;
    for (const auto& item : line.items) {
      if (!is_first) {
        line.total_main_resolved += resolved_gap;
      }
      is_first = false;
      line.total_main_resolved += item.main_resolved_size;
    }
  }

  // Compute total cross lines and max line main resolved size
  int total_cross_lines = 0;
  int resolved_cross_gap =
      ResolveSize(is_row ? box->style.row_gap : box->style.column_gap,
                  is_row ? content_h : content_w);
  {
    bool is_first = true;
    for (const auto& line : lines) {
      if (!is_first) {
        total_cross_lines += resolved_cross_gap;
      }
      is_first = false;
      total_cross_lines += line.cross_size;
    }
  }

  int max_line_main_resolved = 0;
  for (const auto& line : lines) {
    max_line_main_resolved =
        std::max(max_line_main_resolved, line.total_main_resolved);
  }

  // Resolve final container width and height
  int resolved_width = my_width;
  int resolved_height = my_height;
  if (auto_width) {
    int needed_w = is_row ? max_line_main_resolved : total_cross_lines;
    resolved_width = needed_w + box->style.border.Horiz() +
                     box->style.padding.Horiz() + scrollbar_spacing_x;
  }
  if (auto_height) {
    int needed_h = is_row ? total_cross_lines : max_line_main_resolved;
    resolved_height = needed_h + box->style.border.Vert() +
                      box->style.padding.Vert() + scrollbar_spacing_y;
  }

  // Apply min/max constraints to the container size
  {
    int max_w = ResolveSize(box->style.max_width, parent_w);
    if (max_w != -1 && resolved_width > max_w) {
      resolved_width = max_w;
    }
    int min_w = ResolveSize(box->style.min_width, parent_w);
    if (min_w != -1 && resolved_width < min_w) {
      resolved_width = min_w;
    }
    int max_h = ResolveSize(box->style.max_height, parent_h);
    if (max_h != -1 && resolved_height > max_h) {
      resolved_height = max_h;
    }
    int min_h = ResolveSize(box->style.min_height, parent_h);
    if (min_h != -1 && resolved_height < min_h) {
      resolved_height = min_h;
    }
  }

  int final_content_w =
      std::max(0, resolved_width - box->style.border.Horiz() -
                      box->style.padding.Horiz() - scrollbar_spacing_x);
  int final_content_h =
      std::max(0, resolved_height - box->style.border.Vert() -
                      box->style.padding.Vert() - scrollbar_spacing_y);
  int container_main_final = is_row ? final_content_w : final_content_h;
  int container_cross_final = is_row ? final_content_h : final_content_w;

  bool cross_is_definite = is_row ? !auto_height : !auto_width;
  if (cross_is_definite && !lines.empty()) {
    int remaining_cross = container_cross_final - total_cross_lines;
    if (remaining_cross > 0) {
      int extra_per_line = remaining_cross / lines.size();
      int remainder = remaining_cross % lines.size();
      for (size_t l = 0; l < lines.size(); ++l) {
        lines[l].cross_size += extra_per_line + (l < remainder ? 1 : 0);
      }
    }
  }

  // Pass 3: Final Measurement & Positioning
  auto fragment = MakeArenaFragment(resolved_width, resolved_height);
  fragment->dom_node = box->dom_node;
  fragment->visibility = box->style.visibility;
  fragment->background_color = box->style.background_color;
  fragment->foreground_color = box->style.foreground_color;
  fragment->opacity = box->style.opacity;
  fragment->bold = box->style.bold;
  fragment->underlined = box->style.underlined;
  fragment->underlined_double = box->style.underlined_double;
  fragment->strikethrough = box->style.strikethrough;
  fragment->blink = box->style.blink;
  fragment->border_style = box->style.border_style;
  fragment->border_color_top = box->style.border_color_top;
  fragment->border_color_right = box->style.border_color_right;
  fragment->border_color_bottom = box->style.border_color_bottom;
  fragment->border_color_left = box->style.border_color_left;
  if ((box->style.border.Horiz() > 0 || box->style.border.Vert() > 0) &&
      box->style.border_style != BorderStyle::None) {
    fragment->has_border = true;
  }

  int main_start = is_row ? (box->style.padding.left + box->style.border.left)
                          : (box->style.padding.top + box->style.border.top);
  int cross_start = is_row ? (box->style.padding.top + box->style.border.top)
                           : (box->style.padding.left + box->style.border.left);

  // Determine line cross offsets
  std::vector<int> line_cross_offsets(lines.size(), 0);
  int cur_line_offset = 0;
  for (size_t l = 0; l < lines.size(); ++l) {
    line_cross_offsets[l] = cur_line_offset;
    cur_line_offset += lines[l].cross_size + resolved_cross_gap;
  }

  // Position items on each line
  for (size_t l = 0; l < lines.size(); ++l) {
    auto& line = lines[l];
    int line_offset = line_cross_offsets[l];

    int line_cross_pos =
        cross_start +
        (box->style.flex_wrap == FlexWrap::WrapReverse
             ? (container_cross_final - line_offset - line.cross_size)
             : line_offset);

    int justify_free_space =
        main_is_indefinite ? 0
                           : (container_main_final - line.total_main_resolved);

    std::vector<int> item_positions(line.items.size(), 0);
    int cur_pos = main_start;
    if (justify_free_space > 0) {
      if (box->style.justify_content == JustifyContent::FlexEnd) {
        cur_pos += justify_free_space;
        for (size_t i = 0; i < line.items.size(); ++i) {
          item_positions[i] = cur_pos;
          cur_pos += line.items[i].main_resolved_size + resolved_gap;
        }
      } else if (box->style.justify_content == JustifyContent::Center) {
        cur_pos += justify_free_space / 2;
        for (size_t i = 0; i < line.items.size(); ++i) {
          item_positions[i] = cur_pos;
          cur_pos += line.items[i].main_resolved_size + resolved_gap;
        }
      } else if (box->style.justify_content == JustifyContent::SpaceBetween) {
        if (line.items.size() > 1) {
          int extra_gap = justify_free_space / (line.items.size() - 1);
          int remainder = justify_free_space % (line.items.size() - 1);
          for (size_t i = 0; i < line.items.size(); ++i) {
            item_positions[i] = cur_pos;
            cur_pos += line.items[i].main_resolved_size + resolved_gap +
                       extra_gap + (i < remainder ? 1 : 0);
          }
        } else {
          item_positions[0] = cur_pos;
        }
      } else if (box->style.justify_content == JustifyContent::SpaceAround) {
        int spacing = justify_free_space / line.items.size();
        int remainder = justify_free_space % line.items.size();
        cur_pos += spacing / 2;
        for (size_t i = 0; i < line.items.size(); ++i) {
          item_positions[i] = cur_pos;
          cur_pos += line.items[i].main_resolved_size + resolved_gap + spacing +
                     (i < remainder ? 1 : 0);
        }
      } else if (box->style.justify_content == JustifyContent::SpaceEvenly) {
        int spacing = justify_free_space / (line.items.size() + 1);
        int remainder = justify_free_space % (line.items.size() + 1);
        cur_pos += spacing + (remainder > 0 ? 1 : 0);
        if (remainder > 0) {
          remainder--;
        }
        for (size_t i = 0; i < line.items.size(); ++i) {
          item_positions[i] = cur_pos;
          cur_pos += line.items[i].main_resolved_size + resolved_gap + spacing +
                     (i < remainder ? 1 : 0);
        }
      } else {
        // FlexStart
        for (size_t i = 0; i < line.items.size(); ++i) {
          item_positions[i] = cur_pos;
          cur_pos += line.items[i].main_resolved_size + resolved_gap;
        }
      }
    } else {
      for (size_t i = 0; i < line.items.size(); ++i) {
        item_positions[i] = cur_pos;
        cur_pos += line.items[i].main_resolved_size + resolved_gap;
      }
    }

    if (is_reverse) {
      int effective_container_main =
          main_is_indefinite ? line.total_main_resolved : container_main_final;
      int container_start =
          is_row ? (box->style.padding.left + box->style.border.left)
                 : (box->style.padding.top + box->style.border.top);
      int container_end = container_start + effective_container_main;
      for (size_t i = 0; i < line.items.size(); ++i) {
        int pos = item_positions[i];
        int size = line.items[i].main_resolved_size;
        item_positions[i] = container_end - (pos - container_start) - size;
      }
    }

    for (size_t i = 0; i < line.items.size(); ++i) {
      auto& item = line.items[i];
      LayoutConstraints final_c;
      int m_horiz = item.child_ptr->style.margin.Horiz();
      int m_vert = item.child_ptr->style.margin.Vert();

      if (is_row) {
        final_c.width = {item.main_resolved_size - m_horiz,
                         MeasureMode::Exactly};
        if (box->style.align_items == AlignItems::Stretch &&
            item.child_ptr->style.height.unit == Unit::Auto) {
          final_c.height = {line.cross_size - m_vert, MeasureMode::Exactly};
        } else if (auto_height &&
                   constraints.height.mode == MeasureMode::Undefined) {
          final_c.height = {item.cross_size - m_vert, MeasureMode::Exactly};
        } else {
          int max_h = (box->style.overflow_y == Overflow::Scroll)
                          ? 10000
                          : final_content_h;
          final_c.height = {max_h, MeasureMode::AtMost};
        }
      } else {
        if (box->style.align_items == AlignItems::Stretch &&
            item.child_ptr->style.width.unit == Unit::Auto) {
          final_c.width = {line.cross_size - m_horiz, MeasureMode::Exactly};
        } else if (auto_width &&
                   constraints.width.mode == MeasureMode::Undefined) {
          final_c.width = {item.cross_size - m_horiz, MeasureMode::Exactly};
        } else {
          int max_w = (box->style.overflow_x == Overflow::Scroll)
                          ? 10000
                          : final_content_w;
          final_c.width = {max_w, MeasureMode::AtMost};
        }
        final_c.height = {item.main_resolved_size - m_vert,
                          MeasureMode::Exactly};
      }

      int item_cross_pos = line_cross_pos;
      if (box->style.align_items != AlignItems::Stretch) {
        int cross_free_space = line.cross_size - item.cross_size;
        if (cross_free_space > 0) {
          if (box->style.align_items == AlignItems::FlexEnd) {
            item_cross_pos += cross_free_space;
          } else if (box->style.align_items == AlignItems::Center) {
            item_cross_pos += cross_free_space / 2;
          }
        }
      }

      int x = is_row ? item_positions[i] + item.child_ptr->style.margin.left
                     : item_cross_pos + item.child_ptr->style.margin.left;
      int y = is_row ? item_cross_pos + item.child_ptr->style.margin.top
                     : item_positions[i] + item.child_ptr->style.margin.top;

      LayoutContext child_context = CreateChildContext(
          box, resolved_width, resolved_height, x, y, context);
      LayoutInputNode child_input = {item.child_ptr.get()};
      std::shared_ptr<LayoutBox> wrapper_box = nullptr;
      if (item.child_ptr->algorithm == LayoutBox::Algorithm::Text) {
        wrapper_box =
            std::allocate_shared<LayoutBox, LayoutArenaAllocator<LayoutBox>>(
                LayoutArenaAllocator<LayoutBox>());
        wrapper_box->is_anonymous = true;
        wrapper_box->algorithm = LayoutBox::Algorithm::InlineFlow;
        wrapper_box->children.push_back(item.child_ptr);
        wrapper_box->style = item.child_ptr->style;
        wrapper_box->style.display_outside = DisplayOutside::Inline;
        child_input.box = wrapper_box.get();
      }

      item.fragment = RunLayout(child_input, final_c, child_context);

      int rx = x;
      int ry = y;

      if (item.child_ptr->style.position == PositionType::Relative) {
        if (item.child_ptr->style.left.unit != Unit::Auto) {
          rx += item.child_ptr->style.left.Resolve(resolved_width);
        } else if (item.child_ptr->style.right.unit != Unit::Auto) {
          rx -= item.child_ptr->style.right.Resolve(resolved_width);
        }
        if (item.child_ptr->style.top.unit != Unit::Auto) {
          ry += item.child_ptr->style.top.Resolve(resolved_height);
        } else if (item.child_ptr->style.bottom.unit != Unit::Auto) {
          ry -= item.child_ptr->style.bottom.Resolve(resolved_height);
        }
      }

      fragment->children.push_back({item.fragment, rx, ry});
    }
  }

  LayoutOutOfFlowChildren(box, fragment, context);

  int max_cross_used = total_cross_lines;
  int main_pos = max_line_main_resolved;

  int total_content_height =
      is_row ? (total_cross_lines + box->style.padding.Vert() +
                box->style.border.Vert())
             : (max_line_main_resolved + box->style.padding.Vert() +
                box->style.border.Vert());
  int total_content_width =
      is_row ? (max_line_main_resolved + box->style.padding.Horiz() +
                box->style.border.Horiz())
             : (total_cross_lines + box->style.padding.Horiz() +
                box->style.border.Horiz());

  if (box->style.overflow_y != Overflow::Visible ||
      box->style.overflow_x != Overflow::Visible) {
    fragment->clips_descendants = true;
  }

  if (box->dom_node) {
    for (const auto& child_link : fragment->children) {
      if (child_link.fragment && child_link.fragment->dom_node) {
        int child_bottom =
            child_link.y +
            (child_link.fragment->clips_descendants
                 ? child_link.fragment->height
                 : child_link.fragment->dom_node->scroll_height());
        int parent_bottom_needed =
            child_bottom + box->style.padding.bottom + box->style.border.bottom;
        total_content_height =
            std::max(total_content_height, parent_bottom_needed);

        int child_right = child_link.x +
                          (child_link.fragment->clips_descendants
                               ? child_link.fragment->width
                               : child_link.fragment->dom_node->scroll_width());
        int parent_right_needed =
            child_right + box->style.padding.right + box->style.border.right;
        total_content_width =
            std::max(total_content_width, parent_right_needed);
      }
    }

    if (!context.is_measurement) {
      box->dom_node->set_layout_width(fragment->width);
      box->dom_node->set_layout_height(fragment->height);
      box->dom_node->set_scroll_height(total_content_height);
      int max_scroll = std::max(0, total_content_height - fragment->height);
      box->dom_node->ClampScrollY(max_scroll);
      fragment->scroll_y = box->dom_node->scroll_y();
      fragment->visual_scroll_y = box->dom_node->visual_scroll_y();

      box->dom_node->set_scroll_width(total_content_width);
      int max_scroll_x = std::max(0, total_content_width - fragment->width);
      box->dom_node->ClampScrollX(max_scroll_x);
      fragment->scroll_x = box->dom_node->scroll_x();
      fragment->visual_scroll_x = box->dom_node->visual_scroll_x();
    } else {
      int max_scroll = std::max(0, total_content_height - fragment->height);
      fragment->scroll_y = std::clamp(box->dom_node->scroll_y(), 0, max_scroll);
      fragment->visual_scroll_y =
          std::clamp(box->dom_node->visual_scroll_y(), 0.f,
                     static_cast<float>(max_scroll));

      int max_scroll_x = std::max(0, total_content_width - fragment->width);
      fragment->scroll_x =
          std::clamp(box->dom_node->scroll_x(), 0, max_scroll_x);
      fragment->visual_scroll_x =
          std::clamp(box->dom_node->visual_scroll_x(), 0.f,
                     static_cast<float>(max_scroll_x));
    }
  }

  return fragment;
}

std::shared_ptr<PhysicalFragment> LayoutTable(LayoutInputNode node,
                                              LayoutConstraints constraints,
                                              LayoutContext context) {
  auto* box = node.box;
  int avail_width = constraints.width.value;

  int width = 0;
  bool is_auto_width = false;
  bool is_fixed_width = false;

  if (constraints.width.mode == MeasureMode::Exactly) {
    width = avail_width;
    is_fixed_width = true;
  } else {
    int resolved = ResolveSize(box->style.width, avail_width);
    if (resolved != -1) {
      width = resolved;
      is_fixed_width = true;
    } else {
      is_auto_width = true;
      width = (constraints.width.mode == MeasureMode::Undefined)
                  ? 0
                  : std::max(0, avail_width - box->style.margin.Horiz());
    }
  }

  // Apply max-width constraint
  int max_width_resolved = ResolveSize(box->style.max_width, avail_width);
  if (max_width_resolved != -1 && width > max_width_resolved) {
    width = max_width_resolved;
    is_auto_width = false;
  }

  int content_width_limit = std::max(
      0, width - box->style.padding.Horiz() - box->style.border.Horiz());

  // Helper to recursively find rows
  auto FindRows = [](auto& self, LayoutBox* curr,
                     std::vector<LayoutBox*>& rows) -> void {
    if (!curr) {
      return;
    }
    if (curr->dom_node && curr->dom_node->tag() == "tr") {
      rows.push_back(curr);
      return;
    }
    if (curr->dom_node &&
        (curr->dom_node->tag() == "table" || curr->dom_node->tag() == "tbody" ||
         curr->dom_node->tag() == "thead" || curr->dom_node->tag() == "tfoot" ||
         curr->is_anonymous)) {
      for (auto& child : curr->children) {
        self(self, child.get(), rows);
      }
    }
  };

  // Helper to recursively find cells
  auto FindCells = [](auto& self, LayoutBox* curr,
                      std::vector<LayoutBox*>& cells) -> void {
    if (!curr) {
      return;
    }
    if (curr->dom_node &&
        (curr->dom_node->tag() == "td" || curr->dom_node->tag() == "th")) {
      cells.push_back(curr);
      return;
    }
    for (auto& child : curr->children) {
      self(self, child.get(), cells);
    }
  };

  std::vector<LayoutBox*> rows;
  FindRows(FindRows, box, rows);

  std::vector<std::vector<LayoutBox*>> grid;
  size_t num_cols = 0;
  for (auto* row : rows) {
    std::vector<LayoutBox*> cells;
    for (auto& child : row->children) {
      FindCells(FindCells, child.get(), cells);
    }
    num_cols = std::max(num_cols, cells.size());
    grid.push_back(std::move(cells));
  }

  // If table is empty, return empty fragment
  if (grid.empty() || num_cols == 0) {
    auto fragment = MakeArenaFragment(width, 0);
    fragment->dom_node = box->dom_node;
    fragment->visibility = box->style.visibility;
    fragment->background_color = box->style.background_color;
    fragment->foreground_color = box->style.foreground_color;
    fragment->border_style = box->style.border_style;
    if ((box->style.border.Horiz() > 0 || box->style.border.Vert() > 0) &&
        box->style.border_style != BorderStyle::None) {
      fragment->has_border = true;
    }
    if (box->dom_node && !context.is_measurement) {
      box->dom_node->set_layout_width(fragment->width);
      box->dom_node->set_layout_height(fragment->height);
    }
    return fragment;
  }

  std::vector<int> col_preferred_width(num_cols, 0);

  // Pass 1: Measure cell preferred widths
  for (size_t col = 0; col < num_cols; ++col) {
    int max_pref = 0;
    for (size_t row = 0; row < grid.size(); ++row) {
      if (col < grid[row].size()) {
        auto* cell = grid[row][col];
        int cell_w = ResolveSize(cell->style.width, content_width_limit);
        if (cell_w != -1) {
          max_pref = std::max(max_pref, cell_w);
        } else {
          LayoutConstraints cell_c;
          cell_c.width = {content_width_limit, MeasureMode::AtMost};
          cell_c.height = {10000, MeasureMode::AtMost};
          // Measure cell width
          LayoutContext cell_context = context;
          cell_context.is_measurement = true;
          auto cell_frag = RunLayout({cell}, cell_c, cell_context);
          max_pref = std::max(max_pref, cell_frag->width);
        }
      }
    }
    col_preferred_width[col] = std::max(1, max_pref);
  }

  int total_preferred_width = 0;
  for (int w : col_preferred_width) {
    total_preferred_width += w;
  }

  std::vector<int> col_widths = col_preferred_width;

  // Distribute width
  if (is_auto_width && constraints.width.mode == MeasureMode::Undefined) {
    content_width_limit = total_preferred_width;
    width = content_width_limit + box->style.padding.Horiz() +
            box->style.border.Horiz();
  } else {
    if (total_preferred_width <= content_width_limit) {
      // If table is fixed-width or exactly constrained, stretch columns
      if (constraints.width.mode == MeasureMode::Exactly || is_fixed_width) {
        int remaining = content_width_limit - total_preferred_width;
        if (remaining > 0 && total_preferred_width > 0) {
          for (size_t col = 0; col < num_cols; ++col) {
            col_widths[col] +=
                (remaining * col_preferred_width[col]) / total_preferred_width;
          }
          int new_total = 0;
          for (int w : col_widths) {
            new_total += w;
          }
          int remainder = content_width_limit - new_total;
          if (remainder > 0 && !col_widths.empty()) {
            col_widths.back() += remainder;
          }
        }
      } else {
        // Auto-width fits preferred width
        content_width_limit = total_preferred_width;
        width = content_width_limit + box->style.padding.Horiz() +
                box->style.border.Horiz();
      }
    } else {
      // Shrink columns to fit content_width_limit
      if (total_preferred_width > 0) {
        for (size_t col = 0; col < num_cols; ++col) {
          col_widths[col] =
              std::max(1, (col_preferred_width[col] * content_width_limit) /
                              total_preferred_width);
        }
        int new_total = 0;
        for (int w : col_widths) {
          new_total += w;
        }
        int remainder = content_width_limit - new_total;
        if (remainder > 0 && !col_widths.empty()) {
          col_widths.back() += remainder;
        }
      }
    }
  }

  // Create table fragment
  auto fragment = MakeArenaFragment(width, 0);
  fragment->dom_node = box->dom_node;
  fragment->visibility = box->style.visibility;
  fragment->background_color = box->style.background_color;
  fragment->foreground_color = box->style.foreground_color;
  fragment->opacity = box->style.opacity;
  fragment->border_style = box->style.border_style;
  fragment->border_color_top = box->style.border_color_top;
  fragment->border_color_right = box->style.border_color_right;
  fragment->border_color_bottom = box->style.border_color_bottom;
  fragment->border_color_left = box->style.border_color_left;
  if ((box->style.border.Horiz() > 0 || box->style.border.Vert() > 0) &&
      box->style.border_style != BorderStyle::None) {
    fragment->has_border = true;
  }

  int cur_y = box->style.border.top + box->style.padding.top;
  int start_x = box->style.border.left + box->style.padding.left;

  // Pass 2: Layout rows and cells
  for (size_t row = 0; row < grid.size(); ++row) {
    // 1. Measure the natural height of the row with the resolved column widths
    int row_height = 0;
    for (size_t col = 0; col < grid[row].size(); ++col) {
      auto* cell = grid[row][col];
      LayoutConstraints cell_c;
      cell_c.width = {col_widths[col], MeasureMode::Exactly};
      cell_c.height = {10000, MeasureMode::AtMost};
      auto cell_frag = RunLayout({cell}, cell_c, context);
      row_height = std::max(row_height, cell_frag->height);
    }
    row_height = std::max(1, row_height);

    // 2. Create row fragment
    auto row_box = rows[row];
    auto row_frag = MakeArenaFragment(content_width_limit, row_height);
    row_frag->dom_node = row_box->dom_node;
    row_frag->background_color = row_box->style.background_color;
    row_frag->foreground_color = row_box->style.foreground_color;
    row_frag->opacity = row_box->style.opacity;
    row_frag->border_style = row_box->style.border_style;
    if ((row_box->style.border.Horiz() > 0 ||
         row_box->style.border.Vert() > 0) &&
        row_box->style.border_style != BorderStyle::None) {
      row_frag->has_border = true;
    }

    if (row_box->dom_node && !context.is_measurement) {
      row_box->dom_node->set_layout_width(content_width_limit);
      row_box->dom_node->set_layout_height(row_height);
    }

    // 3. Layout cells and add to row_frag
    int cur_x = 0;
    for (size_t col = 0; col < grid[row].size(); ++col) {
      auto* cell = grid[row][col];
      LayoutConstraints final_c;
      final_c.width = {col_widths[col], MeasureMode::Exactly};
      final_c.height = {row_height, MeasureMode::Exactly};

      // Coordinate relative to parent (which is the row)
      LayoutContext child_context = CreateChildContext(
          row_box, content_width_limit, row_height, cur_x, 0, context);
      auto final_cell_frag = RunLayout({cell}, final_c, child_context);

      row_frag->children.push_back({final_cell_frag, cur_x, 0});
      cur_x += col_widths[col];
    }

    // 4. Add row_frag to table fragment at (start_x, cur_y)
    fragment->children.push_back({row_frag, start_x, cur_y});
    cur_y += row_height;
  }

  fragment->height =
      cur_y + box->style.padding.bottom + box->style.border.bottom;

  if (box->dom_node && !context.is_measurement) {
    box->dom_node->set_layout_width(fragment->width);
    box->dom_node->set_layout_height(fragment->height);
  }

  return fragment;
}

}  // namespace rtxui
