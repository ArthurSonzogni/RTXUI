// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// The <slider> component bound to an int.
//
// Try it: drag the thumb, or focus it and use the arrow keys.
#include <rtxui/rtxui.hpp>
#include <string_view>

using namespace rtxui;

class SliderDemo : public Component<SliderDemo> {
 public:
  int volume = 50;

  std::string_view view = R"html(
      <div class="card">
        <h1>Slider</h1>
        <p>Drag the thumb, or focus it and use the arrow keys.</p>

        <slider value="{volume}" min="0" max="100" step="5" width="40" />

        <div class="readout">
          <span>volume</span>
          <span class="value">{volume}%</span>
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
          width: 50;
        }
        h1 {
          color: var(--accent);
        }
        p {
          color: var(--muted);
        }
        slider:focus {
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

  SliderDemo() { Bind(volume); }
};

int main() {
  auto app = Ref<SliderDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
