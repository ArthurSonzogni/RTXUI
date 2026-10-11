// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Translucent colors over the terminal's own background.
//
// Nothing here paints a solid background: the panels are tints with an alpha
// channel. RTXUI asks the terminal for its background color on startup (OSC 11)
// and blends the tints with it, so they look right on a light theme as on a
// dark one. The rest of the screen is left to the terminal, so a translucent
// terminal window still shows through.
//
// Try it: run it in a dark and in a light terminal theme.
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class TranslucentDemo : public Component<TranslucentDemo> {
 public:
  std::string_view view = R"html(
      <div class="card">
        <h1>Translucent Colors</h1>
        <p>Each tint blends with your terminal's background.</p>

        <div class="row">
          <div class="swatch blue">10%</div>
          <div class="swatch blue strong">25%</div>
          <div class="swatch blue stronger">50%</div>
        </div>
        <div class="row">
          <div class="swatch green">10%</div>
          <div class="swatch green strong">25%</div>
          <div class="swatch green stronger">50%</div>
        </div>
        <div class="row">
          <div class="swatch red">10%</div>
          <div class="swatch red strong">25%</div>
          <div class="swatch red stronger">50%</div>
        </div>
      </div>

      <style>
        self {
          display: flex;
          align-items: center;
          justify-content: center;
          width: 100%;
          height: 100%;
        }
        .card {
          border: tall;
          color: rgba(128, 128, 128, 0.5);
          background-color: rgba(128, 128, 128, 0.1);
          border-color: rgba(128, 128, 128, 0.15);
          padding: 1 3;
        }
        h1 {
          font-weight: bold;
        }
        p {
          margin-bottom: 1;
        }
        .row {
          display: flex;
          gap: 2;
          margin-bottom: 1;
        }
        .swatch {
          width: 12;
          text-align: center;
          padding: 1 0;
          color: rgba(255, 255, 255, 1.0);
        }
        .blue { background-color: rgba(88, 166, 255, 0.1); }
        .blue.strong { background-color: rgba(88, 166, 255, 0.25); }
        .blue.stronger { background-color: rgba(88, 166, 255, 0.5); }
        .green { background-color: rgba(63, 185, 80, 0.1); }
        .green.strong { background-color: rgba(63, 185, 80, 0.25); }
        .green.stronger { background-color: rgba(63, 185, 80, 0.5); }
        .red { background-color: rgba(248, 81, 73, 0.1); }
        .red.strong { background-color: rgba(248, 81, 73, 0.25); }
        .red.stronger { background-color: rgba(248, 81, 73, 0.5); }
      </style>
    )html";
};

int main() {
  auto app = Ref<TranslucentDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
