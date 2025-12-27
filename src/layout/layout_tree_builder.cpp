#include "layout/layout_tree_builder.hpp"

namespace rtxui {
// Static Build method implementation
std::shared_ptr<LayoutBox> LayoutTreeBuilder::Build(Element* dom_node) {
  auto text_node = dynamic_cast<TextElement*>(dom_node);
  
  // Fix: dom_node doesn't have tag_name(), we'll use a placeholder or derived name
  auto box = std::make_shared<LayoutBox>(text_node ? "Text" : "Box");
  box->style = dom_node->style;
  box->dom_node = dom_node;

  if (text_node) {
    box->is_text = true;
    box->text_data = text_node->text();
    return box;
  }

  std::vector<std::shared_ptr<LayoutBox>> raw_children;
  for (auto& child_dom : dom_node->children()) {
    auto child_box = Build(child_dom.get());
    if (child_box) {
      raw_children.push_back(child_box);
    }
  }

  if (raw_children.empty()) {
    return box;
  }

    // Flex containers do not group children into anonymous blocks;
    // children are individual flex items.
    if (box->style.display_inside == DisplayInside::Flex) {
      box->children = raw_children;
      return box;
    }
  
    // Standard Block Flow: Fix mixed content.
    // CSS Requirement: If a block container has both block-level and inline-level
    // children, the inline-level children must be wrapped in anonymous block boxes.
    std::shared_ptr<LayoutBox> current_anonymous_block = nullptr;
  
    for (auto& child : raw_children) {
      if (child->style.IsInlineLevel() || child->is_text) {
        if (current_anonymous_block == nullptr) {
          current_anonymous_block = std::make_shared<LayoutBox>("AnonymousBlock");
          current_anonymous_block->style.display_outside = DisplayOutside::Block;
          current_anonymous_block->is_anonymous = true;
        if (!current_anonymous_block->style.foreground_color.has_value()) {
          current_anonymous_block->style.foreground_color = box->style.foreground_color;
        }

        box->children.push_back(current_anonymous_block);
      }
      current_anonymous_block->children.push_back(child);
    } else {
      // Block-level child: break the current anonymous block context
      current_anonymous_block = nullptr;
      box->children.push_back(child);
    }
  }

  return box;
}
}  // namespace rtxui
