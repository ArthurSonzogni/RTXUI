// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_COMPONENT_DEFAULT_TABS_HPP_
#define RTXUI_COMPONENT_DEFAULT_TABS_HPP_

#include <string>
#include <string_view>
#include <vector>

#include "rtxui/internal/component.hpp"

namespace rtxui {

struct TabPaneInfo {
  std::string name;
  std::string label;
  Element* element;
};

class tabs : public Component<tabs> {
 public:
  std::string value;

  void SelectTab(std::string index_str);

  void InitReflection() override;
  std::string_view Setup() override;
  bool OnEvent(Event event) override;
  bool Digest() override;

 private:
  std::vector<TabPaneInfo> GetTabPanes();

  // Identity of the panes the header buttons were last built from. Rebuild
  // the buttons only when this changes, so a keyboard-focused header button
  // survives an unrelated Digest() instead of losing focus to a freshly
  // recreated Element.
  std::vector<Element*> last_pane_elements_;
};

}  // namespace rtxui

#endif  // RTXUI_COMPONENT_DEFAULT_TABS_HPP_
