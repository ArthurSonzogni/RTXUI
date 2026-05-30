// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_COMPONENT_DEFAULT_INPUT_INPUT_HPP_
#define RTXUI_COMPONENT_DEFAULT_INPUT_INPUT_HPP_

#include <string_view>
#include "rtxui/component/default/input/text_input_base.hpp"
#include "rtxui/internal/component.hpp"

namespace rtxui {

class input : public Component<input>, public TextInputBase {
 public:
  void InitReflection() override;
  std::string_view Setup() override;
  bool OnEvent(Event event) override;
  bool Digest() override;
};

}  // namespace rtxui

#endif  // RTXUI_COMPONENT_DEFAULT_INPUT_INPUT_HPP_
