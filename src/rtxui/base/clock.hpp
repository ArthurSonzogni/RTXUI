// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_BASE_CLOCK_HPP_
#define RTXUI_BASE_CLOCK_HPP_

#include <chrono>

namespace rtxui::time {

// The clock animations, transitions and delayed tasks all read. A test can
// replace it, to move time only when it says so.
using ClockFn = double (*)();
void SetCustomClock(ClockFn clock);

// Milliseconds, from an arbitrary origin.
double GetTimeMs();

// The same clock, as the steady_clock time point delayed tasks are due at.
std::chrono::steady_clock::time_point SteadyNow();

}  // namespace rtxui::time

#endif  // RTXUI_BASE_CLOCK_HPP_
