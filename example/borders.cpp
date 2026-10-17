// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Every border style: solid, double, dashed, round, tall, vkey and more,
// then the ways to combine them: a color in the shorthand, a style per side.
//
// Each tile names the style it draws, so this doubles as a lookup table.
#include <rtxui/rtxui.hpp>
#include <string>
#include <string_view>
#include <vector>

using namespace rtxui;

class BordersDemo : public Component<BordersDemo> {
 public:
  std::vector<std::string> styles = {
      "solid",           "round",        "double",
      "heavy",           "dashed",       "dotted",
      "ascii",           "heavy-dashed", "double-horizontal",
      "double-vertical", "tall",         "wide",
      "thick",           "inner",        "outer",
      "panel",           "hkey",         "vkey",
      "outset",          "inset",        "block",
      "shadow",          "blank",        "shade-light",
      "shade-medium",    "shade-dark",   "none",
  };
  std::vector<std::string> combinations = {
      "border: round red",
      "border: double green",
      "border: solid; border-top: heavy",
      "border: solid; border-bottom: double yellow",
      "border-left: heavy magenta",
      "border-top: solid; border-bottom: solid",
      "border: dashed; border-left-style: heavy; border-right-style: heavy",
      "border: inset cyan",
  };

  std::string_view view = R"html(
      <div class="card">
        <h1>Borders</h1>
        <p>Every value of the border property, each drawn on itself.</p>

        <div class="grid">
          <for each="{styles}" as="s">
            <div class="tile" style="border: {s};">{s}</div>
          </for>
        </div>

        <h2>Combined</h2>
        <div class="grid combinations">
          <for each="{combinations}" as="c">
            <div class="tile" style="{c}">{c}</div>
          </for>
        </div>
      </div>

      <style>
        self {
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
          border-color: rgb(48, 54, 61);
          background-color: rgb(22, 27, 34);
          padding: 1 3;
          width: 92;
          max-height: 100%;
          overflow-y: scroll;
        }
        h1, h2 {
          color: rgb(88, 166, 255);
        }
        h2 {
          margin-top: 2;
        }
        p {
          color: rgb(139, 148, 158);
        }
        .grid {
          display: grid;
          grid-template-columns: repeat(4, 1fr);
          gap: 2 2;
        }
        .combinations {
          grid-template-columns: repeat(2, 1fr);
        }
        .tile {
          border-color: rgb(88, 166, 255);
          text-align: center;
          padding: 1;
        }
        .tile:hover {
          border-color: yellow;
          background-color:black;
        }
      </style>
    )html";

  BordersDemo() {
    Bind(styles);
    Bind(combinations);
  }
};

int main() {
  auto app = Ref<BordersDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
