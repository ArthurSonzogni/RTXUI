#ifndef RTXUI_LAYOUT_LAYOUT_TREE_BUILDER_HPP
#define RTXUI_LAYOUT_LAYOUT_TREE_BUILDER_HPP

#include <memory>
#include <vector>

#include "dom/element.hpp"
#include "dom/text_element.hpp"
#include "layout/layout_box.hpp"

namespace rtxui {
class LayoutTreeBuilder {
 public:
  static std::shared_ptr<LayoutBox> Build(Element* dom_node) {
    auto text_node = dynamic_cast<TextElement*>(dom_node);
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

    // Flex containers blockify children (simple pass-through for now)
    if (box->style.display == Display::Flex) {
      box->children = raw_children;
      return box;
    }

    // Standard Block Flow: Fix mixed content
    std::shared_ptr<LayoutBox> current_anonymous_block = nullptr;

    for (auto& child : raw_children) {
      if (child->style.IsInlineLevel()) {
        if (current_anonymous_block == nullptr) {
          current_anonymous_block =
              std::make_shared<LayoutBox>("AnonymousBlock");
          current_anonymous_block->style.display = Display::Block;
          current_anonymous_block->is_anonymous = true;
          box->children.push_back(current_anonymous_block);
        }
        current_anonymous_block->children.push_back(child);
      } else {
        current_anonymous_block = nullptr;
        box->children.push_back(child);
      }
    }

    return box;
  }
};
}  // namespace rtxui
#endif  // RTXUI_LAYOUT_LAYOUT_TREE_BUILDER_HPP
