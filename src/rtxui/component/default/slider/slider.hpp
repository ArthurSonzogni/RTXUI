// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_COMPONENT_DEFAULT_SLIDER_HPP_
#define RTXUI_COMPONENT_DEFAULT_SLIDER_HPP_

#include <string>
#include <string_view>

#include "rtxui/internal/component.hpp"

namespace rtxui {

class slider : public Component<slider> {
 public:
  int value = 0;
  int min = 0;
  int max = 100;
  int step = 1;
  int width = 20;

  // Render bindings
  std::string track_left = "";
  std::string thumb_char = "●";
  std::string track_right = "";

  void InitReflection() override;
  std::string_view Setup() override;
  bool OnEvent(Event event) override;
  bool Digest() override;
};

}  // namespace rtxui

#endif  // RTXUI_COMPONENT_DEFAULT_SLIDER_HPP_
