#include "rtxui/rtxui.hpp"
#include "rtxui/terminal/terminal_device.hpp"

#include <atomic>
#include <chrono>
#include <iostream>
#include <numeric>
#include <vector>

using namespace rtxui;

class StressLayoutComponent : public Component<StressLayoutComponent> {
 public:
  std::vector<std::string> items;
  std::string view;

  StressLayoutComponent() {
    // Generate a list of 500 items for a stress layout test
    items.reserve(500);
    for (int i = 0; i < 500; ++i) {
      items.push_back("Item description " + std::to_string(i) + " with some extra ASCII text filler to test process_text_in_flow performance!");
    }

    // Register properties
    RegisterCollection("items", &items);

    // XML Template: a large flex container with nested blocks, borders, paddings, and texts.
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

  // Measurement phase
  for (int i = 0; i < kBenchmarkFrames; ++i) {
    // Dynamic updates to trigger digest and reconciliation
    app->items[i % 500] = "Updated description " + std::to_string(i) + " with modified ASCII text content!";

    auto t0 = std::chrono::high_resolution_clock::now();
    app->Digest();
    auto t1 = std::chrono::high_resolution_clock::now();

    screen.Draw();
    auto t2 = std::chrono::high_resolution_clock::now();

    device->ClearOutput();

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

  extern std::atomic<int> g_elements_created;
  extern std::atomic<int> g_elements_destroyed;
  int created = g_elements_created.load();
  int destroyed = g_elements_destroyed.load();
  std::cerr << "--- Element Allocations ---\n"
            << "Created:   " << created << "\n"
            << "Destroyed: " << destroyed << "\n"
            << "Alive:     " << (created - destroyed) << "\n"
            << "---------------------------\n";

  return 0;
}
