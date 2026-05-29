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

namespace rtxui {

// --- Forward Declarations ---
std::shared_ptr<PhysicalFragment> LayoutBlockFlow(
    LayoutInputNode node,
    LayoutConstraints constraints);
std::shared_ptr<PhysicalFragment> LayoutInlineFlow(
    LayoutInputNode node,
    LayoutConstraints constraints);
std::shared_ptr<PhysicalFragment> LayoutFlex(LayoutInputNode node,
                                             LayoutConstraints constraints);

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

// --- Dispatcher ---
std::shared_ptr<PhysicalFragment> RunLayout(LayoutInputNode node,
                                            LayoutConstraints constraints) {
  auto* box = node.box;
  if (!box) {
    throw std::runtime_error("LayoutInputNode has null LayoutBox.");
  }

  switch (node.box->algorithm) {
    case LayoutBox::Algorithm::BlockFlow:
      return LayoutBlockFlow(node, constraints);
    case LayoutBox::Algorithm::InlineFlow:
      return LayoutInlineFlow(node, constraints);
    case LayoutBox::Algorithm::Flex:
      return LayoutFlex(node, constraints);
    case LayoutBox::Algorithm::Text:
      throw std::runtime_error(
          "Text nodes should not be laid out directly. Use InlineFlow.");
  }
  return nullptr;
}

// --- Block Layout ---
std::shared_ptr<PhysicalFragment> LayoutBlockFlow(
    LayoutInputNode node,
    LayoutConstraints constraints) {
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

  auto fragment = std::make_shared<PhysicalFragment>(width, 0);
  fragment->dom_node = box->dom_node;
  fragment->background_color = box->style.background_color;
  fragment->foreground_color = box->style.foreground_color;
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

  for (auto& child_box : box->children) {
    LayoutConstraints child_c;
    child_c.width = {
        child_width_limit - child_box->style.margin.Horiz(),
        MeasureMode::AtMost,
    };
    child_c.height = {
        0,
        MeasureMode::Undefined,
    };

    auto child_frag = RunLayout({child_box.get()}, child_c);

    int margin_top = child_box->style.margin.top;
    int margin_bottom = child_box->style.margin.bottom;

    // Sibling margin collapse: use the maximum of the previous child's bottom
    // margin and the current child's top margin.
    int collapsed_margin = is_first_child ? margin_top : std::max(prev_margin_bottom, margin_top);

    fragment->children.push_back({
        child_frag,
        cur_x + child_box->style.margin.left,
        cur_y + collapsed_margin,
    });

    // Advance cur_y to the bottom of the current fragment content.
    cur_y += collapsed_margin + child_frag->height;
    prev_margin_bottom = margin_bottom;
    is_first_child = false;

    max_child_width = std::max(
        max_child_width, child_frag->width + child_box->style.margin.Horiz());
  }

  // Final height includes the bottom margin of the last child and container padding/border.
  cur_y += prev_margin_bottom;
  cur_y += box->style.border.bottom + box->style.padding.bottom;

  if (is_auto_width && constraints.width.mode == MeasureMode::Undefined) {
    fragment->width = max_child_width + box->style.padding.Horiz() +
                      box->style.border.Horiz();
  }

  if (constraints.height.mode == MeasureMode::Exactly) {
    fragment->height = constraints.height.value;
  } else {
    int resolved_h = ResolveSize(box->style.height, constraints.height.value);
    fragment->height = (resolved_h != -1) ? resolved_h : cur_y;
  }

  if (box->style.overflow_y != Overflow::Visible || box->style.overflow_x != Overflow::Visible) {
    fragment->clips_descendants = true;
  }

  if (box->dom_node) {
    box->dom_node->set_scroll_height(cur_y);
    int max_scroll = std::max(0, cur_y - fragment->height);
    if (box->dom_node->scroll_y() > max_scroll) {
      box->dom_node->set_scroll_y(max_scroll);
    }
    fragment->scroll_y = box->dom_node->scroll_y();

    int total_content_width = max_child_width + box->style.padding.Horiz() + box->style.border.Horiz();
    box->dom_node->set_scroll_width(total_content_width);
    int max_scroll_x = std::max(0, total_content_width - fragment->width);
    if (box->dom_node->scroll_x() > max_scroll_x) {
      box->dom_node->set_scroll_x(max_scroll_x);
    }
    fragment->scroll_x = box->dom_node->scroll_x();
  }

  return fragment;
}

// --- Inline Layout ---
std::shared_ptr<PhysicalFragment> LayoutInlineFlow(
    LayoutInputNode node,
    LayoutConstraints constraints) {
  auto* box = node.box;
  int avail_width = constraints.width.value;

  int width = (constraints.width.mode == MeasureMode::Exactly)
                  ? avail_width
                  : ResolveSize(box->style.width, avail_width);
  bool is_fixed_width = (width != -1);
  if (!is_fixed_width) {
    width = avail_width;
  }

  int content_width_limit = std::max(
      0, width - box->style.padding.Horiz() - box->style.border.Horiz());
  auto container_frag = std::make_shared<PhysicalFragment>(width, 0);
  container_frag->dom_node = box->dom_node;
  container_frag->background_color = box->style.background_color;
  container_frag->foreground_color = box->style.foreground_color;
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

  auto commit_line = [&]() {
    max_line_width = std::max(max_line_width, cursor_x);
    cursor_x = 0;
    cursor_y += line_height;
    line_height = 1;
  };

  for (auto& child : box->children) {
    if (child->is_text) {
      // Iterate grapheme-by-grapheme to correctly handle double-width
      // characters (CJK, emoji). We track byte offsets for slicing and
      // column widths for layout/wrap decisions separately.

      const std::string& text = child->text_data;
      size_t byte_start = 0;       // byte offset of current fragment start
      int col_start = 0;           // column offset of current fragment start
      size_t last_space_byte = 0;  // byte offset of last seen space
      int last_space_col = 0;      // column offset of last seen space
      bool have_last_space = false;

      // create_fragment: emit a fragment from byte_start..byte_end of width
      // col_start..col_end_exclusive.
      auto create_fragment = [&](size_t byte_end, int col_width) {
        if (byte_end == byte_start && col_width == 0 &&
            byte_end != text.size()) {
          return;
        }
        auto text_frag =
            std::make_shared<PhysicalFragment>(col_width, 1);
        text_frag->dom_node = child->dom_node;
        text_frag->is_text = true;
        text_frag->text_content = text.substr(byte_start, byte_end - byte_start);
        text_frag->foreground_color = child->style.foreground_color;

        container_frag->children.push_back(
            {text_frag,
             box->style.padding.left + box->style.border.left + cursor_x,
             cursor_y});
        cursor_x += col_width;
      };

      int cur_col = col_start;   // current column position within this text
      for (const Grapheme& g : Graphemes(text)) {
        size_t byte_end = static_cast<size_t>(g.text.data() + g.text.size() - text.data());

        if (g.text.size() == 1 && g.text[0] == ' ') {
          last_space_byte = static_cast<size_t>(g.text.data() - text.data());
          last_space_col = cur_col - col_start;
          have_last_space = true;
        }

        if (cursor_x + (cur_col - col_start) + g.width > content_width_limit) {
          if (have_last_space) {
            // Wrap at last space
            int frag_cols = last_space_col;
            size_t frag_byte_end = last_space_byte;
            create_fragment(frag_byte_end, frag_cols);

            // skip the space byte
            byte_start = last_space_byte + 1;
            col_start = last_space_col + 1;  // space is 1 col wide
            cur_col = col_start;
            have_last_space = false;
            commit_line();
            // Retry the current grapheme
            byte_end = static_cast<size_t>(g.text.data() + g.text.size() - text.data());
            cur_col += g.width;
          } else if (cursor_x > 0) {
            commit_line();
            // cur position unchanged, retry grapheme
            cur_col += g.width;
          } else {
            // Single grapheme fills the line
            cur_col += g.width;
            size_t next_byte = static_cast<size_t>(g.text.data() + g.text.size() - text.data());
            create_fragment(next_byte, cur_col - col_start);
            byte_start = next_byte;
            col_start = cur_col;
            commit_line();
          }
        } else {
          cur_col += g.width;
        }
      }

      // Emit any remaining text
      if (byte_start < text.size()) {
        create_fragment(text.size(), cur_col - col_start);
      }
    } else {
      // For elements in inline flow, we must respect their margins.
      int m_left = child->style.margin.left;
      int m_right = child->style.margin.right;
      int m_top = child->style.margin.top;
      int m_bottom = child->style.margin.bottom;
      int child_m_horiz = m_left + m_right;
      int child_m_vert = m_top + m_bottom;

      LayoutConstraints child_c = {{content_width_limit - child_m_horiz, MeasureMode::AtMost},
                                   {0, MeasureMode::Undefined}};
      auto child_frag = RunLayout({child.get()}, child_c);

      // If the child (plus its horizontal margins) overflows the current line, wrap.
      if (cursor_x + child_frag->width + child_m_horiz > content_width_limit && cursor_x > 0) {
        commit_line();
      }

      container_frag->children.push_back(
          {child_frag,
           box->style.padding.left + box->style.border.left + cursor_x + m_left,
           cursor_y + m_top});

      // Inline boxes affect the line height based on their content + vertical margins.
      line_height = std::max(line_height, child_frag->height + child_m_vert);
      cursor_x += child_frag->width + child_m_horiz;
    }
  }

  commit_line();
  container_frag->height =
      cursor_y + box->style.padding.bottom + box->style.border.bottom;
  if (!is_fixed_width) {
    container_frag->width =
        max_line_width + box->style.padding.Horiz() + box->style.border.Horiz();
  }

  return container_frag;
}

/**
 * LayoutFlex: A W3C-aligned Flexbox implementation for TUI.
 * Handles flex-direction, weighted flex-shrink, flex-grow, and basis.
 */
std::shared_ptr<PhysicalFragment> LayoutFlex(LayoutInputNode node,
                                             LayoutConstraints constraints) {
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

  int content_w = std::max(
      0, my_width - box->style.border.Horiz() - box->style.padding.Horiz() - scrollbar_spacing_x);
  int content_h = std::max(
      0, my_height - box->style.border.Vert() - box->style.padding.Vert() - scrollbar_spacing_y);

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
    int basis = is_row ? ResolveSize(child->style.width, content_w)
                       : ResolveSize(child->style.height, content_h);

    LayoutConstraints child_c;
    if (is_row) {
      int max_h = (box->style.overflow_y == Overflow::Scroll) ? 10000 : content_h;
      child_c.width = {basis != -1 ? basis : 0, basis != -1
                                                    ? MeasureMode::Exactly
                                                    : MeasureMode::Undefined};
      child_c.height = {max_h, MeasureMode::AtMost};
    } else {
      int max_w = (box->style.overflow_x == Overflow::Scroll) ? 10000 : content_w;
      child_c.width = {max_w, MeasureMode::AtMost};
      child_c.height = {basis != -1 ? basis : 0, basis != -1
                                                       ? MeasureMode::Exactly
                                                       : MeasureMode::Undefined};
    }

    auto frag = RunLayout({child.get()}, child_c);
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
  int free_space = container_main - total_main_base;

  if (free_space > 0 && total_grow > 0) {
    for (auto& item : items) {
      if (item.grow > 0) {
        int extra = (free_space * item.grow) / total_grow;
        item.main_resolved_size += extra;
      }
    }
  } else if (free_space < 0 && total_shrink_scaled > 0) {
    bool allow_overflow = (is_row && box->style.overflow_x == Overflow::Scroll) ||
                          (!is_row && box->style.overflow_y == Overflow::Scroll);
    if (!allow_overflow) {
      for (auto& item : items) {
        if (item.shrink > 0) {
          float shrink_factor =
              (item.main_base_size * item.shrink) / total_shrink_scaled;
          item.main_resolved_size += static_cast<int>(free_space * shrink_factor);
        }
      }
    }
  }

  // Pass 3: Final Measurement & Positioning
  auto fragment = std::make_shared<PhysicalFragment>(my_width, my_height);
  fragment->dom_node = box->dom_node;
  fragment->background_color = box->style.background_color;
  fragment->foreground_color = box->style.foreground_color;
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
      int max_h = (box->style.overflow_y == Overflow::Scroll) ? 10000 : content_h;
      final_c.width = {item.main_resolved_size - m_horiz, MeasureMode::Exactly};
      final_c.height = {max_h, MeasureMode::AtMost};
    } else {
      int max_w = (box->style.overflow_x == Overflow::Scroll) ? 10000 : content_w;
      final_c.width = {max_w, MeasureMode::AtMost};
      final_c.height = {item.main_resolved_size - m_vert, MeasureMode::Exactly};
    }

