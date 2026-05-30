// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_COMPONENT_DEFAULT_HR_HPP_
#define RTXUI_COMPONENT_DEFAULT_HR_HPP_

#include <string>
#include <string_view>
#include "rtxui/internal/component.hpp"

namespace rtxui {

class hr : public Component<hr> {
 public:
  std::string line_chars;

  void InitReflection() override;
  std::string_view Setup() override;
  bool Digest() override;
};

}  // namespace rtxui

#endif  // RTXUI_COMPONENT_DEFAULT_HR_HPP_
