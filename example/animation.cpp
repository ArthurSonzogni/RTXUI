// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Keyframe animations.
//
// @keyframes plus the `animation` property drive a property through a cycle
// without any C++ state changing between frames.
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class AnimationDemo : public Component<AnimationDemo> {
 public:
  std::string_view view = R"html(
      <div class="container">
        <h1>Animation & Transition Demo</h1>
        <p>
          Hover over the elements below to see smooth C++ terminal animations.
        </p>

        <div class="card btn-card">
          <h2>Button Hover Transitions</h2>
          <div class="btn">Hover Me</div>
        </div>

        <div class="card grow-card">
          <h2>Hover Grow Effect (Flex)</h2>
          <div class="grow-container">
            <div class="grow-box grow-box-1">Box 1</div>
            <div class="grow-box grow-box-2">Grow</div>
            <div class="grow-box grow-box-3">Box 3</div>
          </div>
        </div>
      </div>

      <style>
        self {
          --bg: rgb(13, 17, 23);
          --surface: rgb(22, 27, 34);
          --text: rgb(230, 237, 243);
          --muted: rgb(139, 148, 158);
          --accent: rgb(88, 166, 255);
          --accent-bright: rgb(121, 192, 255);

          display: block;
          padding: 1 2;
          background-color: var(--bg);
          color: var(--text);
        }

        .container {
          max-width: 80;
          margin: auto
        }

        h1 {
          color: var(--accent);
          margin-bottom: 1;
        }

        h2 {
          color: var(--muted);
          margin-bottom: 1;
        }

        p {
          color: var(--muted);
          margin-bottom: 2;
        }

        .card {
          display: block;
          border: tall;
          border-color: var(--surface);
          padding: 1 2;
          height: 10;
        }

        .btn {
          display: block;
          border: tall;
          border-color: var(--accent);
          background-color: rgb(30, 58, 138);
          color: rgb(191, 219, 254);
          padding: 1 3;
          text-align: center;
          width: 16;
          transition:
            background-color 0.2s ease-in-out,
            border-color 0.2s linear,
            color 0.2s ease,
            border-color 1s linear
            ;
        }
        .btn:hover {
          background-color: rgb(29, 78, 216);
          border-color: white;
          color: white;
        }
        .btn:active {
          background-color: rgb(30, 64, 175);
          border-color: var(--accent-bright);
        }

        /* 2. Hover Grow Animations */
        .grow-container {
          display: flex;
          flex-direction: row;
          width: 100%;
          height: 3;
        }
        .grow-box {
          border: tall;
          border-color: var(--surface);
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
          background-color: var(--accent-bright);
        }
        .grow-box-3 {
          background-color: rgb(30, 58, 138);
          flex-grow: 1.0;
          transition: background-color 0.3s linear;
        }
        .grow-box-3:hover {
          background-color: var(--accent-bright);
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
