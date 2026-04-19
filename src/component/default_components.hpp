// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_DEFAULT_COMPONENTS_HPP_
#define RTXUI_DEFAULT_COMPONENTS_HPP_

#include "component/component.hpp"

namespace rtxui {

class h1 : public Component<h1> {
 public:
  std::string_view Setup() override;
};

class div : public Component<div> {
 public:
  std::string_view Setup() override;
};

class span : public Component<span> {
 public:
  std::string_view Setup() override;
};

class p : public Component<p> {
 public:
  std::string_view Setup() override;
};

class strong : public Component<strong> {
 public:
  std::string_view Setup() override;
};

class ul : public Component<ul> {
 public:
  std::string_view Setup() override;
};

class li : public Component<li> {
 public:
  std::string_view Setup() override;
};

class ol : public Component<ol> {
 public:
  std::string_view Setup() override;
};

class button : public Component<button> {
 public:
  std::string_view Setup() override;
};

}  // namespace rtxui

#endif  // RTXUI_DEFAULT_COMPONENTS_HPP_
