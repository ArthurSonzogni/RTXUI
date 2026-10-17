// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Foreground and background colors.
//
// Colors accept named, #rgb / #rrggbb / #rrggbbaa, rgb() and rgba() forms.
// Each swatch is drawn by a <for> loop, its color set with an inline style.
#include <rtxui/rtxui.hpp>
#include <string>
#include <string_view>
#include <vector>

using namespace rtxui;

struct Swatch {
  std::string color;
  bool light = false;  // Whether it needs dark text.
  bool operator==(const Swatch& other) const = default;
};

void Fields(const Swatch& swatch, FieldSink& out) {
  out("color", swatch.color);
  out("ink", swatch.light ? "black" : "white");
}

class ColorDemo : public Component<ColorDemo> {
 public:
  std::vector<Swatch> named = {
      {"black"},  {"gray"},       {"silver", true}, {"white", true},
      {"maroon"}, {"red"},        {"purple"},       {"fuchsia"},
      {"green"},  {"lime", true}, {"olive"},        {"yellow", true},
      {"navy"},   {"blue"},       {"teal"},         {"aqua", true},
  };
  std::vector<Swatch> hex = {
      {"#f0f"},
      {"#0ff", true},
      {"#ff3366"},
      {"#33cc66", true},
      {"#bae1ff", true},
      {"#e8c4ff", true},
      {"#00ffcc88", true},
      {"#151520"},
  };
  std::vector<Swatch> rgb = {
      {"rgb(255, 0, 128)"},
      {"rgb(0, 255, 128)", true},
      {"rgb(128, 0, 255)"},
      {"rgb(255, 128, 0)", true},
  };
  std::vector<Swatch> rgba = {
      {"rgba(255, 0, 0, 0.5)"},
      {"rgba(0, 0, 255, 0.3)"},
      {"rgba(0, 255, 0, 0.7)", true},
      {"rgba(255, 255, 255, 0.2)"},
  };

  std::string_view view = R"html(
      <div class="card">
        <h1>Colors</h1>
        <p>Every CSS color form; alpha blends with what is underneath.</p>

        <h2>Named</h2>
        <div class="swatches">
          <for each="{named}" as="s">
            <div class="swatch" style="background-color: {s.color}; color: {s.ink};">{s.color}</div>
          </for>
        </div>

        <h2>Hex</h2>
        <div class="swatches">
          <for each="{hex}" as="s">
            <div class="swatch" style="background-color: {s.color}; color: {s.ink};">{s.color}</div>
          </for>
        </div>

        <h2>rgb() and rgba()</h2>
        <div class="swatches wide">
          <for each="{rgb}" as="s">
            <div class="swatch" style="background-color: {s.color}; color: {s.ink};">{s.color}</div>
          </for>
          <for each="{rgba}" as="s">
            <div class="swatch" style="background-color: {s.color}; color: {s.ink};">{s.color}</div>
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
          width: 84;
        }
        h1 {
          color: rgb(88, 166, 255);
        }
        p, h2 {
          color: rgb(139, 148, 158);
        }
        h2 {
          margin: 1 0 0 0;
          font-weight: normal;
        }
        .swatches {
          display: grid;
          grid-template-columns: repeat(4, 1fr);
          gap: 0 1;
        }
        .wide {
          grid-template-columns: repeat(2, 1fr);
        }
        .swatch {
          padding: 0 1;
        }
      </style>
    )html";

  ColorDemo() {
    Bind(named);
    Bind(hex);
    Bind(rgb);
    Bind(rgba);
  }
};

int main() {
  auto app = Ref<ColorDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
