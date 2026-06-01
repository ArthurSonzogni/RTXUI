// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <iostream>
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class AnimationDemo : public Component<AnimationDemo> {
 public:
  std::string_view view = R"html(
      <div class="container">
        <h1>Animation & Transition Demo</h1>
        <p>Hover over the elements below to see smooth C++ terminal animations.</p>

        <div class="row">
          <div class="card btn-card">
            <h3>Button Hover Transitions</h3>
            <div class="btn">Hover Me</div>
          </div>

          <div class="card grow-card">
            <h3>Hover Grow Effect (Flex)</h3>
            <div class="grow-container">
              <div class="grow-box grow-box-1">Box 1</div>
              <div class="grow-box grow-box-2">Grow</div>
              <div class="grow-box grow-box-3">Box 3</div>
            </div>
          </div>
        </div>
      </div>

      <style>
        self {
          display: block;
          padding: 1 2;
          background-color: rgb(15, 23, 42);
          color: rgb(241, 245, 249);
        }
        h1 {
          color: rgb(59, 130, 246);
          margin-bottom: 1;
        }
        p {
          color: rgb(148, 163, 184);
          margin-bottom: 2;
        }
        .container {
          display: block;
        }
        .row {
          display: flex;
          flex-direction: row;
          gap: 4;
        }
        .card {
          display: block;
          border: tall;
          border-color: rgb(30, 41, 59);
          padding: 1 2;
          width: 20%;
          height: 10;
        }
        .grow-card {
          width: 80%;
        }
        h3 {
          color: rgb(148, 163, 184);
          margin-bottom: 1;
        }
        
        /* 1. Button Animations */
        .btn {
          display: block;
          border: tall;
          border-color: rgb(59, 130, 246);
          background-color: rgb(30, 58, 138);
          color: rgb(191, 219, 254);
          padding: 1 3;
          text-align: center;
          width: 16;
          transition: background-color 0.3s ease-in-out, border-color 0.2s linear, color 0.2s ease;
        }
        .btn:hover {
          background-color: rgb(29, 78, 216);
          border-color: rgb(96, 165, 250);
          color: rgb(255, 255, 255);
        }
        .btn:active {
          background-color: rgb(30, 64, 175);
          border-color: rgb(147, 197, 253);
        }

        /* 2. Hover Grow Animations */
        .grow-container {
          display: flex;
          flex-direction: row;
          width: 100%;
          height: 3;
        }
        .grow-box {
          border: solid;
          border-color: rgb(30, 41, 59);
          text-align: center;
          padding-top: 0;
          padding-bottom: 0;
          padding-left: 1;
          padding-right: 1;
        }
        .grow-box-1 {
          background-color: rgb(30, 58, 138);
          flex-grow: 1.0;
        }
        .grow-box-2 {
          background-color: rgb(29, 78, 216);
          flex-grow: 1.0;
          transition: flex-grow 0.4s ease-in-out, background-color 0.3s linear;
        }
        .grow-box-2:hover {
          flex-grow: 10.0;
          background-color: rgb(96, 165, 250);
        }
        .grow-box-3 {
          background-color: rgb(30, 58, 138);
          flex-grow: 1.0;
          transition: background-color 0.3s linear;
        }
        .grow-box-3:hover {
          background-color: rgb(172, 12, 255);
        }
      </style>
    )html";
};

int main() {
  auto app = Ref<AnimationDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
