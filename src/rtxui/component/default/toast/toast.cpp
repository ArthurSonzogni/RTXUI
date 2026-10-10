// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/toast/toast.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <format>
#include <string>

#include "rtxui/base/clock.hpp"
#include "rtxui/diagnostic.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/task.hpp"

namespace rtxui {

namespace {
// The durations of the slides in Setup()'s style.
constexpr double kSlideInMs = 300.0;
constexpr double kSlideOutMs = 200.0;
}  // namespace

void toast::Close() {
  open = false;
  PropagateBinding("open", "false");
}

void toast::AnimationEnded() {
  // The opening animation ends too, while `closing_` is still false.
  if (closing_) {
    FinishClosing();
  }
}

void toast::FinishClosing() {
  closing_ = false;
  toast_class = "closed";
  toast_part = "toast";
}

std::string toast::MirrorSlide(std::string_view interrupted_prefix,
                               double next_duration_ms) const {
  const Element* box = Root()->QuerySelector(".toast");
  if (!box) {
    return "";
  }
  for (const RunningAnimation& animation : box->running_animations) {
    if (animation.finished ||
        !animation.config.name.starts_with(interrupted_prefix)) {
      continue;
    }
    const double duration_ms = animation.config.duration_seconds * 1000.0;
    const double elapsed_ms = time::GetTimeMs() - animation.start_time_ms -
                              animation.config.delay_seconds * 1000.0;
    if (duration_ms <= 0.0) {
      return "";
    }
    // ease-out-cubic is ease-in-cubic played backwards, so the slide in at
    // progress 1 - p is where the slide out at progress p is, and conversely.
    const double progress = std::clamp(elapsed_ms / duration_ms, 0.0, 1.0);
    return std::format("animation-delay: {:.6f}s",
                       -(1.0 - progress) * next_duration_ms / 1000.0);
  }
  return "";
}

void toast::InitReflection() {
  Bind(open);
  Bind(duration);
  Bind(placement);
  Bind(toast_class);
  Bind(toast_part);
  Bind(toast_style);
  Bind(Close);
  Bind(AnimationEnded);
  Component<toast>::InitReflection();
}

std::string_view toast::Setup() {
  return R"html(
    <div class="toast {toast_class}" part="{toast_part}" style="{toast_style}"
         onclick="Close"
         onanimationend="AnimationEnded">
      <slot></slot>
    </div>
    <style>
      /* Horizontal: a terminal has more columns than rows, so the move is
         smoother. By its own width and the 2 cells it sits from the edge:
         just out of sight, whatever its size. */
      @keyframes toast-in-right { from { translate: calc(100% + 2); } }
      @keyframes toast-out-right { to { translate: calc(100% + 2); } }
      @keyframes toast-in-left { from { translate: calc(-100% - 2); } }
      @keyframes toast-out-left { to { translate: calc(-100% - 2); } }
      self {
        display: block;
      }
      .toast {
        position: fixed;
        z-index: 200;
        display: block;
        max-width: 50;
        border: tall;
        border-color: rgb(96, 165, 250);
        background-color: rgb(30, 41, 59);
        color: rgb(226, 232, 240);
        padding: 0 1;
      }
      .bottom-right, .top-right {
        animation: toast-in-right 300ms ease-out-cubic;
      }
      .bottom-left, .top-left {
        animation: toast-in-left 300ms ease-out-cubic;
      }
      .closing.bottom-right, .closing.top-right {
        animation: toast-out-right 200ms ease-in-cubic forwards;
      }
      .closing.bottom-left, .closing.top-left {
        animation: toast-out-left 200ms ease-in-cubic forwards;
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

  // Each opening starts its own timer.
  if (open && !was_open_) {
    // Reopened while sliding out: it slides back in from where it is.
    toast_style = MirrorSlide("toast-out-", kSlideInMs);
    ++generation_;
    closing_ = false;
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
  // Closing keeps the toast on screen until its closing animation ends. Once
  // this frame's styles say whether there is one: none hides it at once.
  if (!open && was_open_) {
    // Closed while sliding in: it slides back out from where it is.
    toast_style = MirrorSlide("toast-in-", kSlideOutMs);
    closing_ = true;
    PostTask([self = Ref<toast>(this), generation = generation_] {
      if (!self->closing_ || self->generation_ != generation) {
        return;
      }
      const Element* box = self->Root()->QuerySelector(".toast");
      if (!box || !(box->HasPlayingAnimations(/*include_infinite=*/false) ||
                    box->HasEndedAnimations())) {
        self->FinishClosing();
      }
    });
  }
  was_open_ = open;

  if (open) {
    toast_class = std::string(corner);
    toast_part = "toast";
  } else if (closing_) {
    toast_class = std::string(corner) + " closing";
    toast_part = "toast closing";
  } else {
    FinishClosing();
  }
  return Component<toast>::Digest();
}

}  // namespace rtxui
