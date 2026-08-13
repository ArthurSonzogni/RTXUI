// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// CSS transitions.
//
// `transition` interpolates a property between its old and new computed value
// whenever a rule stops or starts matching -- here, when :hover applies. Each
// property can carry its own duration and easing function.
//
// Try it: hover each card and watch them settle at different speeds.
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class TransitionsDemo : public Component<TransitionsDemo> {
 public:
  std::string_view view = R"html(
      <div class="card">
        <h1>Transitions</h1>
        <p>Hover a tile. Each one eases on a different curve.</p>

        <div class="row">
          <div class="tile linear">
            <span class="name">linear</span>
            <span class="time">0.2s</span>
          </div>
          <div class="tile ease">
            <span class="name">ease-in-out</span>
            <span class="time">0.5s</span>
          </div>
          <div class="tile slow">
            <span class="name">ease-out</span>
            <span class="time">1.0s</span>
          </div>
        </div>
      </div>

      <style>
        self {
          --bg: rgb(13, 17, 23);
          --border: rgb(48, 54, 61);
          --muted: rgb(139, 148, 158);
          --accent: rgb(88, 166, 255);

          display: flex;
          align-items: center;
          justify-content: center;
          width: 100%;
          height: 100%;
          background-color: var(--bg);
          color: rgb(230, 237, 243);
        }
        .card {
          border: tall;
          border-color: var(--border);
          background-color: rgb(22, 27, 34);
          padding: 1 3;
        }
        h1 {
          color: var(--accent);
          font-weight: bold;
        }
        p {
          color: var(--muted);
          margin-bottom: 1;
        }
        .row {
          display: flex;
          gap: 2;
        }
        .tile {
          display: flex;
          flex-direction: column;
          border: tall;
          border-color: var(--border);
          background-color: var(--bg);
          color: var(--muted);
          padding: 1 2;
          width: 18;
          text-align: center;
        }
        .tile:hover {
          background-color: var(--accent);
          border-color: var(--accent);
          color: var(--bg);
        }
        .name {
          font-weight: bold;
        }
        .linear {
          transition: background-color 0.2s linear, border-color 0.2s linear,
                      color 0.2s linear;
        }
        .ease {
          transition: background-color 0.5s ease-in-out,
                      border-color 0.5s ease-in-out, color 0.5s ease-in-out;
        }
        .slow {
          transition: background-color 1.0s ease-out,
                      border-color 1.0s ease-out, color 1.0s ease-out;
        }
      </style>
    )html";
};

int main() {
  auto app = Ref<TransitionsDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
