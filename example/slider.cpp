// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <iostream>
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class SliderDemo : public Component<SliderDemo> {
 public:
  int volume = 50;

  SliderDemo() {
    Bind(volume);
  }

  std::string_view Setup() override {




    return R"html(
      <div class="container">
        <p class="title">Interactive Slider Element</p>
        <p class="desc">
          Drag the thumb with mouse click or navigate using Arrow Keys (Left/Right or Up/Down) when focused to adjust the value.
        </p>
        
        <div class="slider-wrapper">
          <span class="label">Volume:</span>
          <slider value="{volume}" min="0" max="100" step="5" width="30" />
        </div>
        
        <div class="output-box">
          <span class="label">Live Volume Value:</span>
          <span class="value">{volume}%</span>
        </div>
      </div>

      <style>
        self {
          display: block;
          padding: 1;
          background-color: rgb(15, 23, 42);
          color: white;
          border: tall;
          border-color: rgb(56, 189, 248);
        }
        .container {
          display: block;
        }
        .title {
          display: block;
          font-weight: bold;
          color: rgb(56, 189, 248);
          margin-bottom: 1;
        }
        .desc {
          display: block;
          color: rgb(156, 163, 175);
          margin-bottom: 2;
        }
        .slider-wrapper {
          display: flex;
          gap: 2;
          margin-bottom: 2;
          align-items: center;
        }
        .output-box {
          display: flex;
          gap: 2;
          margin-top: 2;
        }
        .label {
          color: rgb(56, 189, 248);
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
  auto app = Ref<SliderDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
