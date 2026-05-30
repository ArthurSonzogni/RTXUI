// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/p/p.hpp"

namespace rtxui {

const std::string_view p::view = R"html(
    <slot></slot>
    <style>
      self { 
        display: block; 
        margin-top: 1;
        margin-bottom: 1;
      }
    </style>
  )html";

namespace {
int RegisterThis = []() {
  RegisterGlobalComponent("p", []() { return Ref<p>::New(); });
  return 0;
}();
}  // namespace
}  // namespace rtxui
