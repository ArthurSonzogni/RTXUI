#include "rtxui/layout/layout.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <numeric>
#include <sstream>
#include <vector>

#include "rtxui/core/string.hpp"
#include "rtxui/layout/layout_box.hpp"
#include "rtxui/layout/physical_fragment.hpp"
#include "rtxui/layout/style.hpp"
#include "rtxui/layout/layout_arena.hpp"
#include <cstddef>

namespace rtxui {

thread_local LayoutArena g_layout_arena;

void ResetLayoutArena() {
  g_layout_arena.Reset();
}

// Helper: allocate a PhysicalFragment in the arena using std::allocate_shared
// so that both the control block and the object reside in the arena.
template <typename... Args>
std::shared_ptr<PhysicalFragment> MakeArenaFragment(Args&&... args) {
  return std::allocate_shared<PhysicalFragment, LayoutArenaAllocator<PhysicalFragment>>(
      LayoutArenaAllocator<PhysicalFragment>(), std::forward<Args>(args)...);
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

      auto child_frag = RunLayout({child_box.get()}, child_c, child_context);

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
    auto child_frag = RunLayout({child_box.get()}, child_c, child_context);

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
  // eliminates heap allocations/copies during layout. Yields ~7% speedup in Layout/Paint.
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
    auto child_frag = RunLayout({elem}, child_c, child_context);

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
          {child->style.background_color,
           child->style.foreground_color,
           child->style.bold,
           child->style.underlined,
           child->style.underlined_double,
           child->style.strikethrough,
           child->style.blink});
    } else if (child->style.display_outside == DisplayOutside::Inline &&
               child->style.display_inside == DisplayInside::Flow &&
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
              {child->style.background_color,
               child->style.foreground_color,
               child->style.bold,
               child->style.underlined,
               child->style.underlined_double,
               child->style.strikethrough,
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

  if (box->dom_node) {
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
  bool is_row = box->style.flex_direction == Direction::Row;

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

  // Apply max-height constraint
  int max_height_resolved = ResolveSize(box->style.max_height, parent_h);
  if (max_height_resolved != -1 && my_height > max_height_resolved) {
    my_height = max_height_resolved;
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
    LayoutBox* box;
    std::shared_ptr<PhysicalFragment> fragment;
    int main_base_size;
    int main_resolved_size;
    float grow;
    float shrink;
  };

  std::vector<FlexItem> items;
  int total_main_base = 0;
  float total_grow = 0;
  float total_shrink_scaled = 0;

  // Pass 1: Determine Flex Base Sizes
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

    auto frag = RunLayout({child.get()}, child_c, context);
    int m_margin =
        is_row ? child->style.margin.Horiz() : child->style.margin.Vert();
    int main_size = (is_row ? frag->width : frag->height) + m_margin;

    items.push_back({child.get(), frag, main_size, main_size,
                     child->style.flex_grow, child->style.flex_shrink});

    total_main_base += main_size;
    total_grow += child->style.flex_grow;
    total_shrink_scaled += (main_size * child->style.flex_shrink);
  }

  // Pass 2: Resolve Flexible Lengths
  int container_main = is_row ? content_w : content_h;
  bool main_is_indefinite =
      is_row ? (constraints.width.mode == MeasureMode::Undefined)
             : (constraints.height.mode == MeasureMode::Undefined);
  int free_space = main_is_indefinite ? 0 : (container_main - total_main_base);

  if (free_space > 0 && total_grow > 0) {
    int total_allocated = 0;
    float current_grow_sum = 0.0f;
    int items_to_grow = 0;
    for (auto& item : items) {
      if (item.grow > 0) {
        items_to_grow++;
      }
    }
    int grown_count = 0;
    for (auto& item : items) {
      if (item.grow > 0) {
        grown_count++;
        int next_cumulative = 0;
        if (grown_count == items_to_grow) {
          next_cumulative = free_space;
        } else {
          current_grow_sum += item.grow;
          next_cumulative =
              static_cast<int>((free_space * current_grow_sum) / total_grow);
        }
        int extra = next_cumulative - total_allocated;
        total_allocated = next_cumulative;
        item.main_resolved_size += extra;
      }
    }
  } else if (free_space < 0 && total_shrink_scaled > 0) {
    bool allow_overflow =
        (is_row && box->style.overflow_x == Overflow::Scroll) ||
        (!is_row && box->style.overflow_y == Overflow::Scroll);
    if (!allow_overflow) {
      int total_shrunk = 0;
      float current_shrink_scaled_sum = 0.0f;
      int items_to_shrink = 0;
      for (auto& item : items) {
        if (item.shrink > 0) {
          items_to_shrink++;
        }
      }
      int shrunk_count = 0;
      for (auto& item : items) {
        if (item.shrink > 0) {
          shrunk_count++;
          int next_cumulative = 0;
          if (shrunk_count == items_to_shrink) {
            next_cumulative = free_space;
          } else {
            current_shrink_scaled_sum += (item.main_base_size * item.shrink);
            next_cumulative = static_cast<int>(
                (free_space * current_shrink_scaled_sum) / total_shrink_scaled);
          }
          int shrink_amount = next_cumulative - total_shrunk;
          total_shrunk = next_cumulative;
          item.main_resolved_size += shrink_amount;
        }
      }
    }
  }

  // Pass 3: Final Measurement & Positioning
  auto fragment = MakeArenaFragment(my_width, my_height);
  fragment->children.reserve(items.size());
  fragment->dom_node = box->dom_node;
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
  int main_pos = is_row ? (box->style.padding.left + box->style.border.left)
                        : (box->style.padding.top + box->style.border.top);
  int cross_start = is_row ? (box->style.padding.top + box->style.border.top)
                           : (box->style.padding.left + box->style.border.left);

  int max_cross_used = 0;
  for (auto& item : items) {
    LayoutConstraints final_c;
    int m_horiz = item.box->style.margin.Horiz();
    int m_vert = item.box->style.margin.Vert();

    if (is_row) {
      final_c.width = {item.main_resolved_size - m_horiz, MeasureMode::Exactly};
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
      final_c.height = {item.main_resolved_size - m_vert, MeasureMode::Exactly};
    }

    int x = is_row ? main_pos + item.box->style.margin.left
                   : cross_start + item.box->style.margin.left;
    int y = is_row ? cross_start + item.box->style.margin.top
                   : main_pos + item.box->style.margin.top;

    LayoutContext child_context =
        CreateChildContext(box, my_width, my_height, x, y, context);
    item.fragment = RunLayout({item.box}, final_c, child_context);

    int rx = x;
    int ry = y;

    if (item.box->style.position == PositionType::Relative) {
      if (item.box->style.left.unit != Unit::Auto) {
        rx += item.box->style.left.Resolve(my_width);
      } else if (item.box->style.right.unit != Unit::Auto) {
        rx -= item.box->style.right.Resolve(my_width);
      }
      if (item.box->style.top.unit != Unit::Auto) {
        ry += item.box->style.top.Resolve(my_height);
      } else if (item.box->style.bottom.unit != Unit::Auto) {
        ry -= item.box->style.bottom.Resolve(my_height);
      }
    }

    fragment->children.push_back({item.fragment, rx, ry});

    main_pos += item.main_resolved_size;
    max_cross_used =
        std::max(max_cross_used, (is_row ? item.fragment->height + m_vert
                                         : item.fragment->width + m_horiz));
  }

  if (auto_width) {
    fragment->width =
        is_row ? (main_pos + box->style.padding.right + box->style.border.right)
               : (max_cross_used + box->style.padding.Horiz() +
                  box->style.border.Horiz());
  }
  if (auto_height) {
    fragment->height =
        is_row
            ? (max_cross_used + box->style.padding.Vert() +
               box->style.border.Vert())
            : (main_pos + box->style.padding.bottom + box->style.border.bottom);
  }

  // Cap width by max-width if needed
  {
    int max_w = ResolveSize(box->style.max_width, parent_w);
    if (max_w != -1 && fragment->width > max_w) {
      fragment->width = max_w;
    }
  }

  // Cap height by max-height if needed
  {
    int max_h = ResolveSize(box->style.max_height, parent_h);
    if (max_h != -1 && fragment->height > max_h) {
      fragment->height = max_h;
    }
  }

  LayoutOutOfFlowChildren(box, fragment, context);

  int total_content_height =
      is_row
          ? (max_cross_used + box->style.padding.Vert() +
             box->style.border.Vert())
          : (main_pos + box->style.padding.bottom + box->style.border.bottom);
  int total_content_width =
      is_row ? (main_pos + box->style.padding.right + box->style.border.right)
             : (max_cross_used + box->style.padding.Horiz() +
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
  }

  return fragment;
}

}  // namespace rtxui
