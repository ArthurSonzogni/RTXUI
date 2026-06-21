// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/summary/summary.hpp"

namespace rtxui {

const std::string_view summary::view = R"html(
    <slot></slot>
    <style>
      self { 
        display: inline; 
      }
    </style>
  )html";

namespace {
int RegisterThis = []() {
  RegisterGlobalComponent("summary", []() { return Ref<summary>::New(); });
  return 0;
}();
}  // namespace
}  // namespace rtxui
