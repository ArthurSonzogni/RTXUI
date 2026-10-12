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
  // `may_defer`: whether the parent lays out this box through RunLayout, which
  // lets Build() leave its children for RunLayout to build, if it needs them.
  static std::shared_ptr<LayoutBox> Build(Element* dom_node,
                                          InheritedTextStyle parent = {},
                                          bool may_defer = false);
  // Builds the children Build() left out of `box` (children_deferred).
  static void BuildChildren(LayoutBox& box);
};
}  // namespace rtxui
#endif  // RTXUI_LAYOUT_LAYOUT_TREE_BUILDER_HPP
