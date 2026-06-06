// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_COMPONENT_DEFAULT_SELECT_HPP_
#define RTXUI_COMPONENT_DEFAULT_SELECT_HPP_

#include <string>
#include <string_view>
#include <vector>

#include "rtxui/internal/component.hpp"

namespace rtxui {

struct OptionInfo {
  std::string value;
  std::string label;
  Element* element = nullptr;
};

class select : public Component<select> {
 public:
  std::string value;
  bool is_open = false;
  int hovered_index = -1;

  // Render bindings
  std::string selected_label = "Select...";
  std::string arrow_char = "▾";
  std::string dropdown_class = "closed";

  void InitReflection() override;
  std::string_view Setup() override;
  bool OnEvent(Event event) override;
  bool Digest() override;

  void SelectOption(std::string_view value);
  std::vector<OptionInfo> GetOptions();
};

}  // namespace rtxui

#endif  // RTXUI_COMPONENT_DEFAULT_SELECT_HPP_
