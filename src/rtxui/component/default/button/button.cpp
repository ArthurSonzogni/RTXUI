// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/button/button.hpp"

namespace rtxui {

const std::string_view button::view = R"html(
    <slot></slot>
    <style>
      self { 
        display: inline-block; 
        border: tall;
        padding-left: 1;
        padding-right: 1;
      }
    </style>
  )html";

namespace {
int RegisterThis = []() {
  RegisterGlobalComponent("button", []() { return Ref<button>::New(); });
  return 0;
}();
}  // namespace
}  // namespace rtxui
