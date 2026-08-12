// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Measures how layout cost scales with nesting depth. The leaf content is
// identical in every run -- only the number of wrapper elements around it
// changes -- so the curve isolates what each extra level of nesting costs.
//
// Block flow lays each child out once, so its curve is flat. Flex and grid
// re-run their children (measure, then again with the resolved size), so
// without a measurement cache their curves are exponential in depth. Layout
// invocations are reported alongside the timings: they are deterministic,
// which makes them the metric worth regressing against.
#include "benchmark_common.hpp"
#include "rtxui/layout/layout.hpp"
#include "rtxui/terminal/terminal_device.hpp"

#include <iostream>
#include <string>
#include <vector>

using namespace rtxui;
using rtxui::bench::Clock;
using rtxui::bench::MicrosBetween;

namespace {

// The view is generated per run, so it cannot be a compile-time literal like
// the other benchmarks' templates.
std::string g_view;

class NestedComponent : public Component<NestedComponent> {
 public:
  std::string view;
  NestedComponent() { view = g_view; }
};

// `depth` wrappers of the given display type around a fixed leaf.
std::string MakeView(int depth, const char* style, int leaf_items) {
  std::string s = "<template>";
  for (int i = 0; i < depth; ++i) {
    s += std::string("<div style=\"") + style + "\">";
  }
  for (int i = 0; i < leaf_items; ++i) {
    s += "<div>lorem ipsum dolor sit amet " + std::to_string(i) + "</div>";
  }
  for (int i = 0; i < depth; ++i) {
    s += "</div>";
  }
  s += "</template>";
  return s;
}

struct Sample {
  double median_ms = 0.0;
  int layouts = 0;  // RunLayout() executions for a single frame.
};

Sample Measure(int depth, const char* style, int leaf_items) {
  const int kWarmupFrames = 2;
  const int kFrames = 5;

  g_view = MakeView(depth, style, leaf_items);
  auto app = Ref<NestedComponent>::New();
  auto device = std::make_shared<MockTerminalDevice>();
  device->TriggerResize(80, 24);
  Screen screen(app, device);
  app->Digest();

  for (int i = 0; i < kWarmupFrames; ++i) {
    screen.Draw();
    device->ClearOutput();
  }

  Sample sample;
  std::vector<double> times_us;
  for (int i = 0; i < kFrames; ++i) {
    ResetLayoutRunCount();
    auto t0 = Clock::now();
    screen.Draw();
    auto t1 = Clock::now();
    sample.layouts = LayoutRunCount();
    device->ClearOutput();
    times_us.push_back(MicrosBetween(t0, t1));
  }
  sample.median_ms = bench::ComputeStats(times_us).median / 1000.0;
  return sample;
}

struct Variant {
  const char* name;
  const char* style;
};

}  // namespace

int main() {
  const int kLeafItems = 40;
  const int kMaxDepth = 10;
  const int kDepthStep = 2;
  const Variant kVariants[] = {
      {"block", "display: block"},
      {"flex", "display: flex; flex-direction: column"},
      {"grid", "display: grid"},
  };

  std::cout << "{\n"
            << "  \"kind\": \"nesting\",\n"
            << "  \"leaf_items\": " << kLeafItems << ",\n"
            << "  \"max_depth\": " << kMaxDepth << ",\n";

  bool first = true;
  for (const Variant& variant : kVariants) {
    for (int depth = 0; depth <= kMaxDepth; depth += kDepthStep) {
      const Sample sample = Measure(depth, variant.style, kLeafItems);
      char depth_label[3];
      std::snprintf(depth_label, sizeof(depth_label), "%02d", depth);
      if (!first) {
        std::cout << ",\n";
      }
      first = false;
      std::cout << "  \"" << variant.name << "_depth_" << depth_label
                << "_ms\": " << sample.median_ms << ",\n"
                << "  \"" << variant.name << "_depth_" << depth_label
                << "_layouts\": " << sample.layouts;
      std::cout << std::flush;
    }
  }
  std::cout << "\n}\n";
  return 0;
}
