// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/div/div.hpp"

namespace rtxui {

const std::string_view div::view = R"html(
    <slot></slot>
    <style>
      self { display: block; }
    </style>
  )html";

namespace {
int RegisterThis = []() {
  RegisterGlobalComponent("div", []() { return Ref<div>::New(); });
  return 0;
}();
}  // namespace
}  // namespace rtxui
