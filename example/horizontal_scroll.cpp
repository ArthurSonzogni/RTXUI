// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Horizontal overflow.
//
// overflow-x on a container that is narrower than its content produces a
// horizontal scrollbar.
//
// Try it: scroll with Shift+wheel, or drag the scrollbar.
#include <rtxui/rtxui.hpp>
#include <string>
#include <string_view>
#include <vector>

using namespace rtxui;

class HorizontalScrollDemo : public Component<HorizontalScrollDemo> {
 public:
  static std::vector<std::string> Numbered(std::string prefix, int n) {
    std::vector<std::string> out;
    for (int i = 1; i <= n; ++i) {
      out.push_back(prefix + " " + std::to_string(i));
    }
    return out;
  }

  std::vector<std::string> outer = Numbered("Item", 3);
  std::vector<std::string> inner = Numbered("In", 8);
  std::vector<std::string> more = {"Item 4", "Item 5", "Item 6", "Item 7"};

  std::string_view view = R"html(
      <div class="card">
        <h1>Horizontal scrolling</h1>
        <p>overflow-x: scroll on a row wider than its box. Shift+wheel, drag the bar, or focus it and use the arrows. The inner row scrolls on its own until it reaches an end.</p>

        <div class="outer" tabindex="0">
          <for each="{outer}" as="item">
            <div class="tile">{item}</div>
          </for>
          <div class="inner" tabindex="0">
            <for each="{inner}" as="item">
              <div class="tile small">{item}</div>
            </for>
          </div>
          <for each="{more}" as="item">
            <div class="tile">{item}</div>
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
        .outer, .inner {
          display: flex;
          gap: 1;
          overflow-x: scroll;
        }
        .outer {
          border: round;
          border-color: var(--border);
          padding: 0 1;
        }
        .outer:focus, .inner:focus {
          border-color: var(--accent);
        }
        .inner {
          flex-shrink: 0;
          width: 30;
          border: tall;
          border-color: rgb(52, 36, 79);
          background-color: rgb(30, 24, 45);
        }
        .tile {
          flex-shrink: 0;
          width: 12;
          border: tall;
          border-color: var(--border);
          background-color: rgb(13, 17, 23);
          padding: 1;
        }
        .small {
          width: 8;
          padding: 0 1;
          border-color: rgb(188, 140, 255);
        }
      </style>
    )html";

  HorizontalScrollDemo() {
    Bind(outer);
    Bind(inner);
    Bind(more);
  }
};

int main() {
  auto app = Ref<HorizontalScrollDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
