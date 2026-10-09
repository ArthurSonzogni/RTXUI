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

using namespace rtxui;

class ScrollBehaviorDemo : public Component<ScrollBehaviorDemo> {
 public:
  std::string_view view = R"html(
    <div class="container">
      <div class="header">
        <h1>RTXUI Scroll Behavior Demo</h1>
        <p class="desc">
          Click a jump link, or Tab through the items: the left box jumps, the right one glides. The wheel and the keys scroll both at once.
        </p>
      </div>

      <div class="row">
        <div class="column">
          <h2>scroll-behavior: auto</h2>
          <p class="subtitle">Jumps straight to the target.</p>
          <p class="jump">
            <a href="#auto-last">↓ Last item</a>
            <a href="#auto-first">↑ First item</a>
          </p>
          <div id="scroll-auto" class="scrollbox">
            <div class="item" id="auto-first" tabindex="0">Item 1</div>
            <div class="item" tabindex="0">Item 2</div>
            <div class="item" tabindex="0">Item 3</div>
            <div class="item" tabindex="0">Item 4</div>
            <div class="item" tabindex="0">Item 5</div>
            <div class="item" tabindex="0">Item 6</div>
            <div class="item" tabindex="0">Item 7</div>
            <div class="item" tabindex="0">Item 8</div>
            <div class="item" tabindex="0">Item 9</div>
            <div class="item" tabindex="0">Item 10</div>
            <div class="item" tabindex="0">Item 11</div>
            <div class="item" id="auto-last" tabindex="0">Item 12</div>
          </div>
        </div>

        <div class="column">
          <h2>scroll-behavior: smooth</h2>
          <p class="subtitle">Glides to the target.</p>
          <p class="jump">
            <a href="#smooth-last">↓ Last item</a>
            <a href="#smooth-first">↑ First item</a>
          </p>
          <div id="scroll-smooth" class="scrollbox">
            <div class="item" id="smooth-first" tabindex="0">Item 1</div>
            <div class="item" tabindex="0">Item 2</div>
            <div class="item" tabindex="0">Item 3</div>
            <div class="item" tabindex="0">Item 4</div>
            <div class="item" tabindex="0">Item 5</div>
            <div class="item" tabindex="0">Item 6</div>
            <div class="item" tabindex="0">Item 7</div>
            <div class="item" tabindex="0">Item 8</div>
            <div class="item" tabindex="0">Item 9</div>
            <div class="item" tabindex="0">Item 10</div>
            <div class="item" tabindex="0">Item 11</div>
            <div class="item" id="smooth-last" tabindex="0">Item 12</div>
          </div>
        </div>
      </div>
    </div>

    <style>
      self {
        --border: rgb(48, 54, 61);
        --muted: rgb(139, 148, 158);

        display: block;
        width: 80;
        margin: 0 auto;
        background-color: rgb(13, 17, 23);
        color: rgb(230, 237, 243);
      }
      .container {
        display: block;
        padding: 2;
      }
      .header {
        display: block;
        margin-bottom: 2;
      }
      h1 {
        color: rgb(121, 192, 255);
        font-weight: bold;
        margin-bottom: 1;
      }
      .desc {
        color: var(--muted);
      }
      .row {
        display: flex;
        flex-direction: row;
        width: 100%;
      }
      .column {
        display: block;
        flex-grow: 1;
        width: 48%;
        margin-right: 2%;
      }
      h2 {
        color: rgb(224, 242, 254);
        font-weight: bold;
        margin-bottom: 1;
      }
      .subtitle {
        color: var(--muted);
      }
      .jump {
        display: flex;
        gap: 3;
        margin-bottom: 1;
      }
      .scrollbox {
        display: block;
        height: 10;
        overflow-y: scroll;
        border: tall;
        border-color: var(--border);
        background-color: rgb(22, 27, 34);
        padding: 1;
      }
      #scroll-auto {
        scroll-behavior: auto;
      }
      #scroll-smooth {
        scroll-behavior: smooth;
      }
      .item {
        display: block;
        height: 2;
        margin-bottom: 1;
        background-color: var(--border);
        color: rgb(255, 255, 255);
        padding-left: 1;
      }
      .item:focus {
        background-color: rgb(31, 111, 235);
      }
    </style>
  )html";
};

int main() {
  auto component = Ref<ScrollBehaviorDemo>::New();
  Screen screen(component);
  screen.Loop();
  return 0;
}
