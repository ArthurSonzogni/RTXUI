// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Measures per-frame Digest + Draw cost for a large, unchanging tree: the
// steady state where nothing mutates and the engine should do minimal work.
#include "benchmark_common.hpp"
#include "rtxui/terminal/terminal_device.hpp"

#include <vector>

using namespace rtxui;
using rtxui::bench::Clock;
using rtxui::bench::ComputeStats;
using rtxui::bench::EmitFrameJson;
using rtxui::bench::MicrosBetween;
using rtxui::bench::StressLayoutComponent;

int main() {
  auto app = Ref<StressLayoutComponent>::New();
  auto device = std::make_shared<MockTerminalDevice>();
  device->TriggerResize(160, 100);
  Screen screen(app, device);

  const int kWarmupFrames = 10;
  const int kBenchmarkFrames = 50;

  // Warmup phase
  for (int i = 0; i < kWarmupFrames; ++i) {
    app->Digest();
    screen.Draw();
    device->ClearOutput();
  }

  std::vector<double> digest_times_us;
  std::vector<double> draw_times_us;
  std::vector<double> total_times_us;

  // Measurement phase (steady state, no changes)
  for (int i = 0; i < kBenchmarkFrames; ++i) {
    auto t0 = Clock::now();
    app->Digest();
    auto t1 = Clock::now();

    screen.Draw();
    auto t2 = Clock::now();

    device->ClearOutput();

    double digest_us = MicrosBetween(t0, t1);
    double draw_us = MicrosBetween(t1, t2);

    digest_times_us.push_back(digest_us);
    draw_times_us.push_back(draw_us);
    total_times_us.push_back(digest_us + draw_us);
  }

  EmitFrameJson(kBenchmarkFrames, ComputeStats(digest_times_us),
                ComputeStats(draw_times_us), ComputeStats(total_times_us));

  return 0;
}
