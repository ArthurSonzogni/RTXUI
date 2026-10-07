// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_COMPONENT_DEFAULT_TREE_ITEM_HPP_
#define RTXUI_COMPONENT_DEFAULT_TREE_ITEM_HPP_

#include <string>
#include <string_view>

#include "rtxui/component.hpp"

namespace rtxui {

// One node of a tree: a focusable row showing `label`, and the <tree-item>s
// nested inside it, shown indented below while `open`. A node without
// children is a leaf. `open` is two-way bound.
class tree_item : public Component<tree_item> {
 public:
  bool open = false;
  std::string label;
  std::string arrow = " ";
  std::string children_class = "closed";
  // The row's own click handler: toggling a branch, unless the tag carries an
  // onclick of its own, which a click on the row then reaches instead.
  std::string row_action;

  void Toggle();

  void InitReflection() override;
  std::string_view Setup();
  bool OnEvent(Event event) override;
  bool Digest() override;

 private:
  bool HasChildren();
  void SetOpen(bool value);
};

}  // namespace rtxui

#endif  // RTXUI_COMPONENT_DEFAULT_TREE_ITEM_HPP_
