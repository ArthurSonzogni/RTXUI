// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/tooltip/tooltip.hpp"

#include "rtxui/component/component_internal.hpp"
#include "rtxui/dom/element.hpp"

namespace rtxui {

void tooltip::InitReflection() {
  Bind(content);
  Bind(placement);
  Bind(tooltip_class);
  Component<tooltip>::InitReflection();
}

std::string_view tooltip::Setup() {
  return R"html(
    <div class="tooltip-container">
      <div class="tooltip-trigger">
        <slot></slot>
      </div>
      <div class="tooltip-popup {tooltip_class} {placement}">
        {content}
      </div>
    </div>
    <style>
      self {
        display: inline-block;
      }
      .tooltip-container {
        display: inline-block;
        position: relative;
      }
      .tooltip-trigger {
        display: inline-block;
      }
      .tooltip-popup {
        z-index: 1000;
        background-color: rgb(30, 41, 59);
        color: white;
        padding: 0 1;
        white-space: nowrap;
      }
      .tooltip-popup.top,
      .tooltip-popup.top-center,
      .tooltip-popup.top-middle {
        position: absolute;
        bottom: 100%;
        left: 0;
        right: 0;
        margin-left: auto;
        margin-right: auto;
      }
      .tooltip-popup.top-start,
      .tooltip-popup.top-left {
        position: absolute;
        bottom: 100%;
        left: 0;
        right: auto;
        margin-left: 0;
        margin-right: auto;
      }
      .tooltip-popup.top-end,
      .tooltip-popup.top-right {
        position: absolute;
        bottom: 100%;
        right: 0;
        left: auto;
        margin-left: auto;
        margin-right: 0;
      }
      .tooltip-popup.bottom,
      .tooltip-popup.bottom-center,
      .tooltip-popup.bottom-middle {
        position: absolute;
        top: 100%;
        left: 0;
        right: 0;
        margin-left: auto;
        margin-right: auto;
      }
      .tooltip-popup.bottom-start,
      .tooltip-popup.bottom-left {
        position: absolute;
        top: 100%;
        left: 0;
        right: auto;
        margin-left: 0;
        margin-right: auto;
      }
      .tooltip-popup.bottom-end,
      .tooltip-popup.bottom-right {
        position: absolute;
        top: 100%;
        right: 0;
        left: auto;
        margin-left: auto;
        margin-right: 0;
      }
      .tooltip-popup.left,
      .tooltip-popup.left-center,
      .tooltip-popup.left-middle {
        position: absolute;
        right: 100%;
        top: 0;
        bottom: 0;
        margin-top: auto;
        margin-bottom: auto;
      }
      .tooltip-popup.left-start,
      .tooltip-popup.left-top {
        position: absolute;
        right: 100%;
        top: 0;
        bottom: auto;
        margin-top: 0;
        margin-bottom: auto;
      }
      .tooltip-popup.left-end,
      .tooltip-popup.left-bottom {
        position: absolute;
        right: 100%;
        bottom: 0;
        top: auto;
        margin-bottom: 0;
        margin-top: auto;
      }
      .tooltip-popup.right,
      .tooltip-popup.right-center,
      .tooltip-popup.right-middle {
        position: absolute;
        left: 100%;
        top: 0;
        bottom: 0;
        margin-top: auto;
        margin-bottom: auto;
      }
      .tooltip-popup.right-start,
      .tooltip-popup.right-top {
        position: absolute;
        left: 100%;
        top: 0;
        bottom: auto;
        margin-top: 0;
        margin-bottom: auto;
      }
      .tooltip-popup.right-end,
      .tooltip-popup.right-bottom {
        position: absolute;
        left: 100%;
        bottom: 0;
        top: auto;
        margin-bottom: 0;
        margin-top: auto;
      }
      .hidden {
        display: none;
      }
      .visible {
        display: inline-block;
      }
    </style>
  )html";
}

bool tooltip::Digest() {
  bool is_hovered = Root() && Root()->hovered();
  std::string expected_class = is_hovered ? "visible" : "hidden";
  if (tooltip_class != expected_class) {
    tooltip_class = expected_class;
  }
  return Component<tooltip>::Digest();
}

namespace {
int RegisterThis = []() {
  RegisterGlobalComponent("tooltip", []() { return Ref<tooltip>::New(); });
  return 0;
}();
}  // namespace
}  // namespace rtxui
