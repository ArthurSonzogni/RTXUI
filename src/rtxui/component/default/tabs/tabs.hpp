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
  std::vector<Element*> HeaderButtons();

  // One entry per pane, in pane order. The header strip is a <for> over this
  // in the template rather than Elements built by hand in Digest(), so
  // reconciliation owns the buttons' identity, styling and focus -- all three
  // of which this component previously got wrong on its own.
  struct TabHeader {
    std::string label;
    // Empty, or "active-tab". Interpolated into the button's class list.
    std::string active_class;

    // Change detection for the bound collection compares elements.
    bool operator==(const TabHeader&) const = default;
  };
  std::vector<TabHeader> headers_;
};

}  // namespace rtxui

#endif  // RTXUI_COMPONENT_DEFAULT_TABS_HPP_
