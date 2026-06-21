// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_COMPONENT_DEFAULT_SUMMARY_HPP_
#define RTXUI_COMPONENT_DEFAULT_SUMMARY_HPP_

#include <string_view>

#include "rtxui/internal/component.hpp"

namespace rtxui {

class summary : public Component<summary> {
 public:
  static const std::string_view view;
};

}  // namespace rtxui

#endif  // RTXUI_COMPONENT_DEFAULT_SUMMARY_HPP_
