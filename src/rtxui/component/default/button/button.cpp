// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/button/button.hpp"

#include "rtxui/component/component_internal.hpp"
#include "rtxui/dom/element.hpp"

namespace rtxui {

void button::InitReflection() {
  Bind(disabled);
  Component<button>::InitReflection();
}

bool button::Digest() {
  SyncDisabled(Root(), disabled);
  return Component<button>::Digest();
}

const std::string_view button::view = R"html(
    <slot></slot>
    <style>
      self {
        display: inline-block;
        padding-left: 1;
        padding-right: 1;
        cursor: pointer;
        background-color: lighten(10%);
        opacity: 0.85;
        transition: background-color 0.1s linear, opacity 0.1s linear, color 0.1s linear;
      }
      self:hover {
        background-color: rgb(59, 130, 246);
        color: white;
        opacity: 0.95;
      }
      self:focus {
        background-color: rgb(37, 99, 235);
        color: white;
        opacity: 1.0;
      }
      self:active {
        background-color: rgb(29, 78, 216);
        color: white;
        opacity: 1.0;
      }
      self:disabled {
        cursor: default;
        opacity: 0.4;
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
