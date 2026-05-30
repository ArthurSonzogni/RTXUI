// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/h1/h1.hpp"

namespace rtxui {

const std::string_view h1::view = R"html(
    <slot></slot>
    <style>
      self { 
        display: block; 
        font-weight: bold;
        text-decoration: underlined;
        margin-bottom: 1;
      }
    </style>
  )html";

namespace {
int RegisterThis = []() {
  RegisterGlobalComponent("h1", []() { return Ref<h1>::New(); });
  return 0;
}();
}  // namespace
}  // namespace rtxui
