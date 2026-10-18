// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// CSS grid.
//
// grid-template sets the track sizes; items span tracks with grid-column and
// grid-row.
#include <rtxui/rtxui.hpp>
#include <string_view>

using namespace rtxui;

class GridDemo : public Component<GridDemo> {
 public:
  std::string_view view = R"html(
      <div class="card">
        <h1>Grid</h1>
        <p>grid-template sets the tracks: 4 rows / 1fr 2fr 1fr columns. Items span tracks with grid-column and grid-row.</p>

        <div class="grid">
          <div class="red wide">span 2 columns</div>
          <div class="yellow">1fr</div>
          <div class="green">1fr</div>
          <div class="blue tall">2fr, span 2 rows</div>
          <div class="purple">1fr</div>
          <div class="pink">1fr</div>
          <div class="teal">1fr</div>
          <div class="gray full">span 3 columns</div>
        </div>
      </div>

      <style>
        self {
          --border: rgb(48, 54, 61);
          --muted: rgb(139, 148, 158);
          --accent: rgb(88, 166, 255);

          display: flex;
          align-items: center;
          justify-content: center;
          width: 100%;
          color: rgb(230, 237, 243);
        }
        .card {
          border: tall;
          border-color: var(--border);
          background-color: rgb(22, 27, 34);
          padding: 1 3;
          width: 70;
        }
        h1 {
          color: var(--accent);
        }
        p {
          color: var(--muted);
        }
        .grid {
          display: grid;
          grid-template: 1fr 1fr 1fr 1fr / 1fr 2fr 1fr;
          gap: 1 2;
          height: 16;
        }
        .grid div {
          border: tall;
          padding: 0 1;
        }
        .wide {
          grid-column: span 2;
        }
        .tall {
          grid-row: span 2;
        }
        .full {
          grid-column: span 3;
        }
        .red {
          background-color: rgb(73, 32, 36);
          border-color: rgb(248, 81, 73);
        }
        .yellow {
          background-color: rgb(66, 53, 20);
          border-color: rgb(210, 153, 34);
        }
        .green {
          background-color: rgb(24, 57, 37);
          border-color: rgb(63, 185, 80);
        }
        .blue {
          background-color: rgb(22, 45, 77);
          border-color: var(--accent);
        }
        .purple {
          background-color: rgb(52, 36, 79);
          border-color: rgb(188, 140, 255);
        }
        .pink {
          background-color: rgb(73, 31, 54);
          border-color: rgb(247, 120, 186);
        }
        .teal {
          background-color: rgb(19, 59, 61);
          border-color: rgb(57, 197, 207);
        }
        .gray {
          background-color: rgb(33, 38, 45);
          border-color: var(--muted);
        }
      </style>
    )html";
};

int main() {
  auto app = Ref<GridDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
