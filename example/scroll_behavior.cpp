// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// scroll-behavior: smooth versus auto.
//
// scroll-behavior decides how a box scrolls to show something it was asked
// to: an anchor link's target, or an element focused from the keyboard.
// `smooth` glides there, `auto` jumps. The mouse wheel and the scrolling keys
// always scroll at once, whatever the box says.
//
// Try it: click the jump links above each box, or Tab through the items, and
// compare how each column travels.
#include <rtxui/rtxui.hpp>
#include <string>
#include <string_view>
#include <vector>

using namespace rtxui;

class ScrollBehaviorDemo : public Component<ScrollBehaviorDemo> {
 public:
  std::vector<std::string> items = {
      "Item 2", "Item 3", "Item 4", "Item 5",  "Item 6",
      "Item 7", "Item 8", "Item 9", "Item 10", "Item 11",
  };

  std::string_view view = R"html(
      <div class="card">
        <h1>scroll-behavior</h1>
        <p>How a box scrolls to show a link's target or a focused item: auto jumps, smooth glides. The wheel and the keys always scroll at once.</p>

        <div class="row">
          <div class="column">
            <h2>auto</h2>
            <div class="links">
              <a href="#auto-last">↓ Last item</a>
              <a href="#auto-first">↑ First item</a>
            </div>
            <div class="box auto">
              <div class="item" id="auto-first" tabindex="0">First item</div>
              <for each="{items}" as="item">
                <div class="item" tabindex="0">{item}</div>
              </for>
              <div class="item" id="auto-last" tabindex="0">Last item</div>
            </div>
          </div>
          <div class="column">
            <h2>smooth</h2>
            <div class="links">
              <a href="#smooth-last">↓ Last item</a>
              <a href="#smooth-first">↑ First item</a>
            </div>
            <div class="box smooth">
              <div class="item" id="smooth-first" tabindex="0">First item</div>
              <for each="{items}" as="item">
                <div class="item" tabindex="0">{item}</div>
              </for>
              <div class="item" id="smooth-last" tabindex="0">Last item</div>
            </div>
          </div>
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
          gap: 3;
        }
        .column {
          flex: 1;
        }
        h2 {
          margin-bottom: 0;
        }
        .links {
          display: flex;
          justify-content: space-between;
        }
        a {
          color: var(--accent);
          text-decoration: none;
        }
        a:hover, a:focus {
          text-decoration: underline;
        }
        .box {
          height: 10;
          overflow-y: scroll;
          border: round;
          border-color: var(--border);
        }
        .auto {
          scroll-behavior: auto;
        }
        .smooth {
          scroll-behavior: smooth;
        }
        .item {
          padding: 0 1;
        }
        .item:focus {
          background-color: rgb(22, 45, 77);
          color: var(--accent);
        }
      </style>
    )html";

  ScrollBehaviorDemo() { Bind(items); }
};

int main() {
  auto component = Ref<ScrollBehaviorDemo>::New();
  Screen screen(component);
  screen.Loop();
  return 0;
}
