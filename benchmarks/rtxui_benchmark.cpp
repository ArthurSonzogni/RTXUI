#define RTXUI_BENCHMARK
#include "../example/demo.cpp"
#include "rtxui/terminal/terminal_device.hpp"

#include <chrono>
#include <iostream>
#include <numeric>
#include <vector>

using namespace rtxui;

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
    auto t0 = std::chrono::high_resolution_clock::now();
    app->Digest();
    auto t1 = std::chrono::high_resolution_clock::now();

    // 3. Measure Draw
    screen.Draw();
    auto t2 = std::chrono::high_resolution_clock::now();

    device->ClearOutput(); // clear buffer to save memory

    double digest_us = std::chrono::duration<double, std::micro>(t1 - t0).count();
    double draw_us = std::chrono::duration<double, std::micro>(t2 - t1).count();

    digest_times_us.push_back(digest_us);
    draw_times_us.push_back(draw_us);
    total_times_us.push_back(digest_us + draw_us);
  }

  auto avg = [](const std::vector<double>& v) {
    return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
  };

  auto min_val = [](const std::vector<double>& v) {
    return *std::min_element(v.begin(), v.end());
  };

  auto max_val = [](const std::vector<double>& v) {
    return *std::max_element(v.begin(), v.end());
  };

  // Output results in structured JSON format
  std::cout << "{\n"
            << "  \"frames\": " << kBenchmarkFrames << ",\n"
            << "  \"avg_digest_ms\": " << avg(digest_times_us) / 1000.0 << ",\n"
            << "  \"min_digest_ms\": " << min_val(digest_times_us) / 1000.0 << ",\n"
            << "  \"max_digest_ms\": " << max_val(digest_times_us) / 1000.0 << ",\n"
            << "  \"avg_draw_ms\": " << avg(draw_times_us) / 1000.0 << ",\n"
            << "  \"min_draw_ms\": " << min_val(draw_times_us) / 1000.0 << ",\n"
            << "  \"max_draw_ms\": " << max_val(draw_times_us) / 1000.0 << ",\n"
            << "  \"avg_frame_ms\": " << avg(total_times_us) / 1000.0 << ",\n"
            << "  \"min_frame_ms\": " << min_val(total_times_us) / 1000.0 << ",\n"
            << "  \"max_frame_ms\": " << max_val(total_times_us) / 1000.0 << "\n"
            << "}\n";

  return 0;
}
