// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/u/u.hpp"

namespace rtxui {

const std::string_view u::view = R"html(
    <slot></slot>
    <style>
      self { 
        display: inline; 
        text-decoration: underline;
      }
    </style>
  )html";

namespace {
int RegisterThis = []() {
  RegisterGlobalComponent("u", []() { return Ref<u>::New(); });
  return 0;
}();
}  // namespace
}  // namespace rtxui
