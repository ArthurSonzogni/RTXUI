// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Nested scroll containers and scroll chaining.
//
// An inner container consumes wheel events until it reaches its end, then the
// event propagates to its parent.
#include <rtxui/rtxui.hpp>
#include <string>
#include <string_view>
#include <vector>

using namespace rtxui;

class NestedScrollDemo : public Component<NestedScrollDemo> {
 public:
  static std::vector<std::string> Numbered(std::string prefix, int n) {
    std::vector<std::string> out;
    for (int i = 1; i <= n; ++i) {
      out.push_back(prefix + " " + std::to_string(i));
    }
    return out;
  }

  std::vector<std::string> before = Numbered("Outer line", 6);
  std::vector<std::string> inner = Numbered("Inner line", 15);
  std::vector<std::string> after = {"Outer line 7", "Outer line 8",
                                    "Outer line 9", "Outer line 10"};

  std::string_view view = R"html(
      <div class="card">
        <h1>Nested scrolling</h1>
        <p>The wheel scrolls the box under the mouse. Once that box reaches an end, it passes the scroll on to its parent.</p>

        <div class="outer">
          <for each="{before}" as="line">
            <div>{line}</div>
          </for>
          <div class="inner">
            <for each="{inner}" as="line">
              <div>{line}</div>
            </for>
          </div>
          <for each="{after}" as="line">
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
        .outer {
          height: 14;
          overflow-y: scroll;
          border: round;
          border-color: var(--border);
          padding: 0 1;
        }
        .inner {
          height: 6;
          overflow-y: scroll;
          border: tall;
          border-color: rgb(188, 140, 255);
          background-color: rgb(30, 24, 45);
          padding: 0 1;
          margin: 1 0;
        }
      </style>
    )html";

  NestedScrollDemo() {
    Bind(before);
    Bind(inner);
    Bind(after);
  }
};

int main() {
  auto app = Ref<NestedScrollDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
