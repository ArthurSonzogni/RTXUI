// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Opacity and alpha blending, including how nested opacity compounds.
#include <rtxui/rtxui.hpp>
#include <string_view>

using namespace rtxui;

class OpacityDemo : public Component<OpacityDemo> {
 public:
  int opacity_percent = 70;

  float opacity_float() const { return opacity_percent / 100.0f; }
  float nested_opacity() const { return opacity_float() * 0.5f; }

  std::string_view view = R"html(
      <div class="card">
        <h1>Opacity</h1>
        <p>An element and everything in it blend with what is behind. Nested opacity multiplies.</p>

        <div class="control">
          <slider value="{opacity_percent}" min="0" max="100" step="10" width="40" />
          <span class="value">{opacity_float}</span>
        </div>

        <div class="outer">
          opacity: {opacity_float}
          <div class="inner">opacity: 0.5 inside, so {nested_opacity} in all</div>
        </div>

        <div class="fade" tabindex="0">Hover or focus me: opacity eases to 1</div>
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
          width: 56;
        }
        h1 {
          color: var(--accent);
        }
        p {
          color: var(--muted);
        }
        .control {
          display: flex;
          justify-content: space-between;
        }
        slider:focus {
          color: var(--accent);
        }
        .value {
          color: var(--accent);
          font-weight: bold;
        }
        .outer {
          border: tall;
          border-color: var(--accent);
          background-color: rgb(22, 45, 77);
          padding: 0 2;
          margin-top: 1;
          opacity: {opacity_float};
        }
        .inner {
          border: tall;
          border-color: rgb(188, 140, 255);
          background-color: rgb(52, 36, 79);
          padding: 0 2;
          opacity: 0.5;
        }
        .fade {
          border: tall;
          border-color: var(--border);
          background-color: rgb(13, 17, 23);
          text-align: center;
          margin-top: 1;
          opacity: 0.4;
          transition: opacity 0.3s ease-in-out;
        }
        .fade:hover, .fade:focus {
          opacity: 1;
        }
      </style>
    )html";

  OpacityDemo() {
    Bind(opacity_percent);
    Bind(opacity_float);
    Bind(nested_opacity);
  }
};

int main() {
  auto app = Ref<OpacityDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
