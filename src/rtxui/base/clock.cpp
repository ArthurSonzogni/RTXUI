// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/base/clock.hpp"

namespace rtxui::time {

namespace {
ClockFn custom_clock = nullptr;
}  // namespace

void SetCustomClock(ClockFn clock) {
  custom_clock = clock;
}

double GetTimeMs() {
  if (custom_clock) {
    return custom_clock();
  }
  auto now = std::chrono::steady_clock::now();
  return std::chrono::duration<double, std::milli>(now.time_since_epoch())
      .count();
}

std::chrono::steady_clock::time_point SteadyNow() {
  if (!custom_clock) {
    return std::chrono::steady_clock::now();
  }
  return std::chrono::steady_clock::time_point(
      std::chrono::duration_cast<std::chrono::steady_clock::duration>(
          std::chrono::duration<double, std::milli>(custom_clock())));
}

}  // namespace rtxui::time
