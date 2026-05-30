// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <iostream>
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class InputDemo : public Component<InputDemo> {
 public:
  std::string text = "Hello, RTXUI!";

  InputDemo() {
    Bind(text);
  }

  std::string_view Setup() override {
    Import<rtxui::div>();
    Import<rtxui::span>();
    Import<rtxui::input>();
    Import<rtxui::p>();
    return R"html(
      <div class="container">
        <p class="title">Interactive Input Element</p>
        <p class="desc">Two-way data binding, arrow navigation (Ctrl to move by words), deletion (Ctrl to delete words), mouse click positioning, and horizontal scrolling on overflow.</p>
        
        <input class="styled-input" value="{text}" />
        
        <div class="output-box">
          <span class="label">Live Value:</span>
          <span class="value">"{text}"</span>
        </div>
      </div>

      <style>
        self {
          display: block;
          padding: 1;
          background-color: rgb(15, 23, 42);
          color: white;
          border: tall;
          border-color: rgb(99, 102, 241);
        }
        .container {
          display: block;
        }
        .title {
          display: block;
          font-weight: bold;
          color: rgb(129, 140, 248);
          margin-bottom: 1;
        }
        .desc {
          display: block;
          color: rgb(156, 163, 175);
          margin-bottom: 2;
        }
        .styled-input {
          display: inline flex;
          width: 40;
          border: tall;
          border-color: rgb(99, 102, 241);
          background-color: rgb(30, 41, 59);
          color: white;
          padding-left: 1;
          padding-right: 1;
          margin-bottom: 2;
        }
        .output-box {
          display: flex;
          gap: 2;
        }
        .label {
          color: rgb(129, 140, 248);
        }
        .value {
          font-weight: bold;
          color: rgb(244, 63, 94);
        }
      </style>
    )html";
  }
};

int main() {
  auto app = Ref<InputDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
