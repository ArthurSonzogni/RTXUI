// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/b/b.hpp"

namespace rtxui {

const std::string_view b::view = R"html(
    <slot></slot>
    <style>
      self { 
        display: inline; 
        font-weight: bold;
      }
    </style>
  )html";

namespace {
int RegisterThis = []() {
  RegisterGlobalComponent("b", []() { return Ref<b>::New(); });
  return 0;
}();
}  // namespace
}  // namespace rtxui
