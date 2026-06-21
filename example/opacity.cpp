// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <iostream>
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class OpacityDemo : public Component<OpacityDemo> {
 public:
  int opacity_percent = 70;

  float opacity_float() const { return opacity_percent / 100.0f; }

  std::string_view view = R"html(
      <div class="container">
        <p class="title">Opacity Property Demo</p>
        <p class="desc">
          rtxui supports the CSS 'opacity' property (0.0 to 1.0) for alpha-blending
          elements and text transparently over underlying terminal content.
        </p>

        <!-- Live Binding Control -->
        <div class="control-row">
          <span class="label">Adjust Opacity:</span>
          <slider value="{opacity_percent}" min="0" max="100" step="10" width="30" />
          <span class="value">{opacity_percent}%</span>
        </div>

        <div class="demo-grid">
          <!-- 1. Opacity Box & Inheritance -->
          <div class="card box-card">
            <h3>1. Alpha Blending & Inheritance</h3>
            <div class="parent-box" style="opacity: {opacity_float};">
              <span class="parent-label">Parent Container (opacity: {opacity_float})</span>
              <div class="child-box">
                <span class="child-label">Nested Child (inherits/accumulates opacity)</span>
              </div>
            </div>
          </div>

          <!-- 2. CSS Transitions on Opacity -->
          <div class="card transition-card">
            <h3>2. CSS Transition on Hover</h3>
            <p class="sub-desc">Hover or focus the button below to trigger a smooth opacity transition:</p>
            <div class="hover-btn" tabindex="0">Hover/Focus Me</div>
          </div>
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
        .control-row {
          display: flex;
          gap: 2;
          margin-bottom: 2;
          align-items: center;
        }
        .label {
          color: rgb(56, 189, 248);
          width: 16;
        }
        .value {
          font-weight: bold;
          color: rgb(56, 189, 248);
        }
        .demo-grid {
          display: flex;
          flex-direction: row;
          gap: 2;
          width: 100%;
        }
        .card {
          flex: 1;
          display: block;
          border: tall;
          border-color: rgb(30, 41, 59);
          padding: 1 2;
          height: 12;
        }
        h3 {
          color: rgb(96, 165, 250);
          font-weight: bold;
          margin-bottom: 1;
        }
        .sub-desc {
          color: rgb(148, 163, 184);
          margin-bottom: 2;
        }

        /* 1. Inheritance Styling */
        .parent-box {
          display: block;
          border: solid;
          border-color: rgb(59, 130, 246);
          background-color: rgb(30, 58, 138);
          padding: 1;
        }
        .parent-label {
          color: white;
          font-weight: bold;
        }
        .child-box {
          display: block;
          margin-top: 1;
          border: dashed;
          border-color: rgb(147, 197, 253);
          background-color: rgb(30, 41, 59);
          padding: 1;
        }
        .child-label {
          color: rgb(147, 197, 253);
        }

        /* 2. Interactive Button Styling */
        .hover-btn {
          display: block;
          border: tall;
          border-color: rgb(59, 130, 246);
          background-color: rgb(30, 58, 138);
          color: rgb(191, 219, 254);
          padding: 1 3;
          text-align: center;
          width: 20;
          opacity: 0.4;
          transition: opacity 0.3s ease-in-out, background-color 0.2s linear, border-color 0.2s linear;
        }
        .hover-btn:hover {
          opacity: 1.0;
          background-color: rgb(29, 78, 216);
          border-color: white;
          color: white;
        }
        .hover-btn:focus {
          border-color: rgb(147, 197, 253);
        }
      </style>
    )html";

  OpacityDemo() {
    Bind(opacity_percent);
    BindComputed(opacity_float);
  }
};

int main() {
  auto app = Ref<OpacityDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
