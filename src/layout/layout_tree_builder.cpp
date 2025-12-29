#include "layout/layout_tree_builder.hpp"

namespace rtxui {

// Static Build method implementation
std::shared_ptr<LayoutBox> LayoutTreeBuilder::Build(Element* dom_node) {
  auto text_node = dynamic_cast<TextElement*>(dom_node);

  auto box = std::make_shared<LayoutBox>();
  box->style = dom_node->style;
  box->dom_node = dom_node;

  // Text nodes don't usually run an algorithm themselves;
  // they are consumed by the parent's InlineFlow.
  if (text_node) {
    box->is_text = true;
    box->text_data = text_node->text();
    box->algorithm = LayoutBox::Algorithm::Text;
    return box;
  }

  std::vector<std::shared_ptr<LayoutBox>> raw_children;
  for (auto& child_dom : dom_node->children()) {
    if (child_dom.get()->is_slot()) {
      // Skip elements with no tag (e.g., SlotElement)
      for (auto& grandchild_dom : child_dom.get()->children()) {
        auto grandchild_box = Build(grandchild_dom.get());
        if (grandchild_box) {
          raw_children.push_back(grandchild_box);
        }
      }
      continue;
    }

    auto child_box = Build(child_dom.get());
    if (child_box) {
      raw_children.push_back(child_box);
    }
  }

  if (raw_children.empty()) {
    // Set default algorithm even for empty boxes to avoid null pointers
    box->algorithm = LayoutBox::Algorithm::BlockFlow;
    return box;
  }

  // --- Algorithm Selection & Tree Refinement ---
  if (box->style.display_inside == DisplayInside::Flex) {
    box->children = raw_children;
    box->algorithm = LayoutBox::Algorithm::Flex;
    return box;
  }

  // CSS: Decide if the Algorithm should be InlineFlow or BlockFlow. This is
  // BlockFlow if it exist at least one element with display_inside = block.
  bool has_block_child = false;
  for (const auto& child_box : raw_children) {
    if (child_box->style.display_outside == DisplayOutside::Block) {
      has_block_child = true;
    }
  }

  if (!has_block_child) {
    box->algorithm = LayoutBox::Algorithm::InlineFlow;
    box->children = raw_children;
    return box;
  }

  // If BlockFlow, we need to wrap Inline children into anonymous Block boxes.
  std::vector<std::shared_ptr<LayoutBox>> refined_children;
  std::shared_ptr<LayoutBox> anonymous_box = nullptr;
  for (const auto& child_box : raw_children) {
    if (child_box->style.display_outside == DisplayOutside::Inline) {
      if (!anonymous_box) {
        anonymous_box = std::make_shared<LayoutBox>();
        anonymous_box->is_anonymous = true;
        anonymous_box->algorithm = LayoutBox::Algorithm::InlineFlow;
        refined_children.push_back(anonymous_box);
      }
      anonymous_box->children.push_back(child_box);
    } else {
      anonymous_box = nullptr;
      refined_children.push_back(child_box);
    }
  }
  box->children = refined_children;
  box->algorithm = LayoutBox::Algorithm::BlockFlow;

  return box;
}

}  // namespace rtxui
