// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/button/button.hpp"

namespace rtxui {

const std::string_view button::view = R"html(
    <slot></slot>
    <style>
      self { 
        display: inline-block; 
        padding-left: 1;
        padding-right: 1;
        cursor: pointer;
        background-color: lighten(10%);
        opacity: 0.8;
        transition: background-color 0.1s linear, opacity 0.1s linear;
      }
      self:hover {
        background-color: lighten(18%);
        opacity: 0.9;
      }
      self:focus {
        background-color: lighten(28%);
        opacity: 1.0;
      }
      self:active {
        background-color: lighten(40%);
        opacity: 1.0;
      }
    </style>
  )html";

namespace {
int RegisterThis = []() {
  RegisterGlobalComponent("button", []() { return Ref<button>::New(); });
  return 0;
}();
}  // namespace
}  // namespace rtxui
