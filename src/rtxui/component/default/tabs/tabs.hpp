// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_COMPONENT_DEFAULT_TABS_HPP_
#define RTXUI_COMPONENT_DEFAULT_TABS_HPP_

#include <string>
#include <utility>
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
  // The panes the header buttons were last built for, identified by name and
  // label rather than by Element*: the host component re-renders on any state
  // change, which hands out fresh pane elements every time, so comparing
  // addresses reported "the panes changed" on every tab switch.
  std::vector<std::pair<std::string, std::string>> last_pane_ids_;
};

}  // namespace rtxui

#endif  // RTXUI_COMPONENT_DEFAULT_TABS_HPP_
