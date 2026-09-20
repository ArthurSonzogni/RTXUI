// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_COMPONENT_DEFAULT_SEMANTIC_SEMANTIC_HPP_
#define RTXUI_COMPONENT_DEFAULT_SEMANTIC_SEMANTIC_HPP_

#include <string_view>

#include "rtxui/internal/component.hpp"

namespace rtxui {

class header : public Component<header> {
 public:
  static const std::string_view view;
};

class footer : public Component<footer> {
 public:
  static const std::string_view view;
};

class main : public Component<main> {
 public:
  static const std::string_view view;
};

class nav : public Component<nav> {
 public:
  static const std::string_view view;
};

class aside : public Component<aside> {
 public:
  static const std::string_view view;
};

class section : public Component<section> {
 public:
  static const std::string_view view;
};

class article : public Component<article> {
 public:
  static const std::string_view view;
};

}  // namespace rtxui

#endif  // RTXUI_COMPONENT_DEFAULT_SEMANTIC_SEMANTIC_HPP_
