// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// @media queries reacting to terminal size.
//
// Try it: resize the terminal and watch the layout change breakpoint.
#include <rtxui/rtxui.hpp>
#include <string_view>

using namespace rtxui;

class MediaQueriesDemo : public Component<MediaQueriesDemo> {
 public:
  std::string_view view = R"html(
      <div class="card">
        <h1>Media queries</h1>
        <p>Resize the terminal: rules apply by its width and height.</p>

        <div class="wide">Wide: at least 60 columns.</div>
        <div class="narrow">Narrow: under 60 columns.</div>
        <div class="short">Short: 15 rows or fewer.</div>
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
        .card {
          max-width: 100%;
        }
        .wide, .narrow {
          border: tall;
          padding: 0 1;
        }
        .wide {
          border-color: rgb(63, 185, 80);
          background-color: rgb(24, 57, 37);
        }
        .narrow {
          border-color: rgb(248, 81, 73);
          background-color: rgb(73, 32, 36);
        }
        .short {
          margin-top: 1;
          color: rgb(210, 153, 34);
        }
        @media (min-width: 60) {
          .narrow {
            display: none;
          }
        }
        @media (max-width: 59) {
          .wide {
            display: none;
          }
          .card {
            padding: 1;
          }
        }
        @media (min-height: 16) {
          .short {
            display: none;
          }
        }
        @media (max-height: 15) {
          p {
            display: none;
          }
        }
      </style>
    )html";
};

int main() {
  auto app = Ref<MediaQueriesDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
