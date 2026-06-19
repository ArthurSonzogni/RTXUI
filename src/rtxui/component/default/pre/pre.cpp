// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/pre/pre.hpp"

namespace rtxui {

const std::string_view pre::view = R"html(
    <slot></slot>
    <style>
      self { 
        display: block; 
        white-space: pre;
        margin-top: 1;
        margin-bottom: 1;
      }
    </style>
  )html";

namespace {
int RegisterThis = []() {
  RegisterGlobalComponent("pre", []() { return Ref<pre>::New(); });
  return 0;
}();
}  // namespace
}  // namespace rtxui
