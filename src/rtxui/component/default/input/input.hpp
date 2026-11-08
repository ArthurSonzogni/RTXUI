// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_COMPONENT_DEFAULT_INPUT_INPUT_HPP_
#define RTXUI_COMPONENT_DEFAULT_INPUT_INPUT_HPP_

#include <string_view>

#include "rtxui/component.hpp"
#include "rtxui/component/default/input/text_input_base.hpp"

namespace rtxui {

class input : public Component<input>, public TextInputBase {
 public:
  input();
  std::string_view Setup();
  bool OnEvent(Event event) override;
  bool Digest() override;

 private:
  // Runs onchange, as a browser fires `change`: on Enter, or once focus
  // leaves, when the user has edited the value since it last ran.
  void Commit();
  bool edited_ = false;
};

}  // namespace rtxui

#endif  // RTXUI_COMPONENT_DEFAULT_INPUT_INPUT_HPP_
