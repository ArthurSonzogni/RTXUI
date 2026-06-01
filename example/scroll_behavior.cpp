// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <iostream>
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
        display: block;
        width: 80;
        margin: 0 auto;
        background-color: rgb(15, 23, 42);
        color: rgb(241, 245, 249);
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
        color: rgb(147, 197, 253);
        font-weight: bold;
        margin-bottom: 1;
      }
      .desc {
        color: rgb(148, 163, 184);
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
        color: rgb(148, 163, 184);
        margin-bottom: 1;
        height: 2;
      }
      .scrollbox {
        display: block;
        height: 10;
        overflow-y: scroll;
        border: tall;
        border-color: rgb(51, 65, 85);
        background-color: rgb(30, 41, 59);
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
        background-color: rgb(71, 85, 105);
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
