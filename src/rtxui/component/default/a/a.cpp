// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/a/a.hpp"

namespace rtxui {

const std::string_view a::view = R"html(
    <slot></slot>
    <style>
      self {
        display: inline;
        text-decoration: underline;
        color: #3b82f6;
        cursor: pointer;
      }
    </style>
  )html";

namespace {
int RegisterThis = []() {
  RegisterGlobalComponent("a", []() { return Ref<a>::New(); });
  return 0;
}();
}  // namespace
}  // namespace rtxui
