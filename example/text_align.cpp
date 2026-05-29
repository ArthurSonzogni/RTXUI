// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <iostream>
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class TextAlignDemo : public Component<TextAlignDemo> {
 public:
  std::string_view Setup() override {
    Import<rtxui::div>();
    Import<rtxui::h1>();
    Import<rtxui::p>();
    return R"html(
      <div>
        <h1>RTXUI Text Alignment Demo</h1>
        <p>Demonstrates text-align left, right, and center horizontal alignment.</p>
        
        <div class="card left-align">
          <div class="title">text-align: left</div>
          <div class="content">This text is aligned to the left of the container.</div>
        </div>

        <div class="card center-align">
          <div class="title">text-align: center</div>
          <div class="content">This text is aligned to the center of the container.</div>
        </div>

        <div class="card right-align">
          <div class="title">text-align: right</div>
          <div class="content">This text is aligned to the right of the container.</div>
        </div>
      </div>

      <style>
        self {
          display: block;
          padding: 1;
          border: round;
          border-color: blue;
          background-color: rgb(10, 10, 15);
          color: white;
          width: 50;
        }
        h1 {
          color: cyan;
          font-weight: bold;
          text-align: center;
        }
        p {
          margin-bottom: 1;
          color: gray;
          text-align: center;
        }
        .card {
          display: block;
          margin-top: 1;
          padding: 1;
          border: solid;
          border-color: gray;
        }
        .title {
          font-weight: bold;
          color: yellow;
          margin-bottom: 1;
        }
        .left-align {
          text-align: left;
        }
        .center-align {
          text-align: center;
        }
        .right-align {
          text-align: right;
        }
      </style>
    )html";
  }
};

int main() {
  auto app = Ref<TextAlignDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
