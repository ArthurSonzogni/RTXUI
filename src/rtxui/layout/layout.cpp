#include "rtxui/layout/layout.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <numeric>
#include <sstream>
#include <vector>

#include "rtxui/base/string.hpp"
#include "rtxui/layout/layout_arena.hpp"
#include "rtxui/layout/layout_box.hpp"
#include "rtxui/layout/physical_fragment.hpp"
#include "rtxui/layout/style.hpp"

namespace rtxui {

namespace {
thread_local LayoutArena g_layout_arenas[2];
thread_local int g_active_layout_arena = 0;
}  // namespace

// Grid cell alignment: justify-items/justify-self control the inline axis,
// align-items/align-self the block axis. Stretch (the default) fills the
// cell; other values size the item naturally and offset it. Also used by
// LayoutFlex to detect a stretched cross axis ahead of the final pass.
static AlignItems EffectiveAlign(AlignSelf self, AlignItems items);

LayoutArena& ActiveLayoutArena() {
  return g_layout_arenas[g_active_layout_arena];
}

void ResetLayoutArena() {
  g_active_layout_arena ^= 1;
  g_layout_arenas[g_active_layout_arena].Reset();
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
  void* ptr = ActiveLayoutArena().Allocate(result_size, 1);
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
std::shared_ptr<PhysicalFragment> LayoutGrid(LayoutInputNode node,
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
  if (length.unit == Unit::Calc) {
    if (parent_size >= 0) {
      return length.value + (length.calc_percent * parent_size) / 100;
    }
    if (length.calc_percent == 0) {
      return length.value;  // Pure-cell calc() needs no basis.
    }
  }
  if (length.unit == Unit::MinMax) {
    MinMaxExpr expr = GetMinMaxExpr(static_cast<int>(length.value));
    if (parent_size >= 0 || !expr.DependsOnBasis()) {
      return expr.Evaluate(std::max(parent_size, 0));
    }
  }
  return -1;  // Represents 'Auto'
}

// Resolves a width/min-width/max-width/flex-basis Length against `style`'s
// own box, against the border-box (outer) size the rest of layout expects
// `width` to mean. box-sizing:content-box interprets the length as the
// content size instead, so border+padding (already resolved to fixed cell
// counts) are added back to recover the outer size.
int ResolveBoxWidth(const ComputedStyle& style,
                    const Length& length,
                    int parent_size) {
  int resolved = ResolveSize(length, parent_size);
  if (resolved != -1 && style.box_sizing == BoxSizing::ContentBox) {
    resolved += style.border.Horiz() + style.padding.Horiz();
  }
  return resolved;
}

// Same as ResolveBoxWidth, for height/min-height/max-height.
int ResolveBoxHeight(const ComputedStyle& style,
                     const Length& length,
                     int parent_size) {
  int resolved = ResolveSize(length, parent_size);
  if (resolved != -1 && style.box_sizing == BoxSizing::ContentBox) {
    resolved += style.border.Vert() + style.padding.Vert();
  }
  return resolved;
}

void AdjustOutOfFlowCoordinates(PhysicalFragment* frag,
                                int shift_x,
                                int shift_y,
                                Element* shifted_root) {
  if (shift_x == 0 && shift_y == 0) {
    return;
  }
  for (auto& child : frag->children) {
    if (child.fragment) {
      if (child.fragment->dom_node &&
          (child.fragment->dom_node->style.position == PositionType::Absolute ||
           child.fragment->dom_node->style.position == PositionType::Fixed)) {
        bool npa_inside_shifted_subtree = false;
        if (child.fragment->dom_node->style.position == PositionType::Absolute && shifted_root) {
          // Find Nearest Positioned Ancestor (NPA) of the absolute child
          Element* curr = child.fragment->dom_node->Parent();
          Element* npa = nullptr;
          while (curr) {
            if (curr->style.position != PositionType::Static) {
              npa = curr;
              break;
            }
            curr = curr->Parent();
          }
          // Check if NPA is equal to or a descendant of shifted_root
          if (npa) {
            Element* check = npa;
            while (check) {
              if (check == shifted_root) {
                npa_inside_shifted_subtree = true;
                break;
              }
              check = check->Parent();
            }
          }
        }

        if (!npa_inside_shifted_subtree) {
          child.x -= shift_x;
          child.y -= shift_y;
        }
      } else {
        AdjustOutOfFlowCoordinates(child.fragment.get(), shift_x, shift_y, shifted_root);
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
    std::cerr << "LayoutInputNode has null LayoutBox.\n";
    std::abort();
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
    case LayoutBox::Algorithm::Grid:
      return LayoutGrid(node, constraints, context);
    case LayoutBox::Algorithm::Text:
      std::cerr << "Text nodes should not be laid out directly. Use InlineFlow.\n";
      std::abort();
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
      int container_w = parent_context.npa_w;
      int container_h = parent_context.npa_h;
      int npa_offset_x = parent_context.npa_offset_x;
      int npa_offset_y = parent_context.npa_offset_y;

      if (is_fixed) {
        container_w = parent_context.viewport_w;
        container_h = parent_context.viewport_h;
        npa_offset_x = parent_context.viewport_offset_x;
        npa_offset_y = parent_context.viewport_offset_y;
      } else if (parent->style.position != PositionType::Static) {
        container_w = fragment->width;
        container_h = fragment->height;
        npa_offset_x = 0;
        npa_offset_y = 0;
      }

      LayoutConstraints child_c;
      int child_w = ResolveBoxWidth(child_box->style, child_box->style.width, container_w);
      int child_h = ResolveBoxHeight(child_box->style, child_box->style.height, container_h);

      int max_avail_w = container_w;
      if (child_w == -1 && (child_box->style.left.unit == Unit::Auto || child_box->style.right.unit == Unit::Auto)) {
        max_avail_w = parent_context.viewport_w;
      }
      int max_avail_h = container_h;
      if (child_h == -1 && (child_box->style.top.unit == Unit::Auto || child_box->style.bottom.unit == Unit::Auto)) {
        max_avail_h = parent_context.viewport_h;
      }

      child_c.width = {
          child_w != -1 ? child_w : max_avail_w,
          child_w != -1 ? MeasureMode::Exactly : MeasureMode::AtMost};
      child_c.height = {
          child_h != -1 ? child_h : max_avail_h,
          child_h != -1 ? MeasureMode::Exactly : MeasureMode::AtMost};

      int x = 0;
      int y = 0;

      // Temporary layout fragment to calculate child width/height
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

      // We run layout using a temporary child_context which we'll refine below
      LayoutContext child_context = parent_context;
      auto child_frag = RunLayout(child_input, child_c, child_context);

      if (child_box->style.left.unit != Unit::Auto && child_box->style.right.unit != Unit::Auto) {
        int left_val = child_box->style.left.Resolve(container_w);
        int right_val = child_box->style.right.Resolve(container_w);
        int avail_space = container_w - left_val - right_val;
        if (child_box->style.margin_left_auto && child_box->style.margin_right_auto) {
          int m = std::max(0, avail_space - child_frag->width) / 2;
          x = left_val + m;
        } else if (child_box->style.margin_left_auto) {
          x = left_val + std::max(0, avail_space - child_frag->width);
        } else if (child_box->style.margin_right_auto) {
          x = left_val + child_box->style.margin.left;
        } else {
          x = left_val + child_box->style.margin.left;
        }
      } else if (child_box->style.left.unit != Unit::Auto) {
        x = child_box->style.left.Resolve(container_w) + child_box->style.margin.left;
      } else if (child_box->style.right.unit != Unit::Auto) {
        x = container_w - child_box->style.right.Resolve(container_w) -
            child_frag->width - child_box->style.margin.right;
      } else {
        x = child_box->style.margin.left;
      }

      if (child_box->style.top.unit != Unit::Auto && child_box->style.bottom.unit != Unit::Auto) {
        int top_val = child_box->style.top.Resolve(container_h);
        int bottom_val = child_box->style.bottom.Resolve(container_h);
        int avail_space = container_h - top_val - bottom_val;
        if (child_box->style.margin_top_auto && child_box->style.margin_bottom_auto) {
          int m = std::max(0, avail_space - child_frag->height) / 2;
          y = top_val + m;
        } else if (child_box->style.margin_top_auto) {
          y = top_val + std::max(0, avail_space - child_frag->height);
        } else if (child_box->style.margin_bottom_auto) {
          y = top_val + child_box->style.margin.top;
        } else {
          y = top_val + child_box->style.margin.top;
        }
      } else if (child_box->style.top.unit != Unit::Auto) {
        y = child_box->style.top.Resolve(container_h) + child_box->style.margin.top;
      } else if (child_box->style.bottom.unit != Unit::Auto) {
        y = container_h - child_box->style.bottom.Resolve(container_h) -
            child_frag->height - child_box->style.margin.bottom;
      } else {
        y = child_box->style.margin.top;
      }

      int relative_to_parent_x = x - npa_offset_x;
      int relative_to_parent_y = y - npa_offset_y;

      // Update child_context viewport offset and nearest positioned ancestor context
      child_context.viewport_offset_x += relative_to_parent_x;
      child_context.viewport_offset_y += relative_to_parent_y;
      if (child_box->style.position != PositionType::Static) {
        child_context.npa_w = child_w != -1 ? child_w : container_w;
        child_context.npa_h = child_h != -1 ? child_h : container_h;
        child_context.npa_offset_x = 0;
        child_context.npa_offset_y = 0;
      }

      // Re-run layout if viewport_offset changed
      if (relative_to_parent_x != 0 || relative_to_parent_y != 0) {
        child_frag = RunLayout(child_input, child_c, child_context);
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

  // Resolved independently of width, so it's safe to consult before width is
  // known: used below so aspect-ratio can derive an auto width from a
  // definite height (the mirror of the height-from-width derivation further
  // down, which runs once `width` is already settled).
  int early_resolved_height = (constraints.height.mode == MeasureMode::Exactly)
                                   ? constraints.height.value
                                   : ResolveBoxHeight(box->style, box->style.height,
                                                      constraints.height.value);

  int width = 0;
  bool is_auto_width = false;

  if (constraints.width.mode == MeasureMode::Exactly) {
    width = avail_width;
  } else {
    int resolved = ResolveBoxWidth(box->style, box->style.width, avail_width);
    if (resolved != -1) {
      width = resolved;
    } else if (early_resolved_height != -1 && box->style.aspect_ratio > 0) {
      width = static_cast<int>(early_resolved_height * box->style.aspect_ratio +
                               0.5f);
    } else {
      is_auto_width = true;
      width = (constraints.width.mode == MeasureMode::Undefined)
                  ? 0
                  : std::max(0, avail_width - box->style.margin.Horiz());
    }
  }

  // Apply max-width constraint
  int max_width_resolved = ResolveBoxWidth(box->style, box->style.max_width, avail_width);
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
  fragment->dim = box->style.dim;
  fragment->italic = box->style.italic;
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
    int resolved =
        ResolveBoxHeight(box->style, box->style.height, constraints.height.value);
    if (resolved == -1 && box->style.aspect_ratio > 0) {
      resolved = static_cast<int>(width / box->style.aspect_ratio + 0.5f);
    }
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
    // Normally a child's own auto width should fill the available space
    // (MeasureMode::AtMost), matching standard block layout. But when this
    // box itself is being shrink-to-fit measured (is_auto_width &&
    // Undefined; e.g. a flex item with no explicit width), child_width_limit
    // is just the unbounded-placeholder 10000, not a real constraint -- an
    // auto-width block child "filling" it would report a width of ~10000
    // instead of its own natural size, corrupting this box's shrink-to-fit
    // sum (max_child_width below). Propagating Undefined instead makes an
    // auto-width block child shrink-to-fit too, recursing correctly through
    // nested auto-width block containers.
    MeasureMode child_width_mode =
        (is_auto_width && constraints.width.mode == MeasureMode::Undefined)
            ? MeasureMode::Undefined
            : MeasureMode::AtMost;
    child_c.width = {
        child_width_limit - child_box->style.margin.Horiz(),
        child_width_mode,
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
                                 shift - child_box->style.margin.left, 0,
                                 child_box->dom_node);
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
    int max_w = ResolveBoxWidth(box->style, box->style.max_width, avail_width);
    if (max_w != -1 && fragment->width > max_w) {
      fragment->width = max_w;
    }
  }

  // Apply min-width constraint
  {
    int min_w = ResolveBoxWidth(box->style, box->style.min_width, avail_width);
    if (min_w != -1 && fragment->width < min_w) {
      fragment->width = min_w;
    }
  }

  if (constraints.height.mode == MeasureMode::Exactly) {
    fragment->height = constraints.height.value;
  } else {
    int resolved_h =
        ResolveBoxHeight(box->style, box->style.height, constraints.height.value);
    if (resolved_h == -1 && box->style.aspect_ratio > 0) {
      // aspect-ratio derives the auto height from the used width. Content
      // taller than the ratio height overflows (pair with overflow if
      // clipping is desired).
      resolved_h =
          static_cast<int>(fragment->width / box->style.aspect_ratio + 0.5f);
    }
    fragment->height = (resolved_h != -1) ? resolved_h : cur_y;
  }

  // Cap height by max-height if needed
  {
    int max_h =
        ResolveBoxHeight(box->style, box->style.max_height, constraints.height.value);
    if (max_h != -1 && fragment->height > max_h) {
      fragment->height = max_h;
    }
  }

  // Apply min-height constraint
  {
    int min_h =
        ResolveBoxHeight(box->style, box->style.min_height, constraints.height.value);
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
  std::optional<bool> dim;
  std::optional<bool> italic;
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
                  : ResolveBoxWidth(box->style, box->style.width, avail_width);
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
  container_frag->dim = box->style.dim;
  container_frag->italic = box->style.italic;
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
  // CSS line-height acts as a minimum: tall inline children still grow the
  // line beyond it.
  const int min_line_height = box->style.line_height.value_or(1);
  // overflow-wrap: normal lets words longer than the line overflow instead
  // of emergency-breaking them at the container edge.
  const bool overflow_wrap_normal =
      box->style.overflow_wrap.value_or(OverflowWrap::Anywhere) ==
      OverflowWrap::Normal;
  // word-break: break-all treats every grapheme boundary as a break
  // opportunity, so a word wraps as soon as it would cross the line instead
  // of being pushed whole to the next line first.
  const bool word_break_all =
      box->style.word_break.value_or(WordBreak::Normal) == WordBreak::BreakAll;
  int line_height = min_line_height;
  int max_line_width = 0;

  struct LineInfo {
    size_t start_index;
    size_t end_index;
    int occupied_width;
    // True when the line was ended by an explicit newline in the text.
    // text-align: justify never stretches such lines (nor the last line).
    bool hard_break = false;
  };
  std::vector<LineInfo> lines;
  size_t line_start_index = 0;

  auto commit_line = [&](bool hard_break = false) {
    max_line_width = std::max(max_line_width, cursor_x);
    lines.push_back(
        {line_start_index, container_frag->children.size(), cursor_x,
         hard_break});
    cursor_x = 0;
    cursor_y += line_height;
    line_height = min_line_height;
    line_start_index = container_frag->children.size();
  };

  auto process_text_in_flow = [&](std::string_view text, Element* dom_node,
                                  const TextStyle& style) {
    if (box->style.text_overflow == TextOverflow::Ellipsis &&
        (box->style.white_space == WhiteSpace::Nowrap || box->style.white_space == WhiteSpace::Pre)) {
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

      text_frag->background_color = style.background_color;
      text_frag->foreground_color = style.foreground_color;
      text_frag->bold = style.bold;
      text_frag->dim = style.dim;
      text_frag->italic = style.italic;
      text_frag->underlined = style.underlined;
      text_frag->underlined_double = style.underlined_double;
      text_frag->strikethrough = style.strikethrough;
      text_frag->blink = style.blink;

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
          commit_line(/*hard_break=*/true);
          i = byte_end;
          continue;
        }

        if (c == ' ') {
          last_space_byte = i;
          last_space_col = cur_col - col_start;
          have_last_space = true;
        }

        if (box->style.white_space == WhiteSpace::Nowrap || box->style.white_space == WhiteSpace::Pre) {
          cur_col += g_width;
        } else if (cursor_x + (cur_col - col_start) + g_width >
                   content_width_limit) {
          if (have_last_space) {
            emit_frag(last_space_byte, last_space_col);
            byte_start = last_space_byte + 1;
            // cur_col already measures up to the current (overflowing)
            // grapheme; only col_start moves to just past the space, so
            // cur_col - col_start keeps counting the characters typed
            // between the space and here instead of discarding them.
            col_start = last_space_col + 1;
            have_last_space = false;
            commit_line();
            byte_end = i + 1;
            cur_col += g_width;
          } else if (overflow_wrap_normal && !word_break_all) {
            cur_col += g_width;  // Unbreakable word: let it overflow.
          } else if (!word_break_all && cursor_x > 0) {
            commit_line();
            cur_col += g_width;
          } else if (cur_col > col_start) {
            // Emergency break before this grapheme, keeping the line within
            // the limit.
            emit_frag(i, cur_col - col_start);
            byte_start = i;
            col_start = cur_col;
            commit_line();
            cur_col += g_width;
          } else {
            // A single grapheme wider than the line: emit it anyway so
            // layout makes progress.
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
        commit_line(/*hard_break=*/true);
        continue;
      }

      if (g.text.size() == 1 && g.text[0] == ' ') {
        last_space_byte = static_cast<size_t>(g.text.data() - text.data());
        last_space_col = cur_col - col_start;
        have_last_space = true;
      }

      if (box->style.white_space == WhiteSpace::Nowrap || box->style.white_space == WhiteSpace::Pre) {
        cur_col += g.width;
      } else if (cursor_x + (cur_col - col_start) + g.width >
                 content_width_limit) {
        if (have_last_space) {
          emit_frag(last_space_byte, last_space_col);
          byte_start = last_space_byte + 1;
          // cur_col already measures up to the current (overflowing)
          // grapheme; only col_start moves to just past the space, so
          // cur_col - col_start keeps counting the characters typed
          // between the space and here instead of discarding them.
          col_start = last_space_col + 1;
          // letter-spacing inserts NBSP padding between every grapheme
          // pair, including around the space just broken at. Drop any such
          // padding immediately after the space so the next line doesn't
          // start with a stray NBSP. This peeks directly at the text
          // buffer rather than the grapheme iterator's current position:
          // the loop below will still visit these same NBSP graphemes and
          // add their width to cur_col as usual, which nets out to zero
          // pending width since col_start already absorbed it here.
          while (byte_start + 1 < text.size() && text[byte_start] == '\xc2' &&
                 text[byte_start + 1] == '\xa0') {
            byte_start += 2;
            col_start += 1;
          }
          have_last_space = false;
          commit_line();
          byte_end =
              static_cast<size_t>(g.text.data() + g.text.size() - text.data());
          cur_col += g.width;
        } else if (overflow_wrap_normal && !word_break_all) {
          cur_col += g.width;  // Unbreakable word: let it overflow.
        } else if (!word_break_all && cursor_x > 0) {
          commit_line();
          cur_col += g.width;
        } else if (cur_col > col_start) {
          // Emergency break before this grapheme, keeping the line within
          // the limit.
          size_t frag_byte_end =
              static_cast<size_t>(g.text.data() - text.data());
          emit_frag(frag_byte_end, cur_col - col_start);
          byte_start = frag_byte_end;
          col_start = cur_col;
          commit_line();
          cur_col += g.width;
        } else {
          // A single grapheme wider than the line: emit it anyway so layout
          // makes progress.
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

  // <br> forces a line break in inline flow, regardless of its own display
  // style: it carries no content, so it would otherwise be a silent no-op
  // (an empty inline box contributes zero width and nothing to place).
  auto is_br = [](LayoutBox* elem) {
    return elem->dom_node && elem->dom_node->tag() == "br";
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
        box->style.white_space != WhiteSpace::Pre &&
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
           child->style.bold, child->style.dim, child->style.italic,
           child->style.underlined, child->style.underlined_double,
           child->style.strikethrough, child->style.blink});
    } else if (is_br(child.get())) {
      commit_line(/*hard_break=*/true);
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
          // background-color doesn't inherit, so a styled <span> (e.g.
          // class="selection") only carries it on `child` (the element),
          // never on the raw text node inside it (`grandchild`). The other
          // properties here do inherit, so grandchild's own (already
          // correctly inherited) values are used for those.
          process_text_in_flow(
              grandchild->text_data, child->dom_node,
              {child->style.background_color,
               grandchild->style.foreground_color, grandchild->style.bold,
               grandchild->style.dim, grandchild->style.italic,
               grandchild->style.underlined,
               grandchild->style.underlined_double,
               grandchild->style.strikethrough, grandchild->style.blink});
        } else if (is_br(grandchild.get())) {
          commit_line(/*hard_break=*/true);
        } else {
          place_opaque_box(grandchild.get());
        }
      }
    } else {
      place_opaque_box(child.get());
    }
  }

  commit_line();
  if (constraints.height.mode == MeasureMode::Exactly) {
    container_frag->height = constraints.height.value;
  } else {
    int resolved_h =
        ResolveBoxHeight(box->style, box->style.height, constraints.height.value);
    if (resolved_h != -1) {
      container_frag->height = resolved_h;
    } else {
      container_frag->height =
          cursor_y + box->style.padding.bottom + box->style.border.bottom;
    }
  }
  if (!is_fixed_width) {
    container_frag->width =
        max_line_width + box->style.padding.Horiz() + box->style.border.Horiz();
  }

  // Cap width by max-width if needed
  {
    int max_w = ResolveBoxWidth(box->style, box->style.max_width, avail_width);
    if (max_w != -1 && container_frag->width > max_w) {
      container_frag->width = max_w;
    }
  }

  // Apply min-width constraint
  {
    int min_w = ResolveBoxWidth(box->style, box->style.min_width, avail_width);
    if (min_w != -1 && container_frag->width < min_w) {
      container_frag->width = min_w;
    }
  }

  // Apply min-height constraint
  {
    int min_h =
        ResolveBoxHeight(box->style, box->style.min_height, constraints.height.value);
    if (min_h != -1 && container_frag->height < min_h) {
      container_frag->height = min_h;
    }
  }

  // Cap height by max-height if needed
  {
    int max_h =
        ResolveBoxHeight(box->style, box->style.max_height, constraints.height.value);
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

  // text-align: justify stretches the space runs of every soft-wrapped line
  // (never the last line or lines ended by an explicit newline) so the line
  // fills the content width. The spaces are widened in place: each affected
  // text fragment gets an arena-allocated copy of its text with extra spaces
  // inserted, so the styling of the spaces is preserved.
  if (box->style.text_align == TextAlign::Justify && final_content_width > 0 &&
      lines.size() > 1) {
    for (size_t li = 0; li + 1 < lines.size(); ++li) {
      const auto& line = lines[li];
      if (line.hard_break) {
        continue;
      }
      int extra = final_content_width - line.occupied_width;
      if (extra <= 0) {
        continue;
      }

      // Count expandable gaps: runs of spaces inside text fragments.
      // (0x20 never occurs inside a multi-byte UTF-8 sequence.)
      int gap_count = 0;
      for (size_t i = line.start_index; i < line.end_index; ++i) {
        const auto& frag = container_frag->children[i].fragment;
        if (!frag->is_text) {
          continue;
        }
        bool in_run = false;
        for (char c : frag->text_content) {
          if (c == ' ') {
            gap_count += !in_run;
            in_run = true;
          } else {
            in_run = false;
          }
        }
      }
      if (gap_count == 0) {
        continue;
      }

      int gap_seen = 0;
      int shift = 0;
      for (size_t i = line.start_index; i < line.end_index; ++i) {
        auto& link = container_frag->children[i];
        link.x += shift;
        auto& frag = link.fragment;
        if (!frag->is_text ||
            frag->text_content.find(' ') == std::string_view::npos) {
          continue;
        }

        std::string_view text = frag->text_content;
        // Worst case: every gap in this fragment takes the whole remainder.
        char* buffer = static_cast<char*>(
            ActiveLayoutArena().Allocate(text.size() + extra, 1));
        size_t out = 0;
        bool in_run = false;
        int added = 0;
        for (char c : text) {
          buffer[out++] = c;
          if (c == ' ') {
            if (!in_run) {
              in_run = true;
              int share = extra / gap_count + (gap_seen < extra % gap_count);
              ++gap_seen;
              for (int s = 0; s < share; ++s) {
                buffer[out++] = ' ';
              }
              added += share;
            }
          } else {
            in_run = false;
          }
        }
        frag->text_content = std::string_view(buffer, out);
        frag->width += added;
        shift += added;
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
                     : ResolveBoxWidth(box->style, box->style.width, parent_w);
  int my_height = (constraints.height.mode == MeasureMode::Exactly)
                      ? parent_h
                      : ResolveBoxHeight(box->style, box->style.height, parent_h);

  // aspect-ratio derives the flex container's auto dimension from whichever
  // of width/height is already resolved; min/max constraints below still
  // win. Only one of width/height can be auto here for the ratio to apply
  // unambiguously (if both are, sizing falls back to content as before).
  if (my_height == -1 && my_width != -1 && box->style.aspect_ratio > 0) {
    my_height = static_cast<int>(my_width / box->style.aspect_ratio + 0.5f);
  } else if (my_width == -1 && my_height != -1 && box->style.aspect_ratio > 0) {
    my_width = static_cast<int>(my_height * box->style.aspect_ratio + 0.5f);
  }

  // Apply max-width constraint
  int max_width_resolved = ResolveBoxWidth(box->style, box->style.max_width, parent_w);
  if (max_width_resolved != -1 && my_width > max_width_resolved) {
    my_width = max_width_resolved;
  }

  // Apply min-width constraint
  int min_width_resolved = ResolveBoxWidth(box->style, box->style.min_width, parent_w);
  if (min_width_resolved != -1 && my_width < min_width_resolved) {
    my_width = min_width_resolved;
  }

  // Apply max-height constraint
  int max_height_resolved = ResolveBoxHeight(box->style, box->style.max_height, parent_h);
  if (max_height_resolved != -1 && my_height > max_height_resolved) {
    my_height = max_height_resolved;
  }

  // Apply min-height constraint
  int min_height_resolved = ResolveBoxHeight(box->style, box->style.min_height, parent_h);
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
    int basis = -1;
    if (child->style.flex_basis.unit != Unit::Auto) {
      basis = is_row
                  ? ResolveBoxWidth(child->style, child->style.flex_basis, content_w)
                  : ResolveBoxHeight(child->style, child->style.flex_basis, content_h);
    } else {
      basis = is_row ? ResolveBoxWidth(child->style, child->style.width, content_w)
                     : ResolveBoxHeight(child->style, child->style.height, content_h);
    }

    // CSS "transferred size": a row item with an auto flex-basis (width) and
    // aspect-ratio that will be stretched to a definite cross size (height)
    // needs its basis derived from that cross size up front, since the main
    // size resolved here is locked in as an Exactly width by the final pass
    // below - too late for block-flow's own aspect-ratio-from-height to see
    // an auto width by then.
    if (is_row && basis == -1 && child->style.aspect_ratio > 0 &&
        child->style.height.unit == Unit::Auto &&
        EffectiveAlign(child->style.align_self, box->style.align_items) ==
            AlignItems::Stretch &&
        !(auto_height && constraints.height.mode == MeasureMode::Undefined)) {
      int stretched_h = content_h - child->style.margin.Vert();
      basis = static_cast<int>(stretched_h * child->style.aspect_ratio + 0.5f);
    }

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
    int max_w = ResolveBoxWidth(box->style, box->style.max_width, parent_w);
    if (max_w != -1 && resolved_width > max_w) {
      resolved_width = max_w;
    }
    int min_w = ResolveBoxWidth(box->style, box->style.min_width, parent_w);
    if (min_w != -1 && resolved_width < min_w) {
      resolved_width = min_w;
    }
    int max_h = ResolveBoxHeight(box->style, box->style.max_height, parent_h);
    if (max_h != -1 && resolved_height > max_h) {
      resolved_height = max_h;
    }
    int min_h = ResolveBoxHeight(box->style, box->style.min_height, parent_h);
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
  if (cross_is_definite && !lines.empty() &&
      box->style.align_content == AlignContent::Stretch) {
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
  fragment->dim = box->style.dim;
  fragment->italic = box->style.italic;
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
  if (cross_is_definite && !lines.empty() &&
      box->style.align_content != AlignContent::Stretch) {
    int total_lines_cross = 0;
    for (const auto& line : lines) {
      total_lines_cross += line.cross_size;
    }
    int remaining = container_cross_final - total_lines_cross;

    if (box->style.align_content == AlignContent::FlexStart) {
      int cur_line_offset = 0;
      for (size_t l = 0; l < lines.size(); ++l) {
        line_cross_offsets[l] = cur_line_offset;
        cur_line_offset += lines[l].cross_size + resolved_cross_gap;
      }
    } else if (box->style.align_content == AlignContent::FlexEnd) {
      int total_occupied =
          total_lines_cross + (lines.size() - 1) * resolved_cross_gap;
      int start_offset = std::max(0, container_cross_final - total_occupied);
      int cur_line_offset = start_offset;
      for (size_t l = 0; l < lines.size(); ++l) {
        line_cross_offsets[l] = cur_line_offset;
        cur_line_offset += lines[l].cross_size + resolved_cross_gap;
      }
    } else if (box->style.align_content == AlignContent::Center) {
      int total_occupied =
          total_lines_cross + (lines.size() - 1) * resolved_cross_gap;
      int start_offset =
          std::max(0, (container_cross_final - total_occupied) / 2);
      int cur_line_offset = start_offset;
      for (size_t l = 0; l < lines.size(); ++l) {
        line_cross_offsets[l] = cur_line_offset;
        cur_line_offset += lines[l].cross_size + resolved_cross_gap;
      }
    } else if (box->style.align_content == AlignContent::SpaceBetween) {
      if (lines.size() == 1) {
        line_cross_offsets[0] = 0;
      } else {
        float gap = static_cast<float>(remaining) / (lines.size() - 1);
        float current = 0.0f;
        for (size_t l = 0; l < lines.size(); ++l) {
          line_cross_offsets[l] = static_cast<int>(current + 0.5f);
          current += lines[l].cross_size + gap;
        }
      }
    } else if (box->style.align_content == AlignContent::SpaceAround) {
      float gap = static_cast<float>(remaining) / lines.size();
      float current = gap / 2.0f;
      for (size_t l = 0; l < lines.size(); ++l) {
        line_cross_offsets[l] = static_cast<int>(current + 0.5f);
        current += lines[l].cross_size + gap;
      }
    } else if (box->style.align_content == AlignContent::SpaceEvenly) {
      float gap = static_cast<float>(remaining) / (lines.size() + 1);
      float current = gap;
      for (size_t l = 0; l < lines.size(); ++l) {
        line_cross_offsets[l] = static_cast<int>(current + 0.5f);
        current += lines[l].cross_size + gap;
      }
    }
  } else {
    int cur_line_offset = 0;
    for (size_t l = 0; l < lines.size(); ++l) {
      line_cross_offsets[l] = cur_line_offset;
      cur_line_offset += lines[l].cross_size + resolved_cross_gap;
    }
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

      AlignItems effective_align = box->style.align_items;
      if (item.child_ptr->style.align_self != AlignSelf::Auto) {
        switch (item.child_ptr->style.align_self) {
          case AlignSelf::Stretch:
            effective_align = AlignItems::Stretch;
            break;
          case AlignSelf::FlexStart:
            effective_align = AlignItems::FlexStart;
            break;
          case AlignSelf::FlexEnd:
            effective_align = AlignItems::FlexEnd;
            break;
          case AlignSelf::Center:
            effective_align = AlignItems::Center;
            break;
          case AlignSelf::Baseline:
            effective_align = AlignItems::Baseline;
            break;
          default:
            break;
        }
      }

      if (is_row) {
        final_c.width = {item.main_resolved_size - m_horiz,
                         MeasureMode::Exactly};
        if (effective_align == AlignItems::Stretch &&
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
        if (effective_align == AlignItems::Stretch &&
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
      if (effective_align != AlignItems::Stretch) {
        int cross_free_space = line.cross_size - item.cross_size;
        if (cross_free_space > 0) {
          if (effective_align == AlignItems::FlexEnd) {
            item_cross_pos += cross_free_space;
          } else if (effective_align == AlignItems::Center) {
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
    int resolved = ResolveBoxWidth(box->style, box->style.width, avail_width);
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
  int max_width_resolved = ResolveBoxWidth(box->style, box->style.max_width, avail_width);
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

  struct GridCell {
    LayoutBox* box = nullptr;
    int colspan = 1;
    int rowspan = 1;
    bool is_top_left = false;
  };

  std::vector<std::vector<GridCell>> grid;
  size_t num_cols = 0;

  std::vector<std::vector<LayoutBox*>> row_cells(rows.size());
  for (size_t r = 0; r < rows.size(); ++r) {
    FindCells(FindCells, rows[r], row_cells[r]);
  }

  grid.resize(rows.size());
  for (size_t r = 0; r < rows.size(); ++r) {
    size_t c = 0;
    for (LayoutBox* cell : row_cells[r]) {
      while (c < grid[r].size() && grid[r][c].box != nullptr) {
        c++;
      }
      
      int colspan = 1;
      int rowspan = 1;
      if (cell->dom_node) {
        if (auto* cs = cell->dom_node->GetAttribute("colspan")) {
          std::string_view s = *cs;
          while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
            s.remove_prefix(1);
          }
          while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
            s.remove_suffix(1);
          }
          if (!s.empty()) {
            int val = 1;
            auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
            if (ec == std::errc() && ptr == s.data() + s.size()) {
              colspan = std::max(1, val);
            }
          }
        }
        if (auto* rs = cell->dom_node->GetAttribute("rowspan")) {
          std::string_view s = *rs;
          while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
            s.remove_prefix(1);
          }
          while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
            s.remove_suffix(1);
          }
          if (!s.empty()) {
            int val = 1;
            auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
            if (ec == std::errc() && ptr == s.data() + s.size()) {
              rowspan = std::max(1, val);
            }
          }
        }
      }

      if (c + colspan > grid[r].size()) {
        grid[r].resize(c + colspan);
      }

      for (int i = 0; i < rowspan; ++i) {
        if (r + i >= grid.size()) {
          grid.resize(r + i + 1);
        }
        for (int j = 0; j < colspan; ++j) {
          if (c + j >= grid[r + i].size()) {
            grid[r + i].resize(c + j + 1);
          }
          grid[r + i][c + j].box = cell;
          grid[r + i][c + j].colspan = colspan;
          grid[r + i][c + j].rowspan = rowspan;
          grid[r + i][c + j].is_top_left = (i == 0 && j == 0);
        }
      }
      c += colspan;
    }
    num_cols = std::max(num_cols, grid[r].size());
  }

  for (auto& row_grid : grid) {
    row_grid.resize(num_cols);
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

  std::vector<int> col_preferred_width(num_cols, 1);

  // Pass 1: Measure cell preferred widths
  for (size_t row = 0; row < grid.size(); ++row) {
    for (size_t col = 0; col < num_cols; ++col) {
      if (grid[row][col].is_top_left && grid[row][col].box) {
        auto* cell = grid[row][col].box;
        int cell_w = ResolveBoxWidth(cell->style, cell->style.width, content_width_limit);
        int measured_width = 0;
        if (cell_w != -1) {
          measured_width = cell_w;
        } else {
          LayoutConstraints cell_c;
          cell_c.width = {content_width_limit, MeasureMode::AtMost};
          cell_c.height = {10000, MeasureMode::AtMost};
          LayoutContext cell_context = context;
          cell_context.is_measurement = true;
          auto cell_frag = RunLayout({cell}, cell_c, cell_context);
          measured_width = cell_frag->width;
        }
        
        int colspan = grid[row][col].colspan;
        int pref_per_col = (measured_width + colspan - 1) / colspan;
        for (int c = 0; c < colspan; ++c) {
          col_preferred_width[col + c] = std::max(col_preferred_width[col + c], pref_per_col);
        }
      }
    }
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

  std::vector<int> col_x(num_cols + 1, 0);
  for (size_t col = 0; col < num_cols; ++col) {
    col_x[col + 1] = col_x[col] + col_widths[col];
  }

  std::vector<int> row_heights(grid.size(), 1);
  for (size_t row = 0; row < grid.size(); ++row) {
    for (size_t col = 0; col < num_cols; ++col) {
      if (grid[row][col].is_top_left && grid[row][col].box) {
        auto* cell = grid[row][col].box;
        int colspan = grid[row][col].colspan;
        int rowspan = grid[row][col].rowspan;
        
        int cell_avail_width = 0;
        for (int c = 0; c < colspan; ++c) {
          cell_avail_width += col_widths[col + c];
        }

        LayoutConstraints cell_c;
        cell_c.width = {cell_avail_width, MeasureMode::Exactly};
        cell_c.height = {10000, MeasureMode::AtMost};
        LayoutContext cell_context = context;
        cell_context.is_measurement = true;
        auto cell_frag = RunLayout({cell}, cell_c, cell_context);
        


        int height_per_row = (cell_frag->height + rowspan - 1) / rowspan;
        for (int r = 0; r < rowspan; ++r) {
           if (row + r < row_heights.size()) {
             row_heights[row + r] = std::max(row_heights[row + r], height_per_row);
           }
        }
      }
    }
  }

  std::vector<PhysicalFragment::ChildLink> row_bg_links;
  std::vector<PhysicalFragment::ChildLink> row_cells_links;

  for (size_t row = 0; row < grid.size(); ++row) {
    int row_height = row_heights[row];
    LayoutBox* row_box = (row < rows.size()) ? rows[row] : nullptr;

    auto row_bg_frag = MakeArenaFragment(content_width_limit, row_height);
    auto row_cells_frag = MakeArenaFragment(content_width_limit, row_height);

    if (row_box) {
      row_bg_frag->dom_node = row_box->dom_node;
      row_bg_frag->background_color = row_box->style.background_color;
      row_bg_frag->foreground_color = row_box->style.foreground_color;
      row_bg_frag->opacity = row_box->style.opacity;
      row_bg_frag->border_style = row_box->style.border_style;
      row_bg_frag->border_color_top = row_box->style.border_color_top;
      row_bg_frag->border_color_right = row_box->style.border_color_right;
      row_bg_frag->border_color_bottom = row_box->style.border_color_bottom;
      row_bg_frag->border_color_left = row_box->style.border_color_left;
      if ((row_box->style.border.Horiz() > 0 ||
           row_box->style.border.Vert() > 0) &&
          row_box->style.border_style != BorderStyle::None) {
        row_bg_frag->has_border = true;
      }
      if (row_box->dom_node && !context.is_measurement) {
        row_box->dom_node->set_layout_width(content_width_limit);
        row_box->dom_node->set_layout_height(row_height);
      }

      row_cells_frag->dom_node = row_box->dom_node;
      row_cells_frag->opacity = row_box->style.opacity;
    }

    for (size_t col = 0; col < num_cols; ++col) {
      auto& gcell = grid[row][col];
      if (gcell.is_top_left && gcell.box) {
        auto* cell = gcell.box;
        int cell_width = 0;
        for (int c = 0; c < gcell.colspan; ++c) cell_width += col_widths[col + c];
        
        int cell_height = 0;
        for (int r = 0; r < gcell.rowspan; ++r) {
           if (row + r < row_heights.size()) {
             cell_height += row_heights[row + r];
           }
        }

        LayoutConstraints final_c;
        final_c.width = {cell_width, MeasureMode::Exactly};
        final_c.height = {cell_height, MeasureMode::Exactly};

        LayoutContext child_context = CreateChildContext(
            row_box, content_width_limit, row_height, col_x[col], 0, context);
        auto final_cell_frag = RunLayout({cell}, final_c, child_context);

        row_cells_frag->children.push_back({final_cell_frag, col_x[col], 0});
      }
    }

    row_bg_links.push_back({row_bg_frag, start_x, cur_y});
    row_cells_links.push_back({row_cells_frag, start_x, cur_y});
    cur_y += row_height;
  }

  for (auto& link : row_bg_links) fragment->children.push_back(link);
  for (auto& link : row_cells_links) fragment->children.push_back(link);

  fragment->height =
      cur_y + box->style.padding.bottom + box->style.border.bottom;

  if (box->dom_node && !context.is_measurement) {
    box->dom_node->set_layout_width(fragment->width);
    box->dom_node->set_layout_height(fragment->height);
  }

  return fragment;
}

static AlignItems EffectiveAlign(AlignSelf self, AlignItems items) {
  switch (self) {
    case AlignSelf::Auto:
      return items;
    case AlignSelf::Stretch:
      return AlignItems::Stretch;
    case AlignSelf::FlexStart:
      return AlignItems::FlexStart;
    case AlignSelf::FlexEnd:
      return AlignItems::FlexEnd;
    case AlignSelf::Center:
      return AlignItems::Center;
    case AlignSelf::Baseline:
      return AlignItems::Baseline;
  }
  return items;
}

std::shared_ptr<PhysicalFragment> LayoutGrid(
    LayoutInputNode node,
    LayoutConstraints constraints,
    LayoutContext context) {
  auto* box = node.box;

  int border_h = box->style.border.Horiz();
  int border_v = box->style.border.Vert();
  int padding_h = box->style.padding.Horiz();
  int padding_v = box->style.padding.Vert();

  // Resolved independently of width, so it's safe to consult before width is
  // known: lets aspect-ratio derive an auto width from a definite height,
  // mirroring the height-from-width derivation in the final pass below. Must
  // run before avail_width, since column-track sizing needs it.
  int early_resolved_height = (constraints.height.mode == MeasureMode::Exactly)
                                   ? constraints.height.value
                                   : ResolveBoxHeight(box->style, box->style.height,
                                                      constraints.height.value);

  int parent_width = constraints.width.value;
  if (constraints.width.mode != MeasureMode::Exactly) {
    int resolved = ResolveBoxWidth(box->style, box->style.width, constraints.width.value);
    if (resolved != -1) {
      parent_width = resolved;
    } else if (early_resolved_height != -1 && box->style.aspect_ratio > 0) {
      parent_width = static_cast<int>(
          early_resolved_height * box->style.aspect_ratio + 0.5f);
    }
  }
  int avail_width = parent_width - border_h - padding_h;

  int parent_height = 0;
  if (constraints.height.mode != MeasureMode::Undefined) {
    parent_height = constraints.height.value;
  } else {
    int resolved =
        ResolveBoxHeight(box->style, box->style.height, constraints.height.value);
    if (resolved != -1) {
      parent_height = resolved;
    }
  }
  int avail_height = std::max(0, parent_height - border_v - padding_v);

  // 1. Determine columns template. If empty, default to one 1fr column.
  std::vector<Length> cols_template = box->style.grid_template_columns;
  if (cols_template.empty()) {
    cols_template.push_back(Length::Fr(1.0f));
  }
  int C = cols_template.size();

  // 2. Resolve Column Widths
  int col_gap_val = box->style.column_gap.Resolve(avail_width);
  int total_col_gaps = std::max(0, C - 1) * col_gap_val;
  int remaining_width = std::max(0, avail_width - total_col_gaps);

  std::vector<int> col_widths(C, 0);
  float total_col_fr = 0.0f;
  int non_fr_col_width = 0;
  for (int c = 0; c < C; ++c) {
    if (cols_template[c].unit == Unit::Fr) {
      total_col_fr += cols_template[c].value;
    } else {
      col_widths[c] = cols_template[c].Resolve(remaining_width);
      non_fr_col_width += col_widths[c];
    }
  }
  int free_col_width = std::max(0, remaining_width - non_fr_col_width);
  if (total_col_fr > 0.0f) {
    for (int c = 0; c < C; ++c) {
      if (cols_template[c].unit == Unit::Fr) {
        col_widths[c] = static_cast<int>(free_col_width * (cols_template[c].value / total_col_fr));
      }
    }
  } else {
    // If all tracks are Auto, distribute equally
    int num_auto = 0;
    for (int c = 0; c < C; ++c) {
      if (cols_template[c].unit == Unit::Auto) {
        num_auto++;
      }
    }
    if (num_auto > 0) {
      int auto_width = free_col_width / num_auto;
      for (int c = 0; c < C; ++c) {
        if (cols_template[c].unit == Unit::Auto) {
          col_widths[c] = auto_width;
        }
      }
    }
  }

  // Count the number of non-absolute/non-fixed children
  int num_children = 0;
  for (const auto& child : box->children) {
    if (child->style.position != PositionType::Absolute &&
        child->style.position != PositionType::Fixed) {
      num_children++;
    }
  }

  // 3. Auto-placement using Sparse Algorithm with Occupancy Grid
  struct OccupancyGrid {
    int C;
    std::vector<std::vector<bool>> grid;

    OccupancyGrid(int cols) : C(cols) {}

    bool IsOccupied(int r, int c) const {
      if (r >= static_cast<int>(grid.size())) return false;
      if (c >= C) return true;
      return grid[r][c];
    }

    void SetOccupied(int r, int c, int r_span, int c_span) {
      int max_r = r + r_span - 1;
      if (max_r >= static_cast<int>(grid.size())) {
        grid.resize(max_r + 1, std::vector<bool>(C, false));
      }
      for (int i = r; i < r + r_span; ++i) {
        for (int j = c; j < c + c_span; ++j) {
          grid[i][j] = true;
        }
      }
    }

    bool CanFit(int r, int c, int r_span, int c_span) const {
      if (c + c_span > C) return false;
      for (int i = r; i < r + r_span; ++i) {
        for (int j = c; j < c + c_span; ++j) {
          if (IsOccupied(i, j)) return false;
        }
      }
      return true;
    }
  };

  struct PlacedChild {
    LayoutBox* box;
    std::shared_ptr<PhysicalFragment> fragment;
    int row = 0;
    int col = 0;
    int r_span = 1;
    int c_span = 1;
  };
  std::vector<PlacedChild> placed_children;
  placed_children.reserve(num_children);

  OccupancyGrid occupancy(C);
  int cursor_r = 0;
  int cursor_c = 0;

  for (auto& child : box->children) {
    if (child->style.position == PositionType::Absolute ||
        child->style.position == PositionType::Fixed) {
      continue;
    }

    int r_span = std::max(1, child->style.grid_row_span);
    int c_span = std::max(1, child->style.grid_column_span);
    c_span = std::min(c_span, C);

    int placed_r = -1;
    int placed_c = -1;

    int r = cursor_r;
    int c = cursor_c;
    while (true) {
      if (occupancy.CanFit(r, c, r_span, c_span)) {
        placed_r = r;
        placed_c = c;
        break;
      }
      c++;
      if (c + c_span > C) {
        c = 0;
        r++;
      }
    }

    occupancy.SetOccupied(placed_r, placed_c, r_span, c_span);

    // Update cursor for sparse algorithm
    cursor_r = placed_r;
    cursor_c = placed_c + c_span;
    if (cursor_c >= C) {
      cursor_c = 0;
      cursor_r++;
    }

    PlacedChild pc;
    pc.box = child.get();
    pc.row = placed_r;
    pc.col = placed_c;
    pc.r_span = r_span;
    pc.c_span = c_span;
    placed_children.push_back(pc);
  }

  // 4. Resolve total rows needed
  int R = 1;
  for (const auto& pc : placed_children) {
    R = std::max(R, pc.row + pc.r_span);
  }
  R = std::max(R, static_cast<int>(box->style.grid_template_rows.size()));

  // 5. Resolve Row Heights
  int row_gap_val = box->style.row_gap.Resolve(avail_height);
  int total_row_gaps = std::max(0, R - 1) * row_gap_val;
  int remaining_height = std::max(0, avail_height - total_row_gaps);

  std::vector<int> row_heights(R, 0);
  std::vector<bool> is_row_fr(R, false);
  float total_row_fr = 0.0f;

  const auto& rows_template = box->style.grid_template_rows;
  for (int r = 0; r < R; ++r) {
    if (r < static_cast<int>(rows_template.size())) {
      if (rows_template[r].unit == Unit::Fr) {
        is_row_fr[r] = true;
        total_row_fr += rows_template[r].value;
      } else {
        row_heights[r] = rows_template[r].Resolve(remaining_height);
      }
    }
  }

  // Measure children with exact widths and calculated/undefined heights
  for (auto& pc : placed_children) {
    LayoutConstraints child_c;

    int child_width = 0;
    for (int i = 0; i < pc.c_span; ++i) {
      child_width += col_widths[pc.col + i];
    }
    child_width += (pc.c_span - 1) * col_gap_val;
    // Mirror the final pass: only a stretched item fills the cell exactly.
    // A non-stretched one is sized by its own width, so measure it that way
    // (an aspect-ratio height must derive from the used width, not the
    // track width).
    bool stretch_x = EffectiveAlign(pc.box->style.justify_self,
                                    box->style.justify_items) ==
                         AlignItems::Stretch &&
                     pc.box->style.width.unit == Unit::Auto;
    child_c.width = {child_width,
                     stretch_x ? MeasureMode::Exactly : MeasureMode::AtMost};

    bool all_fixed = true;
    int fixed_height_sum = 0;
    for (int i = 0; i < pc.r_span; ++i) {
      int idx = pc.row + i;
      if (idx >= static_cast<int>(rows_template.size()) || is_row_fr[idx] || rows_template[idx].unit == Unit::Auto) {
        all_fixed = false;
        break;
      } else {
        fixed_height_sum += row_heights[idx];
      }
    }

    if (all_fixed) {
      fixed_height_sum += (pc.r_span - 1) * row_gap_val;
      child_c.height = {fixed_height_sum, MeasureMode::Exactly};
    } else {
      child_c.height = {0, MeasureMode::Undefined};
    }

    LayoutContext child_context = CreateChildContext(box, parent_width, parent_height, 0, 0, context);
    child_context.is_measurement = true;

    pc.fragment = RunLayout({pc.box}, child_c, child_context);
  }

  // Distribute measured heights among rows
  // Sort children by row span to size smaller spans first
  std::vector<size_t> indices(placed_children.size());
  std::iota(indices.begin(), indices.end(), 0);
  std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
    return placed_children[a].r_span < placed_children[b].r_span;
  });

  for (size_t idx : indices) {
    auto& pc = placed_children[idx];
    int current_sum = 0;
    for (int i = 0; i < pc.r_span; ++i) {
      current_sum += row_heights[pc.row + i];
    }

    int target_sum = pc.fragment->height - (pc.r_span - 1) * row_gap_val;
    int extra = target_sum - current_sum;
    if (extra > 0) {
      int growable_count = 0;
      for (int i = 0; i < pc.r_span; ++i) {
        int r_idx = pc.row + i;
        if (r_idx >= static_cast<int>(rows_template.size()) || is_row_fr[r_idx] || rows_template[r_idx].unit == Unit::Auto) {
          growable_count++;
        }
      }

      if (growable_count > 0) {
        int extra_per_row = extra / growable_count;
        int remainder = extra % growable_count;
        for (int i = 0; i < pc.r_span; ++i) {
          int r_idx = pc.row + i;
          if (r_idx >= static_cast<int>(rows_template.size()) || is_row_fr[r_idx] || rows_template[r_idx].unit == Unit::Auto) {
            int amount = extra_per_row + (remainder > 0 ? 1 : 0);
            if (remainder > 0) remainder--;
            row_heights[r_idx] += amount;
          }
        }
      }
    }
  }

  // Distribute flexible rows if we have a fixed container height
  if (total_row_fr > 0.0f && (constraints.height.mode != MeasureMode::Undefined || parent_height > 0)) {
    int non_fr_row_height = 0;
    for (int r = 0; r < R; ++r) {
      if (!is_row_fr[r]) {
        non_fr_row_height += row_heights[r];
      }
    }
    int free_row_height = std::max(0, remaining_height - non_fr_row_height);
    for (int r = 0; r < R; ++r) {
      if (is_row_fr[r]) {
        int fr_height = static_cast<int>(free_row_height * (rows_template[r].value / total_row_fr));
        row_heights[r] = std::max(row_heights[r], fr_height);
      }
    }
  }

  // 6. Final Layout Pass
  auto container_frag = MakeArenaFragment(parent_width, 0);
  container_frag->dom_node = box->dom_node;
  container_frag->visibility = box->style.visibility;
  container_frag->background_color = box->style.background_color;
  container_frag->foreground_color = box->style.foreground_color;
  container_frag->opacity = box->style.opacity;
  container_frag->bold = box->style.bold;
  container_frag->dim = box->style.dim;
  container_frag->italic = box->style.italic;
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

  std::vector<int> col_offsets(C, 0);
  int cur_x = box->style.padding.left + box->style.border.left;
  for (int c = 0; c < C; ++c) {
    col_offsets[c] = cur_x;
    cur_x += col_widths[c] + col_gap_val;
  }

  std::vector<int> row_offsets(R, 0);
  int cur_y = box->style.padding.top + box->style.border.top;
  for (int r = 0; r < R; ++r) {
    row_offsets[r] = cur_y;
    cur_y += row_heights[r] + row_gap_val;
  }

  for (auto& pc : placed_children) {
    int r = pc.row;
    int c = pc.col;

    int final_w = 0;
    for (int i = 0; i < pc.c_span; ++i) {
      final_w += col_widths[c + i];
    }
    final_w += (pc.c_span - 1) * col_gap_val;

    int final_h = 0;
    for (int i = 0; i < pc.r_span; ++i) {
      final_h += row_heights[r + i];
    }
    final_h += (pc.r_span - 1) * row_gap_val;

    AlignItems justify =
        EffectiveAlign(pc.box->style.justify_self, box->style.justify_items);
    AlignItems align =
        EffectiveAlign(pc.box->style.align_self, box->style.align_items);

    bool stretch_x = (justify == AlignItems::Stretch) &&
                     pc.box->style.width.unit == Unit::Auto;
    bool stretch_y = (align == AlignItems::Stretch) &&
                     pc.box->style.height.unit == Unit::Auto;

    LayoutConstraints final_c;
    final_c.width = {final_w, stretch_x ? MeasureMode::Exactly : MeasureMode::AtMost};
    final_c.height = {final_h, stretch_y ? MeasureMode::Exactly : MeasureMode::AtMost};

    int cx = col_offsets[c];
    int cy = row_offsets[r];

    LayoutContext final_context = CreateChildContext(box, parent_width, parent_height, cx, cy, context);
    final_context.is_measurement = context.is_measurement;

    auto final_frag = RunLayout({pc.box}, final_c, final_context);

    if (!stretch_x && final_frag->width < final_w) {
      if (justify == AlignItems::Center) {
        cx += (final_w - final_frag->width) / 2;
      } else if (justify == AlignItems::FlexEnd) {
        cx += final_w - final_frag->width;
      }
    }
    if (!stretch_y && final_frag->height < final_h) {
      if (align == AlignItems::Center) {
        cy += (final_h - final_frag->height) / 2;
      } else if (align == AlignItems::FlexEnd) {
        cy += final_h - final_frag->height;
      }
    }

    container_frag->children.push_back({final_frag, cx, cy});
  }

  int content_w = 0;
  if (!col_offsets.empty()) {
    content_w = col_offsets.back() + col_widths.back() - (box->style.padding.left + box->style.border.left);
  }
  int content_h = 0;
  if (!row_offsets.empty()) {
    content_h = row_offsets.back() + row_heights.back() - (box->style.padding.top + box->style.border.top);
  }

  container_frag->width = content_w + border_h + padding_h;
  if (constraints.width.mode == MeasureMode::Exactly) {
    container_frag->width = constraints.width.value;
  }

  // Apply min-width/max-width constraints, unless width is exactly fixed by
  // the parent constraint (matches LayoutBlockFlow/LayoutFlex: an Exactly
  // constraint always wins). Percentages resolve against the incoming
  // constraint (the true containing block), not the local `parent_width`
  // above (this grid's own resolved width, used for track sizing).
  if (constraints.width.mode != MeasureMode::Exactly) {
    int max_w =
        ResolveBoxWidth(box->style, box->style.max_width, constraints.width.value);
    if (max_w != -1 && container_frag->width > max_w) {
      container_frag->width = max_w;
    }
    int min_w =
        ResolveBoxWidth(box->style, box->style.min_width, constraints.width.value);
    if (min_w != -1 && container_frag->width < min_w) {
      container_frag->width = min_w;
    }
  }

  container_frag->height = content_h + border_v + padding_v;
  if (constraints.height.mode == MeasureMode::Exactly) {
    container_frag->height = constraints.height.value;
  } else if (box->style.aspect_ratio > 0 &&
             ResolveBoxHeight(box->style, box->style.height,
                              constraints.height.value) == -1) {
    // aspect-ratio derives the grid container's auto height from its used
    // width; rows taller than the ratio height overflow.
    container_frag->height = static_cast<int>(
        container_frag->width / box->style.aspect_ratio + 0.5f);
  }

  // Apply min-height/max-height constraints, same rule as width above.
  if (constraints.height.mode != MeasureMode::Exactly) {
    int max_h = ResolveBoxHeight(box->style, box->style.max_height,
                                 constraints.height.value);
    if (max_h != -1 && container_frag->height > max_h) {
      container_frag->height = max_h;
    }
    int min_h = ResolveBoxHeight(box->style, box->style.min_height,
                                 constraints.height.value);
    if (min_h != -1 && container_frag->height < min_h) {
      container_frag->height = min_h;
    }
  }

  if (box->dom_node && !context.is_measurement) {
    box->dom_node->set_layout_width(container_frag->width);
    box->dom_node->set_layout_height(container_frag->height);
  }

  LayoutOutOfFlowChildren(box, container_frag, context);

  return container_frag;
}

}  // namespace rtxui

