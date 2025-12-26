#include "layout/layout.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <sstream>
#include <vector>

#include "layout/layout_box.hpp"
#include "layout/physical_fragment.hpp"
#include "layout/style.hpp"

namespace rtxui {

std::shared_ptr<PhysicalFragment> LayoutBlockFlow(
    LayoutInputNode node,
    LayoutConstraints constraints);
std::shared_ptr<PhysicalFragment> LayoutInlineFlow(
    LayoutInputNode node,
    LayoutConstraints constraints);
std::shared_ptr<PhysicalFragment> LayoutFlex(LayoutInputNode node,
                                             LayoutConstraints constraints);

// --- Helper: Dimension Resolver ---
int ResolveWidth(const ComputedStyle& style, int parent_width) {
  if (style.width.unit != Unit::Auto) {
    return style.width.Resolve(parent_width);
  }
  return -1;  // Auto
}
int ResolveHeight(const ComputedStyle& style, int parent_height) {
  if (style.height.unit != Unit::Auto) {
    return style.height.Resolve(parent_height);
  }
  return -1;  // Auto
}

// --- D. Dispatcher ---
std::shared_ptr<PhysicalFragment> RunLayout(LayoutInputNode node,
                                            LayoutConstraints constraints) {
  auto* box = node.box;

  if (box->is_text) {
    return nullptr;
  }

  if (box->style.display == Display::Flex) {
    return LayoutFlex(node, constraints);
  }

  // Determine if this box establishes an Inline Formatting Context (IFC)
  if (box->IsInlineFormattingContext()) {
    return LayoutInlineFlow(node, constraints);
  }

  return LayoutBlockFlow(node, constraints);
}

// --- A. Block Layout ---
std::shared_ptr<PhysicalFragment> LayoutBlockFlow(
    LayoutInputNode node,
    LayoutConstraints constraints) {
  auto* box = node.box;
  int avail_width = constraints.width.value;

  int width = 0;
  bool is_auto_width = false;

  if (constraints.width.mode == MeasureMode::Exactly) {
    width = avail_width;
    is_auto_width = false;
  } else {
    int resolved = ResolveWidth(box->style, avail_width);
    if (resolved != -1) {
      width = resolved;
      is_auto_width = false;
    } else {
      is_auto_width = true;
      if (constraints.width.mode == MeasureMode::Undefined) {
        width = 0;
      } else {
        width = std::max(0, avail_width - box->style.margin.Horiz());
      }
    }
  }

  int content_width_limit = 0;
  if (is_auto_width && constraints.width.mode == MeasureMode::Undefined) {
    content_width_limit = 10000; // Arbitrary large for measurement
  } else {
    content_width_limit = std::max(
        0, width - box->style.padding.Horiz() - box->style.border.Horiz());
  }

  auto fragment = std::make_shared<PhysicalFragment>(width, 0, box->debug_name);
  fragment->background_color = box->style.background_color;
  fragment->foreground_color = box->style.foreground_color;
  if (box->style.border.Horiz() > 0) fragment->has_border = true;

  int cur_y = box->style.border.top + box->style.padding.top;
  int cur_x = box->style.border.left + box->style.padding.left;
  int max_child_width = 0;

  for (auto& child_box : box->children) {
    LayoutConstraints child_c;
    child_c.width = {content_width_limit - child_box->style.margin.Horiz(),
                     MeasureMode::AtMost};
    child_c.height = {0, MeasureMode::Undefined};

    auto child_frag = RunLayout({child_box.get()}, child_c);

    fragment->children.push_back({child_frag,
                                  cur_x + child_box->style.margin.left,
                                  cur_y + child_box->style.margin.top});
    cur_y += child_frag->height + child_box->style.margin.Vert();

    max_child_width = std::max(
        max_child_width, child_frag->width + child_box->style.margin.Horiz());
  }

  cur_y += box->style.border.bottom + box->style.padding.bottom;

  if (is_auto_width && constraints.width.mode == MeasureMode::Undefined) {
    fragment->width = max_child_width + box->style.padding.Horiz() + box->style.border.Horiz();
  }

  if (constraints.height.mode == MeasureMode::Exactly) {
    fragment->height = constraints.height.value;
  } else {
    int resolved_h = ResolveHeight(box->style, constraints.height.value);
    fragment->height = (resolved_h != -1) ? resolved_h : cur_y;
  }

  return fragment;
}

// --- B. Inline Layout ---
std::shared_ptr<PhysicalFragment> LayoutInlineFlow(
    LayoutInputNode node,
    LayoutConstraints constraints) {
  auto* box = node.box;
  int avail_width = constraints.width.value;

  int width = 0;
  bool is_fixed_width = false;

  if (constraints.width.mode == MeasureMode::Exactly) {
    width = avail_width;
    is_fixed_width = true;
  } else {
    int resolved = ResolveWidth(box->style, avail_width);
    if (resolved != -1) {
      width = resolved;
      is_fixed_width = true;
    } else {
      width = avail_width;
    }
  }

  int content_width_limit = std::max(
      0, width - box->style.padding.Horiz() - box->style.border.Horiz());

  auto container_frag = std::make_shared<PhysicalFragment>(width, 0, box->debug_name);
  container_frag->background_color = box->style.background_color;
  container_frag->foreground_color = box->style.foreground_color;
  if (box->is_anonymous) container_frag->tag = "IFC Root";

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
      // Standard Text Layout Logic
      size_t start = 0;
      size_t end = child->text_data.size();
      size_t last_space = start;

      auto create_fragment = [&](size_t fit_end) {
        size_t len = fit_end - start;
        if (len == 0 && fit_end != end) return;

        auto text_frag = std::make_shared<PhysicalFragment>(static_cast<int>(len), 1, "Text");
        text_frag->is_text = true;
        text_frag->text_content = child->text_data.substr(start, len);
        text_frag->foreground_color = child->style.foreground_color;

        container_frag->children.push_back({
            text_frag,
            box->style.padding.left + box->style.border.left + cursor_x,
            cursor_y
        });
        cursor_x += static_cast<int>(len);
      };

      for (size_t i = start; i < end; ++i) {
        if (child->text_data[i] == ' ') last_space = i;
        if (cursor_x + static_cast<int>(i - start + 1) > content_width_limit) {
          if (last_space > start) {
            create_fragment(last_space);
            start = last_space + 1;
            i = start - 1;
            commit_line();
          } else if (cursor_x > 0) {
            commit_line();
            i--; 
          } else {
            size_t split = (i == start) ? i + 1 : i;
            create_fragment(split);
            start = split;
            i = start - 1;
            commit_line();
          }
        }
      }
      if (start < end) create_fragment(end);

    } else {
      // Treat non-raw-text children (like RedText elements) as Atomic Inline Boxes.
      // This ensures they stay on the same line if they fit.
      LayoutConstraints child_c = {{content_width_limit, MeasureMode::AtMost},
                                   {0, MeasureMode::Undefined}};
      auto child_frag = RunLayout({child.get()}, child_c);

      // Wrap if this box exceeds the remaining line width
      if (cursor_x + child_frag->width > content_width_limit && cursor_x > 0) {
        commit_line();
      }

      container_frag->children.push_back({
          child_frag,
          box->style.padding.left + box->style.border.left + cursor_x,
          cursor_y
      });

      line_height = std::max(line_height, child_frag->height);
      cursor_x += child_frag->width;
    }
  }

  commit_line();

  container_frag->height = cursor_y + box->style.padding.bottom + box->style.border.bottom;
  if (!is_fixed_width) {
    container_frag->width = max_line_width + box->style.padding.Horiz() + box->style.border.Horiz();
  }

  return container_frag;
}

