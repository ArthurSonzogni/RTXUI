// Copyright 2025 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "reflection/transparent_component.hpp"
#include <iostream>

namespace {
class Counter : public rtxui::TransparentComponent<Counter> {
 public:
  // --- Transparent State ---
  int count = 0;
  int multiplier = 2;

  // --- Transparent Computed ---
  int result() const { return count * multiplier; }

  // --- Action ---
  void Increment() { count++; }

  std::string_view Setup() {
    return R"html(
      <div>
        <span>Count: {count}</span>
        <span>Result: {result}</span>
        <button onclick="Increment">Add</button>
      </div>
    )html";
  }
};
} // namespace

int main() {
  std::cout << "[Test] Initializing Transparent Component..." << std::endl;
  auto counter = rtxui::Ref<Counter>::New();
  
  std::cout << "[Test] Simulating Action: Increment..." << std::endl;
  counter->Increment();
  
  std::cout << "[Test] Running Digest Cycle..." << std::endl;
  counter->Digest();

  std::cout << "[Test] Final Count: " << counter->count << std::endl;
  return 0;
}
