// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_COMPONENT_DEFAULT_DETAILS_HPP_
#define RTXUI_COMPONENT_DEFAULT_DETAILS_HPP_

#include <string>
#include <string_view>

#include "rtxui/internal/component.hpp"

namespace rtxui {

class details : public Component<details> {
 public:
  bool open = false;
  std::string arrow_char = "▶";
  std::string content_class = "closed";
  bool no_summary = true;

  void Toggle();

  void InitReflection() override;
  std::string_view Setup() override;
  bool OnEvent(Event event) override;
  bool Digest() override;
};

}  // namespace rtxui

#endif  // RTXUI_COMPONENT_DEFAULT_DETAILS_HPP_
