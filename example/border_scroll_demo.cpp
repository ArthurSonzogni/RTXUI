// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <iostream>
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class BorderScrollDemo : public Component<BorderScrollDemo> {
 public:
  std::string border_style = "solid";
  bool show_v_scroll = true;
  bool show_h_scroll = true;

  std::string overflow_y_val = "scroll";
  std::string overflow_x_val = "scroll";

  void SelectBorder(std::string name) {
    if (name == "double-horiz") {
      border_style = "double-horizontal";
    } else if (name == "double-vert") {
      border_style = "double-vertical";
    } else {
      border_style = name;
    }
  }

  void ToggleVScroll() { show_v_scroll = !show_v_scroll; }
  void ToggleHScroll() { show_h_scroll = !show_h_scroll; }

  std::string v_scroll_class() const { return show_v_scroll ? "active" : ""; }
  std::string h_scroll_class() const { return show_h_scroll ? "active" : ""; }

#define BORDER_CLASS_FN(name, actual)              \
  std::string name##_class() const {               \
    return border_style == actual ? "active" : ""; \
  }

  BORDER_CLASS_FN(solid, "solid")
  BORDER_CLASS_FN(round, "round")
  BORDER_CLASS_FN(double_style, "double")
  BORDER_CLASS_FN(ascii, "ascii")
  BORDER_CLASS_FN(dashed, "dashed")
  BORDER_CLASS_FN(heavy, "heavy")
  BORDER_CLASS_FN(dotted, "dotted")
  BORDER_CLASS_FN(double_horizontal, "double-horizontal")
  BORDER_CLASS_FN(double_vertical, "double-vertical")
  BORDER_CLASS_FN(shadow, "shadow")
  BORDER_CLASS_FN(shade_light, "shade-light")
  BORDER_CLASS_FN(shade_medium, "shade-medium")
  BORDER_CLASS_FN(shade_dark, "shade-dark")
  BORDER_CLASS_FN(squiggle, "squiggle")
  BORDER_CLASS_FN(tall, "tall")
  BORDER_CLASS_FN(panel, "panel")
  BORDER_CLASS_FN(thick, "thick")
  BORDER_CLASS_FN(outer, "outer")
  BORDER_CLASS_FN(inner, "inner")
  BORDER_CLASS_FN(wide, "wide")
  BORDER_CLASS_FN(vkey, "vkey")
  BORDER_CLASS_FN(hkey, "hkey")
  BORDER_CLASS_FN(blank, "blank")
  BORDER_CLASS_FN(none, "none")

  std::string_view view = R"html(
      <div class="main-container">
        <div class="sidebar">
          <h2>Controls</h2>
          
          <div class="toggles">
            <button onclick="ToggleVScroll" class="{v_scroll_class}">V-Scroll</button>
            <button onclick="ToggleHScroll" class="{h_scroll_class}">H-Scroll</button>
          </div>
          
          <div class="section-title">Select Border Style:</div>
          <div class="buttons-grid">
            <div class="row">
              <button onclick="SelectBorder(solid)" class="{solid_class}">solid</button>
              <button onclick="SelectBorder(round)" class="{round_class}">round</button>
              <button onclick="SelectBorder(double)" class="{double_style_class}">double</button>
            </div>
            <div class="row">
              <button onclick="SelectBorder(ascii)" class="{ascii_class}">ascii</button>
              <button onclick="SelectBorder(dashed)" class="{dashed_class}">dashed</button>
              <button onclick="SelectBorder(heavy)" class="{heavy_class}">heavy</button>
            </div>
            <div class="row">
              <button onclick="SelectBorder(dotted)" class="{dotted_class}">dotted</button>
              <button onclick="SelectBorder(double-horiz)" class="{double_horizontal_class}">double-h</button>
              <button onclick="SelectBorder(double-vert)" class="{double_vertical_class}">double-v</button>
            </div>
            <div class="row">
              <button onclick="SelectBorder(shadow)" class="{shadow_class}">shadow</button>
              <button onclick="SelectBorder(shade-light)" class="{shade_light_class}">light</button>
              <button onclick="SelectBorder(shade-medium)" class="{shade_medium_class}">medium</button>
            </div>
            <div class="row">
              <button onclick="SelectBorder(shade-dark)" class="{shade_dark_class}">dark</button>
              <button onclick="SelectBorder(squiggle)" class="{squiggle_class}">squigg</button>
              <button onclick="SelectBorder(tall)" class="{tall_class}">tall</button>
            </div>
            <div class="row">
              <button onclick="SelectBorder(panel)" class="{panel_class}">panel</button>
              <button onclick="SelectBorder(thick)" class="{thick_class}">thick</button>
              <button onclick="SelectBorder(outer)" class="{outer_class}">outer</button>
            </div>
            <div class="row">
              <button onclick="SelectBorder(inner)" class="{inner_class}">inner</button>
              <button onclick="SelectBorder(wide)" class="{wide_class}">wide</button>
              <button onclick="SelectBorder(vkey)" class="{vkey_class}">vkey</button>
            </div>
            <div class="row">
              <button onclick="SelectBorder(hkey)" class="{hkey_class}">hkey</button>
              <button onclick="SelectBorder(blank)" class="{blank_class}">blank</button>
              <button onclick="SelectBorder(none)" class="{none_class}">none</button>
            </div>
          </div>
          
          <div class="status-box">
            <div>Active Border: <span class="highlight">{border_style}</span></div>
          </div>
        </div>

        <div class="demo-area">
          <h2>Interactive Area</h2>
          <p class="instruction">Focus the test area (Tab) and scroll/drag. Observe how borders and scrollbars mix correctly without overlapping!</p>
          
          <div class="scroll-container-wrapper">
            <div id="scrollable-content" tabindex="0">
              <div class="scroll-inner">
                <div class="line header-line">RTXUI Border & Scrollbar Test Container</div>
                <div class="line">Line 01: Focus this box (Tab) and use Arrow Keys to scroll.</div>
                <div class="line">Line 02: Click on any border style on the left to see live changes.</div>
                <div class="line">Line 03: The scrollbars sit inside the border edges cleanly.</div>
                <div class="line">Line 04: This is a very long line of content to trigger the horizontal scrollbar.</div>
                <div class="line">Line 05: Notice how the corners (top/bottom/left/right) are preserved!</div>
                <div class="line">Line 06: Both scrollbars will meet at the corner but not overlap the border.</div>
                <div class="line">Line 07: You can scroll vertically and horizontally simultaneously.</div>
                <div class="line">Line 08: Try different border styles like 'round', 'double', 'shade-dark', or 'tall'!</div>
                <div class="line">Line 09: Hello world from the RTXUI library!</div>
                <div class="line">Line 10: End of the test content area. Enjoy testing!</div>
              </div>
            </div>
          </div>
        </div>
      </div>

      <style>
        self {
          display: block;
          padding: 1;
          background-color: rgb(15, 23, 42);
          color: white;
          width: 100%;
          height: 100%;
        }
        .main-container {
          display: flex;
          width: 100%;
          height: 100%;
          gap: 2;
        }
        .sidebar {
          display: block;
          width: 48;
          flex-shrink: 0;
          border: solid;
          border-color: rgb(71, 85, 105);
          padding: 1;
        }
        h2 {
          font-weight: bold;
          color: rgb(56, 189, 248);
          margin-bottom: 1;
        }
        .demo-area {
          display: block;
          flex-grow: 1;
          border: solid;
          border-color: rgb(71, 85, 105);
          padding: 1;
        }
        .instruction {
          color: rgb(148, 163, 184);
          margin-bottom: 2;
        }
        .scroll-container-wrapper {
          display: flex;
          align-items: center;
          justify-content: center;
          width: 100%;
          height: 18;
        }
        #scrollable-content {
          display: block;
          width: 44;
          height: 13;
          border: {border_style};
          border-color: rgb(244, 63, 94);
          background-color: rgb(30, 41, 59);
          overflow-y: {overflow_y_val};
          overflow-x: {overflow_x_val};
        }
        #scrollable-content:focus {
          border-color: rgb(56, 189, 248);
        }
        .scroll-inner {
          display: block;
          width: 75;
        }
        .line {
          display: block;
          padding: 0 1;
          margin-bottom: 1;
          color: rgb(226, 232, 240);
        }
        .header-line {
          font-weight: bold;
          color: rgb(251, 146, 60);
        }
        .toggles {
          display: flex;
          gap: 1;
          margin-bottom: 2;
        }
        .toggles button {
          width: 20;
        }
        .section-title {
          display: block;
          font-weight: bold;
          margin-bottom: 1;
          color: rgb(148, 163, 184);
        }
        .buttons-grid {
          display: flex;
          flex-direction: column;
          gap: 1;
          margin-bottom: 1;
          width: 44;
        }
        .row {
          display: flex;
          width: 100%;
          gap: 1;
        }
        button {
          width: 13;
          background-color: rgb(30, 41, 59);
          color: rgb(203, 213, 225);
          border: solid;
          border-color: rgb(71, 85, 105);
          padding: 0 1;
          cursor: pointer;
        }
        button:hover {
          background-color: rgb(59, 130, 246);
          border-color: rgb(96, 165, 250);
          color: white;
        }
        button.active {
          background-color: rgb(244, 63, 94);
          border-color: rgb(251, 113, 133);
          color: white;
          font-weight: bold;
        }
        .status-box {
          display: block;
          margin-top: 1;
          border: dashed;
          border-color: rgb(100, 116, 139);
          padding: 1;
          color: rgb(148, 163, 184);
        }
        .highlight {
          color: rgb(244, 63, 94);
          font-weight: bold;
        }
      </style>
    )html";

  BorderScrollDemo() {
    Bind(border_style);
    Bind(show_v_scroll);
    Bind(show_h_scroll);
    Bind(overflow_y_val);
    Bind(overflow_x_val);

    Bind(solid_class);
    Bind(round_class);
    Bind(double_style_class);
    Bind(ascii_class);
    Bind(dashed_class);
    Bind(heavy_class);
    Bind(dotted_class);
    Bind(double_horizontal_class);
    Bind(double_vertical_class);
    Bind(shadow_class);
    Bind(shade_light_class);
    Bind(shade_medium_class);
    Bind(shade_dark_class);
    Bind(squiggle_class);
    Bind(tall_class);
    Bind(panel_class);
    Bind(thick_class);
    Bind(outer_class);
    Bind(inner_class);
    Bind(wide_class);
    Bind(vkey_class);
    Bind(hkey_class);
    Bind(blank_class);
    Bind(none_class);

    Bind(ToggleVScroll);
    Bind(ToggleHScroll);
    Bind(v_scroll_class);
    Bind(h_scroll_class);

    Bind(SelectBorder);
  }

  bool Digest() override {
    bool changed = Component::Digest();
    std::string new_overflow_y = show_v_scroll ? "scroll" : "visible";
    std::string new_overflow_x = show_h_scroll ? "scroll" : "visible";
    if (new_overflow_y != overflow_y_val || new_overflow_x != overflow_x_val) {
      overflow_y_val = new_overflow_y;
      overflow_x_val = new_overflow_x;
      changed = true;
    }
    return changed;
  }
};

int main() {
  auto app = Ref<BorderScrollDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
