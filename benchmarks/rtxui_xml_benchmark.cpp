#include "rtxui/xml/xml.hpp"
#include <chrono>
#include <iostream>
#include <numeric>
#include <vector>

const std::string_view kXmlTemplate = R"xml(
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
        <div class="card">
          <div class="card-title">Card #1</div>
          <span class="card-body">Item description 1 with some extra ASCII text filler to test xml parsing speed!</span>
        </div>
        <div class="card">
          <div class="card-title">Card #2</div>
          <span class="card-body">Item description 2 with some extra ASCII text filler to test xml parsing speed!</span>
        </div>
        <div class="card">
          <div class="card-title">Card #3</div>
          <span class="card-body">Item description 3 with some extra ASCII text filler to test xml parsing speed!</span>
        </div>
      </div>
    </div>
  </template>
)xml";

int main() {
  const int kIterations = 10000;

  // Warmup
  for (int i = 0; i < 100; ++i) {
    auto res = xml::Parse(kXmlTemplate);
    if (!res) {
      std::cerr << "Warmup parse failed: " << res.error().message << "\n";
      return 1;
    }
  }

  auto t0 = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < kIterations; ++i) {
    auto res = xml::Parse(kXmlTemplate);
    (void)res;
  }
  auto t1 = std::chrono::high_resolution_clock::now();

  double duration_us = std::chrono::duration<double, std::micro>(t1 - t0).count();
  std::cout << "{\n"
            << "  \"iterations\": " << kIterations << ",\n"
            << "  \"total_time_ms\": " << duration_us / 1000.0 << ",\n"
            << "  \"avg_parse_us\": " << duration_us / kIterations << "\n"
            << "}\n";

  return 0;
}
