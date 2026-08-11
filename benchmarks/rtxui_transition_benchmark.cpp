// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Frame cost while transitions are in flight.
//
// The other frame benchmarks measure a tree that is not animating, where
// ResolveTargetStyles() takes its early return and the per-element style
// copies never happen. That hides the cost of the path an app with any
// animation at all is on every frame: target_style is reset from base_style
// for every element in the tree, the pseudo-class pass runs, and every
// element's style is recomputed by TriggerTransitions.
#include <rtxui/rtxui.hpp>

#include <memory>
#include <string>
#include <vector>

#include "benchmark_common.hpp"
#include "rtxui/terminal/terminal_device.hpp"

using namespace rtxui;
using rtxui::bench::Clock;
using rtxui::bench::ComputeStats;
using rtxui::bench::EmitFrameJson;
using rtxui::bench::MicrosBetween;

namespace {

// Every card transitions, and `lit` flips the class that drives them, so each
// measured frame has the whole tree mid-animation rather than settled.
class TransitionStressComponent : public Component<TransitionStressComponent> {
 public:
  std::vector<std::string> items;
  bool lit = false;
  std::string state_class() const { return lit ? "lit" : ""; }
  std::string view;

  explicit TransitionStressComponent(int item_count = 500) {
    items.reserve(item_count);
    for (int i = 0; i < item_count; ++i) {
      items.push_back("Item " + std::to_string(i) + " with filler text");
    }
    RegisterCollection("items", &items);
    Bind(lit);
    Bind(state_class);

    view = R"xml(
      <template>
        <style>
          .root-container {
            display: flex;
            flex-direction: column;
            width: 100%;
            height: 100%;
          }
          .grid {
            display: flex;
            flex-direction: row;
            flex-wrap: wrap;
          }
          .card {
            display: block;
            width: 30%;
            border: solid;
            margin: 1;
            padding: 1;
            background-color: #202020;
            color: #a0a0a0;
            transition: background-color 4s linear, color 4s linear;
          }
          .card.lit {
            background-color: #d0d0d0;
            color: #101010;
          }
        </style>
        <div class="root-container">
          <div class="grid">
            <for each="{items}" as="item">
              <div class="card {state_class}">
                <div>{item}</div>
              </div>
            </for>
          </div>
        </div>
      </template>
    )xml";
  }
};

}  // namespace

int main() {
  auto app = Ref<TransitionStressComponent>::New();
  auto device = std::make_shared<MockTerminalDevice>();
  device->TriggerResize(160, 100);
  Screen screen(app, device);

  const int kWarmupFrames = 10;
  const int kBenchmarkFrames = 50;

  for (int i = 0; i < kWarmupFrames; ++i) {
    app->Digest();
    screen.Draw();
    device->ClearOutput();
  }

  // Start the transitions and leave them running: 4s each, so all 50 measured
  // frames land while every card is still interpolating.
  app->lit = true;
  app->Digest();
  app->ResolveTargetStyles();

  std::vector<double> digest_times_us;
  std::vector<double> draw_times_us;
  std::vector<double> total_times_us;

  for (int i = 0; i < kBenchmarkFrames; ++i) {
    auto t0 = Clock::now();
    app->Digest();
    // What a real frame does while animating; Screen::Step() calls this on
    // every tick, and it is the stage the early return skips when nothing is
    // in flight.
    app->ResolveTargetStyles();
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
