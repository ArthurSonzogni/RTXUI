// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <iostream>
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class Counter : public Component<Counter> {
 public:
  int count = 0;

  int double_count() const { return count * 2; }

  Counter() {
    Bind(count);
    BindComputed(double_count);
    Import("Increment", [this]() { count++; });
    Import("Decrement", [this]() { count--; });
  }

  std::string_view Setup() override {
    Import<rtxui::div>();
    Import<rtxui::span>();
    Import<rtxui::button>();
    return R"html(
      <div class="counter-container">
        <div class="row">
          <span>Count: {count}</span>
          <span>Double: {double_count}</span>
        </div>
        <div class="row button-row">
          <button onclick="Increment">Increment</button>
          <button onclick="Decrement">Decrement</button>
        </div>
      </div>

      <style>
        self {
          display: block;
          padding: 1;
          background-color: rgb(15, 23, 42);
          color: white;
          border: tall;
          border-color: rgb(59, 130, 246);
        }
        .counter-container {
          display: block;
        }
        .row {
          display: block;
          margin-bottom: 1;
        }
        .button-row {
          display: flex;
          gap: 2;
        }
        span {
          margin-right: 2;
        }
        button {
          border: solid;
          border-color: rgb(59, 130, 246);
          padding-left: 1;
          padding-right: 1;
        }
      </style>
    )html";
  }
};

int main() {
  auto app = Ref<Counter>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