// --- C. Flex Layout ---
std::shared_ptr<PhysicalFragment> LayoutFlex(LayoutInputNode node,
                                             LayoutConstraints constraints) {
  auto* box = node.box;
  bool is_row = box->style.flex_direction == Direction::Row;

  int parent_w = constraints.width.value;
  int parent_h = constraints.height.value;

  int my_width = (constraints.width.mode == MeasureMode::Exactly) ? parent_w : ResolveWidth(box->style, parent_w);
  int my_height = (constraints.height.mode == MeasureMode::Exactly) ? parent_h : ResolveHeight(box->style, parent_h);
  
  bool auto_width = (my_width == -1);
  bool auto_height = (my_height == -1);

  if (auto_width) my_width = parent_w;
  if (auto_height) my_height = parent_h;

  int content_w = std::max(0, my_width - box->style.border.Horiz() - box->style.padding.Horiz());
  int content_h = std::max(0, my_height - box->style.border.Vert() - box->style.padding.Vert());

  auto fragment = std::make_shared<PhysicalFragment>(my_width, my_height, box->debug_name);
  fragment->background_color = box->style.background_color;
  fragment->foreground_color = box->style.foreground_color;

  struct FlexItem {
    LayoutBox* box;
    std::shared_ptr<PhysicalFragment> fragment;
    int main_size;
    int cross_size;
    float grow;
  };

  std::vector<FlexItem> items;
  int total_main_used = 0;
  int max_cross_used = 0;
  float total_grow = 0;

  for (auto& child : box->children) {
    LayoutConstraints child_c;
    if (is_row) {
      int rw = ResolveWidth(child->style, content_w);
      if (rw != -1) {
        child_c.width = {rw, MeasureMode::Exactly};
      } else {
        child_c.width = {0, MeasureMode::Undefined};
      }
      child_c.height = {content_h, MeasureMode::AtMost};
    } else {
      child_c.width = {content_w, MeasureMode::AtMost};
      int rh = ResolveHeight(child->style, content_h);
      if (rh != -1) {
        child_c.height = {rh, MeasureMode::Exactly};
      } else {
        child_c.height = {0, MeasureMode::Undefined};
      }
    }

    auto child_frag = RunLayout({child.get()}, child_c);
    int main = is_row ? child_frag->width : child_frag->height;
    int cross = is_row ? child_frag->height : child_frag->width;
    int m_main = is_row ? child->style.margin.Horiz() : child->style.margin.Vert();
    int m_cross = is_row ? child->style.margin.Vert() : child->style.margin.Horiz();

    items.push_back({child.get(), child_frag, main + m_main, cross + m_cross, child->style.flex_grow});
    total_main_used += (main + m_main);
    max_cross_used = std::max(max_cross_used, cross + m_cross);
    total_grow += child->style.flex_grow;
  }

  // Distribute Space
  int avail_main = is_row ? content_w : content_h;
  int remaining = avail_main - total_main_used;
  if (remaining > 0 && total_grow > 0) {
    for (auto& item : items) {
      if (item.grow > 0) {
        int extra = static_cast<int>(remaining * (item.grow / total_grow));
        int new_main = (is_row ? item.fragment->width : item.fragment->height) + extra;
        LayoutConstraints gc;
        if (is_row) { gc.width = {new_main, MeasureMode::Exactly}; gc.height = {content_h, MeasureMode::AtMost}; }
        else { gc.width = {content_w, MeasureMode::AtMost}; gc.height = {new_main, MeasureMode::Exactly}; }
        item.fragment = RunLayout({item.box}, gc);
        item.main_size = (is_row ? item.fragment->width : item.fragment->height) + (is_row ? item.box->style.margin.Horiz() : item.box->style.margin.Vert());
      }
    }
  }

  // Positioning
  int m_pos = is_row ? (box->style.padding.left + box->style.border.left) : (box->style.padding.top + box->style.border.top);
  int c_start = is_row ? (box->style.padding.top + box->style.border.top) : (box->style.padding.left + box->style.border.left);

  for (auto& item : items) {
    int x = is_row ? m_pos + item.box->style.margin.left : c_start + item.box->style.margin.left;
    int y = is_row ? c_start + item.box->style.margin.top : m_pos + item.box->style.margin.top;
    fragment->children.push_back({item.fragment, x, y});
    m_pos += item.main_size;
  }

  if (auto_width) fragment->width = is_row ? total_main_used + box->style.padding.Horiz() : max_cross_used + box->style.padding.Horiz();
  if (auto_height) fragment->height = is_row ? max_cross_used + box->style.padding.Vert() : total_main_used + box->style.padding.Vert();

  return fragment;
}

}  // namespace rtxui
