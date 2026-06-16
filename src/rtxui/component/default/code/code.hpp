// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_COMPONENT_DEFAULT_CODE_HPP_
#define RTXUI_COMPONENT_DEFAULT_CODE_HPP_

#include <string_view>

#include "rtxui/internal/component.hpp"

namespace rtxui {

class code : public Component<code> {
 public:
  static const std::string_view view;
};

}  // namespace rtxui

#endif  // RTXUI_COMPONENT_DEFAULT_CODE_HPP_