    item.fragment = RunLayout({item.box}, final_c);

    int x = is_row ? main_pos + item.box->style.margin.left
                   : cross_start + item.box->style.margin.left;
    int y = is_row ? cross_start + item.box->style.margin.top
                   : main_pos + item.box->style.margin.top;

    fragment->children.push_back({item.fragment, x, y});

    main_pos += item.main_resolved_size;
    max_cross_used =
        std::max(max_cross_used, (is_row ? item.fragment->height + m_vert
                                         : item.fragment->width + m_horiz));
  }

  if (auto_width)
    fragment->width =
        is_row ? (main_pos + box->style.padding.right + box->style.border.right)
               : (max_cross_used + box->style.padding.Horiz() +
                  box->style.border.Horiz());
  if (auto_height)
    fragment->height =
        is_row
            ? (max_cross_used + box->style.padding.Vert() +
               box->style.border.Vert())
            : (main_pos + box->style.padding.bottom + box->style.border.bottom);

  int total_content_height = is_row ? (max_cross_used + box->style.padding.Vert() + box->style.border.Vert())
                                    : (main_pos + box->style.padding.bottom + box->style.border.bottom);
  int total_content_width = is_row ? (main_pos + box->style.padding.right + box->style.border.right)
                                   : (max_cross_used + box->style.padding.Horiz() + box->style.border.Horiz());

  if (box->style.overflow_y != Overflow::Visible || box->style.overflow_x != Overflow::Visible) {
    fragment->clips_descendants = true;
  }

  if (box->dom_node) {
    box->dom_node->set_scroll_height(total_content_height);
    int max_scroll = std::max(0, total_content_height - fragment->height);
    if (box->dom_node->scroll_y() > max_scroll) {
      box->dom_node->set_scroll_y(max_scroll);
    }
    fragment->scroll_y = box->dom_node->scroll_y();

    box->dom_node->set_scroll_width(total_content_width);
    int max_scroll_x = std::max(0, total_content_width - fragment->width);
    if (box->dom_node->scroll_x() > max_scroll_x) {
      box->dom_node->set_scroll_x(max_scroll_x);
    }
    fragment->scroll_x = box->dom_node->scroll_x();
  }

  return fragment;
}

}  // namespace rtxui
