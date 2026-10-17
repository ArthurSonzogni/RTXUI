// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Animating layout.
//
// A transition on flex-grow resizes boxes smoothly, and their neighbors make
// room, with no C++ state changing between frames.
#include <rtxui/rtxui.hpp>
#include <string_view>

using namespace rtxui;

class AnimationDemo : public Component<AnimationDemo> {
 public:
  std::string_view view = R"html(
      <div class="card">
        <h1>Animating layout</h1>
        <p>Hover or focus a panel: its flex-grow eases from 1 to 4, and the others make room.</p>

        <div class="row">
          <div class="panel" tabindex="0">Files</div>
          <div class="panel" tabindex="0">Search</div>
          <div class="panel" tabindex="0">Settings</div>
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
          width: 64;
        }
        h1 {
          color: var(--accent);
        }
        p {
          color: var(--muted);
        }
        .row {
          display: flex;
          gap: 1;
        }
        .panel {
          flex-grow: 1;
          flex-basis: 0;
          min-width: 12;
          border: tall;
          border-color: var(--border);
          background-color: rgb(13, 17, 23);
          padding: 1;
          text-align: center;
          transition: flex-grow 0.2s ease-in-out, border-color 0.1s linear;
        }
        .panel:hover, .panel:focus {
          flex-grow: 4;
          border-color: var(--accent);
          color: var(--accent);
        }
      </style>
    )html";
};

int main() {
  auto app = Ref<AnimationDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
