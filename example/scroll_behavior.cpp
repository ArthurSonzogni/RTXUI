// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// scroll-behavior: smooth versus auto.
//
// Try it: click the jump buttons and compare how each column travels.
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class ScrollBehaviorDemo : public Component<ScrollBehaviorDemo> {
 public:
  std::string_view view = R"html(
    <div class="container">
      <div class="header">
        <h1>RTXUI Scroll Behavior Demo</h1>
        <p class="desc">
          Compare the two scroll behaviors below using the mouse wheel, Arrow keys, or PageUp/PageDown.
        </p>
      </div>

      <div class="row">
        <div class="column">
          <h2>scroll-behavior: auto (Instant Content)</h2>
          <p class="subtitle">Content snaps instantly, scrollbar thumb glides smoothly.</p>
          <div id="scroll-auto" class="scrollbox">
            <div class="item">Item 1</div>
            <div class="item">Item 2</div>
            <div class="item">Item 3</div>
            <div class="item">Item 4</div>
            <div class="item">Item 5</div>
            <div class="item">Item 6</div>
            <div class="item">Item 7</div>
            <div class="item">Item 8</div>
            <div class="item">Item 9</div>
            <div class="item">Item 10</div>
            <div class="item">Item 11</div>
            <div class="item">Item 12</div>
          </div>
        </div>

        <div class="column">
          <h2>scroll-behavior: smooth (Smooth Content)</h2>
          <p class="subtitle">Both viewport content and scrollbar thumb glide smoothly.</p>
          <div id="scroll-smooth" class="scrollbox">
            <div class="item">Item 1</div>
            <div class="item">Item 2</div>
            <div class="item">Item 3</div>
            <div class="item">Item 4</div>
            <div class="item">Item 5</div>
            <div class="item">Item 6</div>
            <div class="item">Item 7</div>
            <div class="item">Item 8</div>
            <div class="item">Item 9</div>
            <div class="item">Item 10</div>
            <div class="item">Item 11</div>
            <div class="item">Item 12</div>
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
        margin-bottom: 1;
        height: 2;
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
    </style>
  )html";
};

int main() {
  auto component = Ref<ScrollBehaviorDemo>::New();
  Screen screen(component);
  screen.Loop();
  return 0;
}
