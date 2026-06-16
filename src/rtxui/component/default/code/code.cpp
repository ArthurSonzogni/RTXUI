// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/code/code.hpp"

namespace rtxui {

const std::string_view code::view = R"html(
    <slot></slot>
    <style>
      self { 
        display: inline; 
        background-color: rgba(255, 255, 255, 0.1);
        padding-left: 1;
        padding-right: 1;
      }
    </style>
  )html";

namespace {
int RegisterThis = []() {
  RegisterGlobalComponent("code", []() { return Ref<code>::New(); });
  return 0;
}();
}  // namespace
}  // namespace rtxui
