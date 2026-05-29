// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class ColorBox : public Component<ColorBox> {
 public:
  struct Props {
    std::string text;
    std::string bg_color;
    std::string fg_color = "black";
  } props;

  ColorBox() {
    Bind(props.text);
    Bind(props.bg_color);
    Bind(props.fg_color);
  }

  std::string_view Setup() override {
    Import<rtxui::div>();
    return R"html(
      <div class="box-content">
        {text}
      </div>

      <style>
        self {
          display: block;
          flex-grow: 1;
        }
        .box-content {
          padding-left: 2;
          padding-right: 2;
          padding-top: 1;
          padding-bottom: 1;
          color: {fg_color};
          background-color: {bg_color};
        }
      </style>
    )html";
  }
};

class ColorDemo : public Component<ColorDemo> {
 public:
  std::string_view Setup() override {
    Import<rtxui::div>();
    Import<rtxui::h1>();
    Import<rtxui::p>();
    Import<ColorBox>();
    return R"html(
      <div class="container">
        <h1>RTXUI Color System Demo</h1>
        <p>Scroll to see standard keywords, hex codes, rgb(), and rgba() syntaxes.</p>
        
        <div class="h2">1. Standard Keywords (16-color base)</div>
        <div class="row">
          <ColorBox text="black" bg_color="black" fg_color="white"></ColorBox>
          <ColorBox text="gray" bg_color="gray" fg_color="white"></ColorBox>
          <ColorBox text="silver" bg_color="silver" fg_color="black"></ColorBox>
          <ColorBox text="white" bg_color="white" fg_color="black"></ColorBox>
        </div>
        <div class="row">
          <ColorBox text="maroon" bg_color="maroon" fg_color="white"></ColorBox>
          <ColorBox text="red" bg_color="red" fg_color="white"></ColorBox>
          <ColorBox text="purple" bg_color="purple" fg_color="white"></ColorBox>
          <ColorBox text="fuchsia" bg_color="fuchsia" fg_color="white"></ColorBox>
        </div>
        <div class="row">
          <ColorBox text="green" bg_color="green" fg_color="black"></ColorBox>
          <ColorBox text="lime" bg_color="lime" fg_color="black"></ColorBox>
          <ColorBox text="olive" bg_color="olive" fg_color="white"></ColorBox>
          <ColorBox text="yellow" bg_color="yellow" fg_color="black"></ColorBox>
        </div>
        <div class="row">
          <ColorBox text="navy" bg_color="navy" fg_color="white"></ColorBox>
          <ColorBox text="blue" bg_color="blue" fg_color="white"></ColorBox>
          <ColorBox text="teal" bg_color="teal" fg_color="white"></ColorBox>
          <ColorBox text="aqua" bg_color="aqua" fg_color="black"></ColorBox>
        </div>

        <div class="h2">2. Hex Colors (#RGB, #RRGGBB, #RGBA, #RRGGBBAA)</div>
        <div class="row">
          <ColorBox text="#f0f" bg_color="#f0f" fg_color="white"></ColorBox>
          <ColorBox text="#0ff" bg_color="#0ff" fg_color="black"></ColorBox>
          <ColorBox text="#ff3366" bg_color="#ff3366" fg_color="white"></ColorBox>
          <ColorBox text="#33cc66" bg_color="#33cc66" fg_color="black"></ColorBox>
        </div>
        <div class="row">
          <ColorBox text="#ffb3ba" bg_color="#ffb3ba" fg_color="black"></ColorBox>
          <ColorBox text="#ffdfba" bg_color="#ffdfba" fg_color="black"></ColorBox>
          <ColorBox text="#ffffba" bg_color="#ffffba" fg_color="black"></ColorBox>
          <ColorBox text="#baffc9" bg_color="#baffc9" fg_color="black"></ColorBox>
        </div>
        <div class="row">
          <ColorBox text="#bae1ff" bg_color="#bae1ff" fg_color="black"></ColorBox>
          <ColorBox text="#e8c4ff" bg_color="#e8c4ff" fg_color="black"></ColorBox>
          <ColorBox text="#ffd3e8" bg_color="#ffd3e8" fg_color="black"></ColorBox>
          <ColorBox text="#cbf3f0" bg_color="#cbf3f0" fg_color="black"></ColorBox>
        </div>
        <div class="row">
          <ColorBox text="#ff007f" bg_color="#ff007f" fg_color="white"></ColorBox>
          <ColorBox text="#39ff14" bg_color="#39ff14" fg_color="black"></ColorBox>
          <ColorBox text="#00ffcc88" bg_color="#00ffcc88" fg_color="black"></ColorBox>
          <ColorBox text="#151520" bg_color="#151520" fg_color="white"></ColorBox>
        </div>

        <div class="h2">3. RGB Colors</div>
        <div class="row">
          <ColorBox text="rgb(255,0,128)" bg_color="rgb(255,0,128)" fg_color="white"></ColorBox>
          <ColorBox text="rgb(0,255,128)" bg_color="rgb(0,255,128)" fg_color="black"></ColorBox>
          <ColorBox text="rgb(128,0,255)" bg_color="rgb(128,0,255)" fg_color="white"></ColorBox>
          <ColorBox text="rgb(255,128,0)" bg_color="rgb(255,128,0)" fg_color="black"></ColorBox>
        </div>

        <div class="h2">4. RGBA Colors with Alpha Blending</div>
        <div class="row">
          <ColorBox text="rgba(255,0,0,0.5)" bg_color="rgba(255,0,0,0.5)" fg_color="white"></ColorBox>
          <ColorBox text="rgba(0,0,255,0.3)" bg_color="rgba(0,0,255,0.3)" fg_color="white"></ColorBox>
          <ColorBox text="rgba(0,255,0,0.7)" bg_color="rgba(0,255,0,0.7)" fg_color="black"></ColorBox>
          <ColorBox text="rgba(255,255,255,0.2)" bg_color="rgba(255,255,255,0.2)" fg_color="white"></ColorBox>
        </div>
      </div>

      <style>
        self {
          display: block;
          padding: 2;
          background-color: #111118;
          color: #eee;
          height: 22;
          overflow-y: scroll;
        }
        h1 {
          color: #ffcc00;
        }
        .h2 {
          display: block;
          color: #00ccff;
          margin-top: 2;
          margin-bottom: 1;
          font-weight: bold;
        }
        p {
          margin-bottom: 1;
        }
        .container {
          display: block;
        }
        .row {
          display: flex;
          width: 100%;
          gap: 2;
          margin-bottom: 1;
        }
      </style>
    )html";
  }
};

int main() {
  auto app = Ref<ColorDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
