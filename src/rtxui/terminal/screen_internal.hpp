// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_TERMINAL_SCREEN_INTERNAL_HPP_
#define RTXUI_TERMINAL_SCREEN_INTERNAL_HPP_

#include <memory>
#include <utility>

#include "rtxui/component.hpp"
#include "rtxui/refcounted.hpp"
#include "rtxui/screen.hpp"

namespace rtxui {

class ScreenImpl;
class TerminalDevice;

// What the library, its tests and benchmarks need of a Screen beyond the
// public API.
struct ScreenInternals {
  // A Screen drawing to `device` instead of the process's terminal.
  static Screen Make(Ref<ComponentBase> component,
                     std::shared_ptr<TerminalDevice> device) {
    return Screen(std::move(component), std::move(device));
  }

  static ScreenImpl& Impl(Screen& screen) { return *screen.impl_; }
  static const ScreenImpl& Impl(const Screen& screen) { return *screen.impl_; }
};

}  // namespace rtxui

#endif  // RTXUI_TERMINAL_SCREEN_INTERNAL_HPP_
