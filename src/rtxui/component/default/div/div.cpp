// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/div/div.hpp"

namespace rtxui {

const std::string_view div::view = R"html(
    <slot></slot>
    <style>
      self {
        display: block;
        scrollbar-color: rgba(200, 200, 200, 0.78432) rgba(80, 80, 80, 0.4706);
        transition: scrollbar-color 0.2s;
      }
      self:scrollbar-hover {
        scrollbar-color: rgba(220, 220, 220, 0.8628) rgba(90, 90, 90, 0.5491);
      }
      self:scrollbar-thumb-hover {
        scrollbar-color: rgba(240, 240, 240, 0.9412) rgba(90, 90, 90, 0.5491);
      }
      self:scrollbar-thumb-active {
        scrollbar-color: rgba(255, 255, 255, 1.0) rgba(100, 100, 100, 0.6275);
      }
    </style>
  )html";

namespace {
int RegisterThis = []() {
  RegisterGlobalComponent("div", []() { return Ref<div>::New(); });
  return 0;
}();
}  // namespace
}  // namespace rtxui
