// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_COMPONENT_DEFAULT_A_HPP_
#define RTXUI_COMPONENT_DEFAULT_A_HPP_

#include <string_view>

#include "rtxui/component.hpp"

namespace rtxui {

class a : public Component<a> {
 public:
  a();
  ~a();
  a(const a&) = delete;
  a& operator=(const a&) = delete;
  static const std::string_view view;
};

}  // namespace rtxui

#endif  // RTXUI_COMPONENT_DEFAULT_A_HPP_
