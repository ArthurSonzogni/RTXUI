// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Shared helpers for the RTXUI benchmarks: timing, summary statistics, the
// JSON output format consumed by tools/benchmark.py, and the stress component
// reused by the layout / steady-state benchmarks.
#ifndef RTXUI_BENCHMARKS_BENCHMARK_COMMON_HPP
#define RTXUI_BENCHMARKS_BENCHMARK_COMMON_HPP

#include <algorithm>
#include <chrono>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#include "rtxui/rtxui.hpp"

namespace rtxui::bench {

using Clock = std::chrono::high_resolution_clock;

// Microseconds elapsed between two time points.
inline double MicrosBetween(Clock::time_point begin, Clock::time_point end) {
  return std::chrono::duration<double, std::micro>(end - begin).count();
}

// Summary of a sample of durations (all values in microseconds).
struct Stats {
  double avg = 0.0;
  double min = 0.0;
  double max = 0.0;
  double median = 0.0;
};

// Computes avg/min/max/median over a copy of the sample.
inline Stats ComputeStats(std::vector<double> values) {
  Stats s;
  if (values.empty()) {
    return s;
  }
  s.avg = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
  s.min = *std::min_element(values.begin(), values.end());
  s.max = *std::max_element(values.begin(), values.end());
  std::sort(values.begin(), values.end());
  const size_t n = values.size();
  s.median =
      (n % 2 == 0) ? (values[n / 2 - 1] + values[n / 2]) / 2.0 : values[n / 2];
  return s;
}

// Emits one stage of frame-timing stats, converting microseconds to
// milliseconds. `comma` controls the trailing separator.
inline void EmitStageJson(const std::string& stage,
                          const Stats& s,
                          bool comma) {
  std::cout << "  \"avg_" << stage << "_ms\": " << s.avg / 1000.0 << ",\n"
            << "  \"min_" << stage << "_ms\": " << s.min / 1000.0 << ",\n"
            << "  \"max_" << stage << "_ms\": " << s.max / 1000.0 << ",\n"
            << "  \"median_" << stage << "_ms\": " << s.median / 1000.0
            << (comma ? ",\n" : "\n");
}

// Emits the standard frame-timing JSON document consumed by
// tools/benchmark.py (digest + draw + total-frame stages).
inline void EmitFrameJson(int frames,
                          const Stats& digest,
                          const Stats& draw,
                          const Stats& frame) {
  std::cout << "{\n"
            << "  \"frames\": " << frames << ",\n";
  EmitStageJson("digest", digest, true);
  EmitStageJson("draw", draw, true);
  EmitStageJson("frame", frame, false);
  std::cout << "}\n";
}

// A large, wrap-heavy grid used to stress layout + paint. The list of items is
// registered as a reactive collection so mutating `items` drives
// reconciliation.
class StressLayoutComponent : public Component<StressLayoutComponent> {
 public:
  std::vector<std::string> items;
  std::string view;

  explicit StressLayoutComponent(int item_count = 500) {
    items.reserve(item_count);
    for (int i = 0; i < item_count; ++i) {
      items.push_back("Item description " + std::to_string(i) +
                      " with some extra ASCII text filler to stress layout!");
    }

    RegisterCollection("items", &items);

    view = R"xml(
      <template>
        <style>
          .root-container {
            display: flex;
            flex-direction: column;
            width: 100%;
            height: 100%;
            border: 1px solid #777;
            padding: 2px;
          }
          .header {
            display: block;
            background-color: #222;
            color: #fff;
            padding: 1px;
            font-weight: bold;
            text-align: center;
          }
          .grid {
            display: flex;
            flex-direction: row;
            flex-wrap: wrap;
            padding: 1px;
          }
          .card {
            display: block;
            width: 30%;
            border: 1px solid #444;
            margin: 1px;
            padding: 1px;
          }
          .card-title {
            display: block;
            color: #0ff;
            font-weight: bold;
          }
          .card-body {
            display: inline;
            white-space: nowrap;
          }
        </style>
        <div class="root-container">
          <div class="header">STRESS LAYOUT BENCHMARK</div>
          <div class="grid">
            <for each="{items}" as="item">
              <div class="card">
                <div class="card-title">Card #{$index}</div>
                <span class="card-body">{item}</span>
              </div>
            </for>
          </div>
        </div>
      </template>
    )xml";
  }
};

}  // namespace rtxui::bench

#endif  // RTXUI_BENCHMARKS_BENCHMARK_COMMON_HPP
