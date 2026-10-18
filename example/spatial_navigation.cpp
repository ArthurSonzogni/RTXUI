// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Arrow-key spatial navigation.
//
// Focus moves to the nearest focusable element in the direction pressed,
// computed from the laid-out geometry.
//
// Try it: move around the grid with the arrow keys.
#include <rtxui/rtxui.hpp>
#include <string>
#include <string_view>

using namespace rtxui;

class SpatialNavDemo : public Component<SpatialNavDemo> {
 public:
  std::string picked = "nothing yet";

  void Pick(std::string name) { picked = name; }

  std::string_view view = R"html(
      <div class="card">
        <h1>Spatial navigation</h1>
        <p>The arrow keys (or hjkl) move focus to the nearest element in that direction, by where things are drawn. Enter or Space activates.</p>

        <div class="grid">
          <div class="tile" tabindex="0" onclick="Pick(Files)">Files</div>
          <div class="tile" tabindex="0" onclick="Pick(Search)">Search</div>
          <div class="tile" tabindex="0" onclick="Pick(Git)">Git</div>
          <div class="tile" tabindex="0" onclick="Pick(Run)">Run</div>
          <div class="tile" tabindex="0" onclick="Pick(Debug)">Debug</div>
          <div class="tile" tabindex="0" onclick="Pick(Tests)">Tests</div>
          <div class="tile" tabindex="0" onclick="Pick(Docs)">Docs</div>
          <div class="tile" tabindex="0" onclick="Pick(Shell)">Shell</div>
          <div class="tile" tabindex="0" onclick="Pick(Settings)">Settings</div>
        </div>

        <div class="controls">
          <checkbox>Wrap</checkbox>
          <slider value="40" width="16" />
          <button onclick="Pick(Apply)">Apply</button>
        </div>

        <div class="readout">
          <span>picked</span>
          <span class="value">{picked}</span>
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
          width: 54;
        }
        h1 {
          color: var(--accent);
        }
        p {
          color: var(--muted);
        }
        .grid {
          display: grid;
          grid-template-columns: repeat(3, 1fr);
          gap: 1 2;
        }
        .tile {
          border: tall;
          border-color: var(--border);
          background-color: rgb(13, 17, 23);
          text-align: center;
          transition: border-color 0.1s linear;
        }
        .tile:hover {
          background-color: rgb(33, 38, 45);
        }
        .tile:focus {
          border-color: var(--accent);
          color: var(--accent);
        }
        .controls {
          display: flex;
          justify-content: space-between;
          margin-top: 1;
        }
        checkbox:focus, slider:focus, button:focus {
          color: var(--accent);
        }
        .readout {
          display: flex;
          justify-content: space-between;
          border-top: solid;
          border-color: var(--border);
          margin-top: 1;
          padding-top: 1;
          color: var(--muted);
        }
        .value {
          color: var(--accent);
          font-weight: bold;
        }
      </style>
    )html";

  SpatialNavDemo() {
    Bind(picked);
    Bind(Pick);
  }
};

int main() {
  auto component = Ref<SpatialNavDemo>::New();
  Screen screen(component);
  screen.Loop();
  return 0;
}
