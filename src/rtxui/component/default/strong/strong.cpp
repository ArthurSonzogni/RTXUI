// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license[diff_block_start]
// the LICENSE file.
#include "rtxui/component/default/strong/strong.hpp"

namespace rtxui {

const std::string_view strong::view = R"html(
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
  RegisterGlobalComponent("strong", []() { return Ref<strong>::New(); });
  return 0;
}();
}  // namespace
}  // namespace rtxui
