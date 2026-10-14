// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_TERMINAL_INPUT_WAIT_HPP_
#define RTXUI_TERMINAL_INPUT_WAIT_HPP_

#include <algorithm>
#include <chrono>

namespace rtxui {

// How long the event loop waits for input before it runs again: until the
// next frame while something moves (`moving`), and at the latest until the
// next delayed task is due (`next_task`, zero for none). Otherwise 100ms, for
// hot reload to check for changes.
inline std::chrono::milliseconds InputWait(
    bool moving,
    std::chrono::steady_clock::duration next_task) {
  std::chrono::milliseconds wait(moving ? 16 : 100);
  if (next_task > std::chrono::steady_clock::duration::zero()) {
    wait =
        std::min(wait, std::chrono::ceil<std::chrono::milliseconds>(next_task));
  }
  return wait;
}

}  // namespace rtxui

#endif  // RTXUI_TERMINAL_INPUT_WAIT_HPP_
