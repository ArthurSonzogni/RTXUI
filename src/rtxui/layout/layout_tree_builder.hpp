#ifndef RTXUI_LAYOUT_LAYOUT_TREE_BUILDER_HPP
#define RTXUI_LAYOUT_LAYOUT_TREE_BUILDER_HPP

#include <memory>
#include <vector>

#include "rtxui/dom/element.hpp"
#include "rtxui/dom/text_element.hpp"
#include "rtxui/layout/layout_box.hpp"

namespace rtxui {
class LayoutTreeBuilder {
 public:
  static std::shared_ptr<LayoutBox> Build(Element* dom_node, TextAlign parent_align = TextAlign::Left);
};
}  // namespace rtxui
#endif  // RTXUI_LAYOUT_LAYOUT_TREE_BUILDER_HPP
