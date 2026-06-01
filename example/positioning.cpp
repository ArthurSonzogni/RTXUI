// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <rtxui/rtxui.hpp>
#include <string>

using namespace rtxui;

class PositioningApp : public Component<PositioningApp> {
 public:
  // State
  int box_x = 10;
  int box_y = 4;
  std::string info_text = "Use buttons to move the absolute red card.";

  // Callback
  void MoveLeft() {
    if (box_x > 1) {
      box_x -= 2;
    }
  }

  void MoveRight() {
    if (box_x < 42) {
      box_x += 2;
    }
  }

  void MoveUp() {
    if (box_y > 1) {
      box_y -= 1;
    }
  }

  void MoveDown() {
    if (box_y < 9) {
      box_y += 1;
    }
  }

  // View
  std::string_view view = R"html(
    <div class="screen-container">
      <div class="header">
        <h1>RTXUI Positioning Demo</h1>
        <p>This demo showcases <strong>absolute</strong>, <strong>fixed</strong> positioning, and <strong>z-index</strong> stacking layers.</p>
      </div>

      <div class="row">
        <!-- Control Panel -->
        <div class="controls">
          <h3>Controls</h3>
          <div class="control-box">
            <div class="control-row">
              <button onclick="MoveLeft">◀ Left</button>
              <button onclick="MoveRight">Right ▶</button>
            </div>
            <div class="control-row">
              <button onclick="MoveUp">▲ Up</button>
              <button onclick="MoveDown">▼ Down</button>
            </div>
          </div>
          <div class="info-panel">
            Position: ({box_x}, {box_y})
          </div>
        </div>

        <!-- Relative Parent Container -->
        <div class="layout-area">
          <!-- Static / normal flow background block -->
          <div class="background-desc">
            This area (blue border) is a relative container. Items inside can be positioned absolutely inside it.
          </div>

          <!-- Midground block with z-index: 5 -->
          <div class="midground-card">
            <span class="card-label">Midground (Z-Index = 5)</span>
          </div>

          <!-- Interactive Absolute block with z-index: 10 -->
          <div class="absolute-card">
            <span class="card-title">Absolute (Z-Index = 10)</span>
            <span class="card-coord">X:{box_x} Y:{box_y}</span>
          </div>
        </div>
      </div>

      <!-- Fixed Status Bar at the bottom of the screen with z-index: 100 -->
      <div class="fixed-status">
        [FIXED STATUS BAR] Current Position: ({box_x}, {box_y}) | RTXUI Layout Engine
      </div>

      <!-- Scrolling filler block to showcase fixed vs absolute positioning -->
      <div class="scroll-filler">
        <h3>Scrolling Demonstration</h3>
        <p>Scroll down to see that the absolute layout area scrolls out of view, while the status bar at the bottom remains fixed in place.</p>
        <p>Scroll down further...</p>
        <p>Line A</p>
        <p>Line B</p>
        <p>Line C</p>
        <p>Line D</p>
        <p>Line E</p>
        <p>Line F</p>
        <p>Line G</p>
        <p>Line H</p>
        <p>Line I</p>
        <p>Line J</p>
      </div>
    </div>

    <style>
      self {
        display: block;
        padding: 1 2;
        background-color: rgb(15, 23, 42);
        color: rgb(241, 245, 249);
        width: 100%;
        height: 100%;
        overflow-y: scroll;
      }
      .screen-container {
        display: block;
        width: 100%;
      }
      .header {
        display: block;
        margin-bottom: 2;
      }
      h1 {
        color: rgb(59, 130, 246);
        font-weight: bold;
      }
      h3 {
        color: rgb(148, 163, 184);
        margin-bottom: 1;
      }
      p {
        color: rgb(148, 163, 184);
      }
      .row {
        display: flex;
        flex-direction: row;
        gap: 4;
      }
      .controls {
        display: block;
        border: tall;
        border-color: rgb(71, 85, 105);
        padding: 1 2;
        width: 25;
        height: 12;
      }
      .control-box {
        display: flex;
        flex-direction: column;
        gap: 1;
        margin-bottom: 1;
      }
      .control-row {
        display: flex;
        flex-direction: row;
        gap: 1;
      }
      button {
        background-color: rgb(30, 41, 59);
        color: white;
        border: solid;
        border-color: rgb(71, 85, 105);
        padding: 0 1;
        text-align: center;
      }
      button:hover {
        background-color: rgb(59, 130, 246);
        border-color: rgb(96, 165, 250);
      }
      .info-panel {
        color: rgb(56, 189, 248);
        font-weight: bold;
      }
      
      /* Positioning Area */
      .layout-area {
        position: relative;
        display: block;
        width: 60;
        height: 15;
        border: solid;
        border-color: rgb(59, 130, 246);
        background-color: rgb(30, 41, 59);
      }
      .background-desc {
        display: block;
        color: rgb(148, 163, 184);
        padding: 1;
      }
      .midground-card {
        position: absolute;
        top: 5;
        left: 20;
        width: 25;
        height: 6;
        background-color: rgb(15, 23, 42);
        border: double;
        border-color: rgb(96, 165, 250);
        z-index: 5;
        padding: 1;
      }
      .card-label {
        color: rgb(96, 165, 250);
      }
      
      /* Target of our absolute moving coordinate state variables */
      .absolute-card {
        position: absolute;
        top: {box_y};
        left: {box_x};
        width: 15;
        height: 4;
        background-color: rgb(220, 38, 38);
        border: double;
        border-color: rgb(248, 113, 113);
        z-index: 10;
        padding: 0 1;
        display: block;
      }
      .card-title {
        color: white;
        font-weight: bold;
        display: block;
      }
      .card-coord {
        color: rgb(254, 205, 211);
        display: block;
      }
      
      /* Fixed layout card */
      .fixed-status {
        position: fixed;
        bottom: 0;
        left: 0;
        width: 100%;
        background-color: rgb(79, 70, 229);
        color: white;
        padding-left: 2;
        z-index: 100;
      }
      .scroll-filler {
        display: block;
        margin-top: 5;
        margin-bottom: 5;
      }
    </style>
  )html";

  // Constructor
  PositioningApp() {
    Bind(box_x);
    Bind(box_y);
    Bind(MoveLeft);
    Bind(MoveRight);
    Bind(MoveUp);
    Bind(MoveDown);
  }
};

int main() {
  auto app = Ref<PositioningApp>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
