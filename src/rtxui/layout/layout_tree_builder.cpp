#include "rtxui/layout/layout_tree_builder.hpp"
#include "rtxui/layout/layout_arena.hpp"

namespace rtxui {

// Static Build method implementation
std::shared_ptr<LayoutBox> LayoutTreeBuilder::Build(Element* dom_node,
                                                    TextAlign parent_align,
                                                    WhiteSpace parent_ws) {
  if (!dom_node || dom_node->style.display_none) {
    return nullptr;
  }
  auto text_node = dynamic_cast<TextElement*>(dom_node);

  auto box = std::allocate_shared<LayoutBox, LayoutArenaAllocator<LayoutBox>>(
      LayoutArenaAllocator<LayoutBox>());
  box->style = dom_node->style;
  box->dom_node = dom_node;

  TextAlign resolved_align = dom_node->style.text_align.value_or(parent_align);
  box->style.text_align = resolved_align;

  WhiteSpace resolved_ws = dom_node->style.white_space.value_or(parent_ws);
  box->style.white_space = resolved_ws;

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
        auto grandchild_box =
            Build(grandchild_dom.get(), resolved_align, resolved_ws);
        if (grandchild_box) {
          raw_children.push_back(grandchild_box);
        }
      }
      continue;
    }

    auto child_box = Build(child_dom.get(), resolved_align, resolved_ws);
    if (child_box) {
      raw_children.push_back(child_box);
    }
  }

  // --- Algorithm Selection & Tree Refinement ---
  if (dom_node->tag() == "table") {
    box->children = raw_children;
    box->algorithm = LayoutBox::Algorithm::Table;
    box->style.display_outside = DisplayOutside::Block;
    return box;
  }

  if (box->style.display_inside == DisplayInside::Flex) {
    box->children = raw_children;
    box->algorithm = LayoutBox::Algorithm::Flex;
    return box;
  }

  if (box->style.display_outside == DisplayOutside::Block || dom_node->is_slot()) {
    box->algorithm = LayoutBox::Algorithm::BlockFlow;
    std::vector<std::shared_ptr<LayoutBox>> refined_children;
    std::shared_ptr<LayoutBox> anonymous_box = nullptr;
    for (const auto& child_box : raw_children) {
      if (child_box->style.display_outside == DisplayOutside::Inline) {
        if (!anonymous_box) {
          anonymous_box = std::allocate_shared<LayoutBox, LayoutArenaAllocator<LayoutBox>>(
              LayoutArenaAllocator<LayoutBox>());
          anonymous_box->is_anonymous = true;
          anonymous_box->algorithm = LayoutBox::Algorithm::InlineFlow;
          anonymous_box->style.text_align = resolved_align;
          anonymous_box->style.white_space = resolved_ws;
          refined_children.push_back(anonymous_box);
        }
        anonymous_box->children.push_back(child_box);
      } else {
        anonymous_box = nullptr;
        refined_children.push_back(child_box);
      }
    }
    box->children = refined_children;
  } else {  // Inline
    box->algorithm = LayoutBox::Algorithm::InlineFlow;
    box->children = raw_children;
  }

  return box;
}

}  // namespace rtxui
