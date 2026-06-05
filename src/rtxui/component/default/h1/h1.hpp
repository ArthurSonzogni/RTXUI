// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_COMPONENT_DEFAULT_H1_HPP_
#define RTXUI_COMPONENT_DEFAULT_H1_HPP_

#include <string_view>

#include "rtxui/internal/component.hpp"

namespace rtxui {

class h1 : public Component<h1> {
 public:
  static const std::string_view view;
};

class h2 : public Component<h2> {
 public:
  static const std::string_view view;
};

class h3 : public Component<h3> {
 public:
  static const std::string_view view;
};

class h4 : public Component<h4> {
 public:
  static const std::string_view view;
};

class h5 : public Component<h5> {
 public:
  static const std::string_view view;
};

class h6 : public Component<h6> {
 public:
  static const std::string_view view;
};


}  // namespace rtxui

#endif  // RTXUI_COMPONENT_DEFAULT_H1_HPP_
