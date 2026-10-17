// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// position: relative, absolute and fixed, plus z-index stacking.
#include <algorithm>
#include <rtxui/rtxui.hpp>
#include <string>
#include <string_view>

using namespace rtxui;

class PositioningApp : public Component<PositioningApp> {
 public:
  int x = 8;
  int y = 4;

  void Move(std::string direction) {
    if (direction == "left") {
      x = std::max(0, x - 2);
    } else if (direction == "right") {
      x = std::min(40, x + 2);
    } else if (direction == "up") {
      y = std::max(0, y - 1);
    } else if (direction == "down") {
      y = std::min(5, y + 1);
    }
  }

  std::string_view view = R"html(
      <div class="card">
        <h1>Positioning</h1>
        <p>The stage is position: relative; the cards in it are absolute, stacked by z-index. The status bar is fixed to the screen.</p>

        <div class="buttons">
          <button onclick="Move(left)">◀</button>
          <button onclick="Move(right)">▶</button>
          <button onclick="Move(up)">▲</button>
          <button onclick="Move(down)">▼</button>
        </div>

        <div class="stage">
          <div class="back">z-index: 5</div>
          <div class="front">z-index: 10</div>
        </div>
      </div>

      <div class="status">position: fixed; bottom: 0   The front card is at left: {x}, top: {y}.</div>

      <style>
        self {
          --border: rgb(48, 54, 61);
          --muted: rgb(139, 148, 158);
          --accent: rgb(88, 166, 255);

          display: flex;
          align-items: center;
          justify-content: center;
          width: 100%;
          height: 100%;
          background-color: rgb(13, 17, 23);
          color: rgb(230, 237, 243);
        }
        .card {
          border: tall;
          border-color: var(--border);
          background-color: rgb(22, 27, 34);
          padding: 1 3;
          width: 66;
        }
        h1 {
          color: var(--accent);
        }
        p {
          color: var(--muted);
        }
        .buttons {
          display: flex;
          gap: 1;
        }
        button {
          border: tall;
          border-color: var(--border);
          background-color: rgb(13, 17, 23);
          padding: 0 1;
        }
        button:hover, button:focus {
          border-color: var(--accent);
          color: var(--accent);
        }
        .stage {
          position: relative;
          height: 12;
          margin-top: 1;
          border: round;
          border-color: var(--border);
        }
        .back, .front {
          position: absolute;
          width: 18;
          height: 5;
          border: tall;
          padding: 1;
        }
        .back {
          top: 2;
          left: 24;
          z-index: 5;
          text-align: right;
          border-color: rgb(188, 140, 255);
          background-color: rgb(52, 36, 79);
        }
        .front {
          top: {y};
          left: {x};
          z-index: 10;
          border-color: rgb(248, 81, 73);
          background-color: rgb(73, 32, 36);
        }
        .status {
          position: fixed;
          bottom: 0;
          left: 0;
          width: 100%;
          padding: 0 1;
          background-color: rgb(22, 45, 77);
          color: var(--accent);
        }
      </style>
    )html";

  PositioningApp() {
    Bind(x);
    Bind(y);
    Bind(Move);
  }
};

int main() {
  auto app = Ref<PositioningApp>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
