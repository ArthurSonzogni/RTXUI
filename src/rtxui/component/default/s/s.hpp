// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_COMPONENT_DEFAULT_S_HPP_
#define RTXUI_COMPONENT_DEFAULT_S_HPP_

#include <string_view>

#include "rtxui/internal/component.hpp"

namespace rtxui {

class s : public Component<s> {
 public:
  static const std::string_view view;
};

class strike : public Component<strike> {
 public:
  static const std::string_view view;
};

class del : public Component<del> {
 public:
  static const std::string_view view;
};

}  // namespace rtxui

#endif  // RTXUI_COMPONENT_DEFAULT_S_HPP_
