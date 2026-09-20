// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Measures per-frame Digest + Draw cost while mutating one item every frame,
// exercising reconciliation, re-layout and re-paint. Also reports element
// allocation counts to catch reconciliation churn / leaks.
#include <atomic>
#include <iostream>
#include <string>
#include <vector>

#include "benchmark_common.hpp"
#include "rtxui/terminal/terminal_device.hpp"

using namespace rtxui;
using rtxui::bench::Clock;
using rtxui::bench::ComputeStats;
using rtxui::bench::EmitFrameJson;
using rtxui::bench::MicrosBetween;
using rtxui::bench::StressLayoutComponent;

int main() {
  const int kWarmupFrames = 10;
  const int kBenchmarkFrames = 50;
  {
    auto app = Ref<StressLayoutComponent>::New();
    auto device = std::make_shared<MockTerminalDevice>();
    device->TriggerResize(160, 100);
    Screen screen(app, device);

    // Warmup phase
    for (int i = 0; i < kWarmupFrames; ++i) {
      app->Digest();
      screen.Draw();
      device->ClearOutput();
    }

    std::vector<double> digest_times_us;
    std::vector<double> draw_times_us;
    std::vector<double> total_times_us;

    // Measurement phase
    for (int i = 0; i < kBenchmarkFrames; ++i) {
      // Dynamic updates to trigger digest and reconciliation
      app->items[i % app->items.size()] = "Updated description " +
                                          std::to_string(i) +
                                          " with modified ASCII text content!";

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
  }

  // The app and screen are destroyed above, so anything still alive here is
  // an actual leak (a live tree would previously mask one).
  extern std::atomic<int> g_elements_created;
  extern std::atomic<int> g_elements_destroyed;
  int created = g_elements_created.load();
  int destroyed = g_elements_destroyed.load();
  std::cerr << "--- Element Allocations ---\n"
            << "Created:   " << created << "\n"
            << "Destroyed: " << destroyed << "\n"
            << "Alive:     " << (created - destroyed) << "\n"
            << "---------------------------\n";

  return (created == destroyed) ? 0 : 1;
}
