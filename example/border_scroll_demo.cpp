// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Borders and scrollbars sharing an edge.
//
// A scrollbar is laid out inside the border box, so it has to coexist with the
// border. Switch border styles to see each combination.
#include <rtxui/rtxui.hpp>
#include <string>
#include <string_view>
#include <vector>

using namespace rtxui;

class BorderScrollDemo : public Component<BorderScrollDemo> {
 public:
  std::vector<std::string> styles = {
      "solid",           "round",      "double",
      "heavy",           "dashed",     "dotted",
      "ascii",           "squiggle",   "double-horizontal",
      "double-vertical", "tall",       "wide",
      "thick",           "inner",      "outer",
      "panel",           "hkey",       "vkey",
      "shadow",          "blank",      "shade-light",
      "shade-medium",    "shade-dark", "none",
  };
  std::vector<std::string> lines = {
      "Each line is wider than the box, so it also scrolls sideways.",
      "The scrollbars meet in the corner, inside the border.",
      "Try round, double, tall, shadow, or a shade.",
      "Untick a checkbox to drop that scrollbar.",
      "Line 5",
      "Line 6",
      "Line 7",
      "Line 8",
      "Line 9",
      "Line 10",
      "Line 11",
      "The last line.",
  };
  std::string border_style = "round";
  bool vertical = true;
  bool horizontal = true;

  std::string overflow_y() const { return vertical ? "scroll" : "hidden"; }
  std::string overflow_x() const { return horizontal ? "scroll" : "hidden"; }

  std::string_view view = R"html(
      <div class="card">
        <h1>Borders and scrollbars</h1>
        <p>A scrollbar sits inside the border. Pick a style and scroll the box: Tab to it, then the arrows, or the wheel.</p>

        <div class="controls">
          <select value="{border_style}">
            <for each="{styles}" as="style">
              <option value="{style}">{style}</option>
            </for>
          </select>
          <checkbox checked="{vertical}">Vertical</checkbox>
          <checkbox checked="{horizontal}">Horizontal</checkbox>
        </div>

        <div class="box" tabindex="0">
          <for each="{lines}" as="line">
            <div>{line}</div>
          </for>
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
          height: 100%;
          background-color: rgb(13, 17, 23);
          color: rgb(230, 237, 243);
        }
        .card {
          border: tall;
          border-color: var(--border);
          background-color: rgb(22, 27, 34);
          padding: 1 3;
          width: 60;
        }
        h1 {
          color: var(--accent);
        }
        p {
          color: var(--muted);
        }
        .controls {
          display: flex;
          gap: 3;
          margin-bottom: 1;
        }
        select {
          width: 22;
        }
        select::part(dropdown-list) {
          max-height: 10;
          overflow-y: scroll;
        }
        select:focus, checkbox:focus {
          color: var(--accent);
        }
        .box {
          height: 10;
          border: {border_style};
          border-color: var(--border);
          overflow-y: {overflow_y};
          overflow-x: {overflow_x};
        }
        .box:focus {
          border-color: var(--accent);
        }
        .box div {
          white-space: nowrap;
          padding: 0 1;
        }
      </style>
    )html";

  BorderScrollDemo() {
    Bind(styles);
    Bind(lines);
    Bind(border_style);
    Bind(vertical);
    Bind(horizontal);
    Bind(overflow_y);
    Bind(overflow_x);
  }
};

int main() {
  auto app = Ref<BorderScrollDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
