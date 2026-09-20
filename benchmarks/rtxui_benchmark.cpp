#define RTXUI_BENCHMARK
#include <chrono>
#include <vector>

#include "../example/demo.cpp"
#include "benchmark_common.hpp"
#include "rtxui/terminal/terminal_device.hpp"

using namespace rtxui;
using rtxui::bench::Clock;
using rtxui::bench::ComputeStats;
using rtxui::bench::EmitFrameJson;
using rtxui::bench::MicrosBetween;

int main() {
  auto app = Ref<App>::New();
  auto device = std::make_shared<MockTerminalDevice>();
  device->TriggerResize(120, 40);
  Screen screen(app, device);

  const int kWarmupFrames = 50;
  const int kBenchmarkFrames = 250;

  // Warmup phase
  for (int i = 0; i < kWarmupFrames; ++i) {
    app->count = i;
    app->slider_val = (i * 3) % 100;
    app->Digest();
    screen.Draw();
    device->ClearOutput();
  }

  // Microsecond timing arrays
  std::vector<double> digest_times_us;
  std::vector<double> draw_times_us;
  std::vector<double> total_times_us;

  // Measurement phase
  for (int i = 0; i < kBenchmarkFrames; ++i) {
    // 1. Trigger state updates
    app->count = i;
    app->slider_val = (i * 3) % 100;
    if (i % 10 == 0) {
      app->show_secret = !app->show_secret;
    }
    // Simulate list additions/deletions every 20 frames
    if (i % 20 == 0) {
      app->AddTodo();
    } else if (i % 20 == 10 && !app->todos.empty()) {
      app->RemoveTodo(std::to_string(app->todos.size() - 1));
    }
    task::TaskRunner::Current()->RunUntilNextDelayedTask();

    // 2. Measure Digest
    auto t0 = Clock::now();
    app->Digest();
    auto t1 = Clock::now();

    // 3. Measure Draw
    screen.Draw();
    auto t2 = Clock::now();

    device->ClearOutput();  // clear buffer to save memory

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
