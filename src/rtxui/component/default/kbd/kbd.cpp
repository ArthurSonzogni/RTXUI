// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/kbd/kbd.hpp"

namespace rtxui {

const std::string_view kbd::view = R"html(
    <slot></slot>
    <style>
      self {
        display: inline;
        font-weight: bold;
        background-color: rgba(255, 255, 255, 0.15);
        padding-left: 1;
        padding-right: 1;
      }
    </style>
  )html";

namespace {
int RegisterThis = []() {
  RegisterGlobalComponent("kbd", []() { return Ref<kbd>::New(); });
  return 0;
}();
}  // namespace
}  // namespace rtxui
