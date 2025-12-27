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
  static std::shared_ptr<LayoutBox> Build(Element* dom_node);
};
}  // namespace rtxui
#endif  // RTXUI_LAYOUT_LAYOUT_TREE_BUILDER_HPP
