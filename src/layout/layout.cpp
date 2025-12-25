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

  // FIX: Respect Exactly constraints (forcing size), override style resolution
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
      // Standard Block expansion or Shrink fit init
      if (constraints.width.mode == MeasureMode::Undefined) {
        width = 0;
      } else {
        width = std::max(0, avail_width - box->style.margin.Horiz());
      }
    }
  }

  // Determine content width for children constraint
  int content_width_limit = 0;

  if (is_auto_width && constraints.width.mode == MeasureMode::Undefined) {
    // Shrink-to-fit mode: give children infinite space to find max content
    content_width_limit = 10000;
  } else {
    content_width_limit = std::max(
        0, width - box->style.padding.Horiz() - box->style.border.Horiz());
  }

  auto fragment = std::make_shared<PhysicalFragment>(width, 0, box->debug_name);
  fragment->background_color = box->style.background_color;
  fragment->foreground_color = box->style.foreground_color;
  if (box->style.border.Horiz() > 0) {
    fragment->has_border = true;
  }

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

  // Finalize Width if Shrink-to-Fit
  if (is_auto_width && constraints.width.mode == MeasureMode::Undefined) {
    fragment->width = max_child_width + box->style.padding.Horiz() +
                      box->style.border.Horiz();
  }

  // Resolve Height (similar logic for Exactly)
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

  auto container_frag =
      std::make_shared<PhysicalFragment>(width, 0, box->debug_name);
  container_frag->background_color = box->style.background_color;
  container_frag->foreground_color = box->style.foreground_color;
  if (box->is_anonymous) {
    container_frag->tag = "IFC Root";
  }

  int cursor_x = 0;
  int cursor_y = box->style.padding.top + box->style.border.top;
  int line_height = 1;
  int max_line_width = 0;

  auto commit_line = [&](int& x, int& y, int& lh) {
    if (x > max_line_width) {
      max_line_width = x;
    }
    x = 0;
    y += lh;
    lh = 1;
  };

  for (auto& child : box->children) {
    // 'child' is a shared_ptr<Box>, so we use arrow operator directly
    
    if (child->is_text) {
      size_t start = 0;
      size_t end = child->text_data.size();
      size_t last_space = start;
      
      auto create_fragment = [&](size_t fit_end) {
        size_t len = fit_end - start;
        // Avoid creating empty fragments if possible, though valid logic might allow them
        if (len == 0 && fit_end != end) return; 

        auto text_frag =
            std::make_shared<PhysicalFragment>(static_cast<int>(len), 1, "Text");
        text_frag->is_text = true;
        text_frag->text_content = child->text_data.substr(start, len);
        text_frag->background_color = child->style.background_color;
        text_frag->foreground_color = child->style.foreground_color;

        container_frag->children.push_back({
            text_frag,
            box->style.padding.left + box->style.border.left + cursor_x,
            cursor_y,
        });
        cursor_x += static_cast<int>(len);
      };

      for(size_t i = start; i < end; ++i) {
        if (child->text_data[i] == ' ') {
          last_space = i;
        }
        
        // Check if adding the current character exceeds limit
        // Using simplistic 1 char = 1 px width logic
        size_t len = i - start + 1;
        
        if (cursor_x + static_cast<int>(len) > content_width_limit) {
          if (last_space > start) {
            // Case 1: Wrap at the last known space
            create_fragment(last_space);
            start = last_space + 1;
            i = start - 1; 
            commit_line(cursor_x, cursor_y, line_height);
          } else if (cursor_x > 0) {
            // Case 2: Mid-word break (no space found on this line yet).
            // Since we are not at the start of the line (cursor_x > 0),
            // we can move the entire pending word to the next line.
            commit_line(cursor_x, cursor_y, line_height);
            i--; // Retry the current character on the new line.
          } else {
            // Case 3: Force split.
            // We are at the start of the line (cursor_x == 0) and the word is 
            // still too long to fit. We must split it here to avoid infinite loops
            // or ignored overflows.
            size_t split_index = i;
            if (i == start) {
                // If a single character is too wide, we must include it anyway 
                // to make progress, otherwise we loop forever.
                split_index = i + 1;
            }
            create_fragment(split_index);
            start = split_index;
            i = start - 1;
            commit_line(cursor_x, cursor_y, line_height);
          }
        }
      }
      
      // Create fragment for remaining text
      if (start < end) {
        create_fragment(end);
      } 

    } else {
      // Handle non-text children (Block logic)
      LayoutConstraints child_c = {{content_width_limit, MeasureMode::AtMost},
                                   {0, MeasureMode::Undefined}};
      
      // Construct LayoutInputNode strictly for the function call
      auto child_frag = RunLayout({child.get()}, child_c);

      if (cursor_x + child_frag->width > content_width_limit && cursor_x > 0) {
        commit_line(cursor_x, cursor_y, line_height);
      }
      
      container_frag->children.push_back(
          {child_frag,
           box->style.padding.left + box->style.border.left + cursor_x,
           cursor_y});
           
      line_height = std::max(line_height, child_frag->height);
      cursor_x += child_frag->width;
    }
  }

  // FIX: Removed incorrect cursor_x-- 
  commit_line(cursor_x, cursor_y, line_height);

  container_frag->height =
      cursor_y + box->style.padding.bottom + box->style.border.bottom;

  if (!is_fixed_width) {
    container_frag->width =
        max_line_width + box->style.padding.Horiz() + box->style.border.Horiz();
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

  int my_width = 0;
  int my_height = 0;
  bool auto_width = false;
  bool auto_height = false;

  // FIX: Respect Exactly constraints from parent
  if (constraints.width.mode == MeasureMode::Exactly) {
    my_width = parent_w;
  } else {
    int resolved = ResolveWidth(box->style, parent_w);
    if (resolved != -1) {
      my_width = resolved;
    } else {
      my_width = parent_w;
      auto_width = true;
    }  // Default to fill, shrink later
  }

  if (constraints.height.mode == MeasureMode::Exactly) {
    my_height = parent_h;
  } else {
    int resolved = ResolveHeight(box->style, parent_h);
    if (resolved != -1) {
      my_height = resolved;
    } else {
      my_height = parent_h;
      auto_height = true;
    }
  }

  int content_w = std::max(
      0, my_width - box->style.border.Horiz() - box->style.padding.Horiz());
  int content_h = (my_height > 0)
                      ? std::max(0, my_height - box->style.border.Vert() -
                                        box->style.padding.Vert())
                      : 0;

  auto fragment =
      std::make_shared<PhysicalFragment>(my_width, my_height, box->debug_name);
  fragment->background_color = box->style.background_color;
  fragment->foreground_color = box->style.foreground_color;
  if (box->style.border.Horiz() > 0) {
    fragment->has_border = true;
  }

  // 1. Measure Children
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
      // Resolve child width against content_w to handle percentages correctly
      int resolved_child_w = ResolveWidth(child->style, content_w);

      if (resolved_child_w != -1) {
        // Explicit size (Fixed or Percent) -> Exact Constraint
        child_c.width = {resolved_child_w, MeasureMode::Exactly};
      } else {
        // Auto size -> Undefined (Implies "Measure Content / Shrink Wrap")
        child_c.width = {0, MeasureMode::Undefined};
      }
      child_c.height = {content_h, MeasureMode::AtMost};
    } else {
      child_c.width = {content_w, MeasureMode::AtMost};
      child_c.height = {0, MeasureMode::Undefined};
    }

    auto child_frag = RunLayout({child.get()}, child_c);

    int main = is_row ? child_frag->width : child_frag->height;
    int cross = is_row ? child_frag->height : child_frag->width;

    int margin_main =
        is_row ? child->style.margin.Horiz() : child->style.margin.Vert();
    int margin_cross =
        is_row ? child->style.margin.Vert() : child->style.margin.Horiz();

    items.push_back({child.get(), child_frag, main + margin_main,
                     cross + margin_cross, child->style.flex_grow});
    total_main_used += (main + margin_main);
    max_cross_used = std::max(max_cross_used, cross + margin_cross);
    total_grow += child->style.flex_grow;
  }

  // 2. Distribute Free Space
  int available_main = is_row ? content_w : content_h;
  int remaining = available_main - total_main_used;

  if (remaining > 0 && total_grow > 0) {
    for (auto& item : items) {
      if (item.grow > 0) {
        int extra = static_cast<int>(remaining * (item.grow / total_grow));

        LayoutConstraints grow_c;
        // Calculate new TARGET BOX size
        int new_main_inner =
            (is_row ? item.fragment->width : item.fragment->height) + extra;

        if (is_row) {
          grow_c.width = {new_main_inner, MeasureMode::Exactly};
          grow_c.height = {content_h, MeasureMode::AtMost};
        } else {
          grow_c.width = {content_w, MeasureMode::AtMost};
          grow_c.height = {new_main_inner, MeasureMode::Exactly};
        }

        auto new_frag = RunLayout({item.box}, grow_c);
        item.fragment = new_frag;

        int margin_main = is_row ? item.box->style.margin.Horiz()
                                 : item.box->style.margin.Vert();
        int margin_cross = is_row ? item.box->style.margin.Vert()
                                  : item.box->style.margin.Horiz();

        item.main_size =
            (is_row ? new_frag->width : new_frag->height) + margin_main;
        item.cross_size =
            (is_row ? new_frag->height : new_frag->width) + margin_cross;
        max_cross_used = std::max(max_cross_used, item.cross_size);
      }
    }
  }

  // 3. Position
  int main_pos = is_row ? (box->style.padding.left + box->style.border.left)
                        : (box->style.padding.top + box->style.border.top);
  int cross_start = is_row ? (box->style.padding.top + box->style.border.top)
                           : (box->style.padding.left + box->style.border.left);

  for (auto& item : items) {
    int x, y;
    if (is_row) {
      x = main_pos + item.box->style.margin.left;
      y = cross_start + item.box->style.margin.top;
    } else {
      x = cross_start + item.box->style.margin.left;
      y = main_pos + item.box->style.margin.top;
    }

    fragment->children.push_back({item.fragment, x, y});
    main_pos += item.main_size;
  }

  // 4. Finalize Container Size (Shrink Wrap if Auto)
  if (auto_width) {
    fragment->width = is_row ? (total_main_used + box->style.padding.Horiz() +
                                box->style.border.Horiz())
                             : (max_cross_used + box->style.padding.Horiz() +
                                box->style.border.Horiz());
    // Expand to fill if Grow used space (fill constraint)
    if (remaining > 0 && total_grow > 0) {
      fragment->width = std::max(fragment->width, parent_w);
    }
  }

  if (auto_height) {
    fragment->height = is_row ? (max_cross_used + box->style.padding.Vert() +
                                 box->style.border.Vert())
                              : (total_main_used + box->style.padding.Vert() +
                                 box->style.border.Vert());
    if (remaining > 0 && total_grow > 0) {
      fragment->height = std::max(fragment->height, parent_h);
    }
  }

  return fragment;
}

}  // namespace rtxui
