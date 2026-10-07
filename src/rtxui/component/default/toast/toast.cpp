// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/toast/toast.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <string>

#include "rtxui/diagnostic.hpp"
#include "rtxui/task.hpp"

namespace rtxui {

void toast::Close() {
  open = false;
  PropagateBinding("open", "false");
}

void toast::InitReflection() {
  Bind(open);
  Bind(duration);
  Bind(placement);
  Bind(toast_class);
  Bind(Close);
  Component<toast>::InitReflection();
}

std::string_view toast::Setup() {
  return R"html(
    <div class="toast {toast_class}" part="toast" onclick="Close">
      <slot></slot>
    </div>
    <style>
      @keyframes toast-in {
        from { opacity: 0; }
        to { opacity: 1; }
      }
      self {
        display: block;
      }
      .toast {
        position: fixed;
        z-index: 200;
        display: block;
        max-width: 50;
        border: round;
        border-color: rgb(96, 165, 250);
        background-color: rgb(30, 41, 59);
        color: rgb(226, 232, 240);
        padding: 0 1;
        animation: toast-in 150ms ease-out;
      }
      .closed {
        display: none;
      }
      .bottom-right { bottom: 1; right: 2; }
      .bottom-left { bottom: 1; left: 2; }
      .top-right { top: 1; right: 2; }
      .top-left { top: 1; left: 2; }
    </style>
  )html";
}

bool toast::Digest() {
  static constexpr std::array<std::string_view, 4> kPlacements = {
      "bottom-right", "bottom-left", "top-right", "top-left"};
  std::string_view corner = placement;
  if (std::ranges::find(kPlacements, corner) == kPlacements.end()) {
    ReportDiagnostic("<toast placement=\"" + placement +
                     "\">: expected bottom-right, bottom-left, top-right or "
                     "top-left");
    corner = kPlacements[0];
  }
  toast_class = open ? std::string(corner) : "closed";

  // Each opening starts its own timer.
  if (open && !was_open_) {
    ++generation_;
    if (duration > 0) {
      PostDelayedTask(
          [self = Ref<toast>(this), generation = generation_] {
            if (self->open && self->generation_ == generation) {
              self->Close();
            }
          },
          std::chrono::milliseconds(duration));
    }
  }
  was_open_ = open;
  return Component<toast>::Digest();
}

}  // namespace rtxui
