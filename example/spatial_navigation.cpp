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
#include <string>
#include <vector>

#include "rtxui/rtxui.hpp"

using namespace rtxui;

class SpatialNavDemo : public Component<SpatialNavDemo> {
 public:
  std::string last_action = "None";
  int click_count = 0;

  void OnButtonClick(std::string label) {
    last_action = "Clicked: " + label;
    click_count++;
  }

  SpatialNavDemo() {
    Import("OnButtonClick",
           [this](std::string label) { OnButtonClick(label); });
    Bind(last_action);
    Bind(click_count);
  }

  std::string_view view = R"html(
    <style>
      self {

        display: flex;
        flex-direction: column;
        padding: 1;
        gap: 1;
        background-color: rgb(13, 17, 23);
        color: #f1f5f9;
      }
      h1 {
        color: #38bdf8;
        font-weight: bold;
        margin-bottom: 0;
      }
      .info {
        color: #94a3b8;
        margin-bottom: 1;
      }
      .section-title {
        color: #7dd3fc;
        font-size: small;
        margin-top: 1;
        text-decoration: underline;
      }
      .row {
        display: flex;
        gap: 2;
        align-items: center;
      }
      .card {
        border: tall;
        border-color: #1e293b;
        padding: 1;
        width: 15;
        height: 5;
        display: flex;
        flex-direction: column;
        justify-content: center;
        align-items: center;
        background-color: #1e293b;
        transition: all 0.1s linear;
      }
      .card:hover {
        background-color: #334155;
      }
      .card:focus {
        background-color: #0c4a6e;
        border-color: #38bdf8;
        color: #fff;
      }
      .card:active {
        background-color: #075985;
      }
      
      button {
        border-color: #334155;
      }
      button:focus {
        border-color: #38bdf8;
        background-color: #0c4a6e;
      }

      .nested-container {
        display: flex;
        flex-direction: column;
        gap: 1;
        border: dashed;
        border-color: #334155;
        padding: 1;
        background-color: rgba(30, 41, 59, 0.5);
      }

      .status {
        border: solid;
        border-color: #0ea5e9;
        padding: 1;
        background-color: #0c4a6e;
        color: #e0f2fe;
        margin-top: 1;
      }
      .status span {
        font-weight: bold;
        color: #fff;
      }

      .footer {
        color: #64748b;
        margin-top: 1;
        border-top: light;
        padding-top: 1;
      }
    </style>

    <div>
      <h1>RTXUI Spatial Navigation</h1>
      <p class="info">Arrow keys / hjkl to navigate • Space / Enter to activate</p>
      
      <div class="section-title">Interactive Cards</div>
      <div class="row">
        <div class="card" focusable="true" onclick="OnButtonClick('Card 1')">Card 1</div>
        <div class="card" focusable="true" onclick="OnButtonClick('Card 2')">Card 2</div>
        <div class="card" focusable="true" onclick="OnButtonClick('Card 3')">Card 3</div>
      </div>

      <div class="section-title">Standard Components</div>
      <div class="row">
        <button onclick="OnButtonClick('Button A')">Button A</button>
        <button onclick="OnButtonClick('Button B')">Button B</button>
        <button onclick="OnButtonClick('Button C')">Button C</button>
      </div>

      <div class="row">
        <checkbox>Feature</checkbox>
        <div style="display: flex; flex-direction: column; align-items: center;">
          <span style="font-size: small; color: #64748b;">Horizontal</span>
          <slider value="50" style="width: 20;" />
        </div>
        <div style="display: flex; gap: 1; align-items: center; border: solid; border-color: #1e293b; padding: 1;">
           <span style="font-size: small; color: #64748b;">Vertical</span>
           <slider direction="vertical" value="30" width="5" />
        </div>
        <input value="Search..." style="width: 15;" />
      </div>

      <div class="section-title">Complex Layout</div>
      <div class="row">
        <div class="nested-container">
          <p style="color: #94a3b8;">Nested</p>
          <button onclick="OnButtonClick('Nested 1')">Action 1</button>
          <button onclick="OnButtonClick('Nested 2')">Action 2</button>
        </div>
        <div style="flex-grow: 1; display: flex; align-items: center; justify-content: center; border: tall; border-color: #334155; height: 7; background-color: #111827;">
           <div class="card" focusable="true" onclick="OnButtonClick('Core Card')">Core Card</div>
        </div>
      </div>

      <div class="status">
        <div>Event: <span>{last_action}</span></div>
        <div>Count: <span>{click_count}</span></div>
      </div>

      <div class="footer">
        Geometric focus search ensures intuitive movement in 2D space.
      </div>
    </div>
  )html";
};

int main() {
  auto component = Ref<SpatialNavDemo>::New();
  Screen screen(component);
  screen.Loop();
  return 0;
}
