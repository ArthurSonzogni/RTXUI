// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <iostream>
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class Counter : public Component<Counter> {
 public:
  int count = 0;

  Counter() {
    Bind(count);
    Import("Increment", [this]() { count++; });
    Import("Decrement", [this]() { count--; });
  }

  std::string_view Setup() override {
    Import<rtxui::div>();
    Import<rtxui::span>();
    Import<rtxui::p>();
    Import<rtxui::button>();
    return R"html(
      <div>
        <p>Simple Clicker Demo</p>
        <button onclick="Decrement">-</button>
        <span> Value: {count} </span>
        <button onclick="Increment">+</button>
      </div>

      <style>
        self {
          display: block;
          padding: 1;
          border: tall;
          border-color: green;
          background-color: rgb(20, 20, 20);
          color: white;
        }
        p {
          font-weight: bold;
          color: cyan;
          margin-bottom: 1;
        }
        button {
          border: solid;
          border-color: yellow;
          padding-left: 1;
          padding-right: 1;
        }
        span {
          margin-left: 1;
          margin-right: 1;
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
