// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <iostream>
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class ProgressDemo : public Component<ProgressDemo> {
 public:
  int progress_val = 45;

  std::string_view view = R"html(
      <div class="container">
        <p class="title">Interactive Progress Bar Element</p>
        <p class="desc">
          The progress bar below is bound to the same value as the slider.
          Use the slider to change the progress bar level in real-time.
        </p>
        
        <div class="slider-wrapper">
          <span class="label">Control:</span>
          <slider value="{progress_val}" min="0" max="100" step="1" width="30" />
        </div>

        <div class="progress-wrapper">
          <span class="label">Progress:</span>
          <progress value="{progress_val}" max="100" width="30" />
        </div>
        
        <div class="output-box">
          <span class="label">Current Completion:</span>
          <span class="value">{progress_val}%</span>
        </div>
      </div>

      <style>
        self {
          display: block;
          padding: 1;
          background-color: rgb(18, 18, 18);
          color: white;
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
        .slider-wrapper, .progress-wrapper {
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
          width: 12;
        }
        .value {
          font-weight: bold;
          color: rgb(56, 189, 248);
        }
      </style>
    )html";

  ProgressDemo() { Bind(progress_val); }
};

int main() {
  auto app = Ref<ProgressDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
