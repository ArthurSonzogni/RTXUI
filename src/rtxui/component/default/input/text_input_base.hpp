// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_COMPONENT_DEFAULT_INPUT_TEXT_INPUT_BASE_HPP_
#define RTXUI_COMPONENT_DEFAULT_INPUT_TEXT_INPUT_BASE_HPP_

#include <string>
#include <string_view>

#include "rtxui/internal/component.hpp"

namespace rtxui {

class TextInputBase {
 public:
  std::string value;
  int cursor_pos = 0;

  // Render bindings
  std::string left_text;
  std::string cursor_char;
  std::string right_text;
  std::string cursor_class = "cursor";

 protected:
  bool is_focused_ = false;
  int ideal_column_ = 0;

  void KeepCursorVisible(Element* root, bool is_multiline);
  bool OnEventShared(ComponentBase* self, Event event, bool is_multiline);
  bool DigestShared(ComponentBase* self);
};

}  // namespace rtxui

#endif  // RTXUI_COMPONENT_DEFAULT_INPUT_TEXT_INPUT_BASE_HPP_
