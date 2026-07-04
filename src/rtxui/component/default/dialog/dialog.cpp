// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/dialog/dialog.hpp"

#include "rtxui/component/component_internal.hpp"
#include "rtxui/dom/element.hpp"

namespace rtxui {

void dialog::InitReflection() {
  Bind(open);
  Bind(title);
  Bind(overlay_class);
  Component<dialog>::InitReflection();
}

std::string_view dialog::Setup() {
  return R"html(
    <div class="dialog-overlay {overlay_class}">
      <div class="dialog-box">
        <div class="dialog-header">
          <span class="dialog-title">{title}</span>
        </div>
        <div class="dialog-body">
          <slot></slot>
        </div>
      </div>
    </div>
    <style>
      self {
        display: block;
      }
      .dialog-overlay {
        position: fixed;
        top: 0;
        left: 0;
        width: 100%;
        height: 100%;
        z-index: 100;
        display: flex;
        justify-content: center;
        align-items: center;
        background-color: rgba(0, 0, 0, 0.4);
      }
      .dialog-box {
        display: flex;
        flex-direction: column;
        border: double;
        border-color: rgb(59, 130, 246);
        background-color: rgb(30, 41, 59);
        padding: 1;
        min-width: 30;
        max-width: 60;
      }
      .dialog-box button {
        background-color: rgb(75, 85, 99);
        color: white;
        border: solid;
        border-color: rgb(156, 163, 175);
      }
      .dialog-box button:hover {
        background-color: rgb(59, 130, 246);
        border-color: rgb(96, 165, 250);
      }
      .dialog-box button:focus {
        background-color: rgb(37, 99, 235);
        border-color: rgb(147, 197, 253);
      }
      .dialog-box button:active {
        background-color: rgb(29, 78, 216);
        border-color: rgb(96, 165, 250);
      }
      .dialog-header {
        display: block;
        border-bottom: solid;
        border-color: rgb(74, 85, 104);
        padding-bottom: 1;
        margin-bottom: 1;
        font-weight: bold;
        color: rgb(96, 165, 250);
      }
      .dialog-body {
        display: block;
      }
      .closed {
        display: none;
      }
      .open {
        display: flex;
      }
    </style>
  )html";
}

bool dialog::Digest() {
  overlay_class = open ? "open" : "closed";
  return Component<dialog>::Digest();
}

namespace {
int RegisterThis = []() {
  RegisterGlobalComponent("dialog", []() { return Ref<dialog>::New(); });
  return 0;
}();
}  // namespace
}  // namespace rtxui
