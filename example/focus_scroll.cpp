// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <iostream>
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class FocusScrollDemo : public Component<FocusScrollDemo> {
 public:
  std::string_view view = R"html(
    <div class="container">
      <h2>Keyboard Focus Scroll-Into-View Demo</h2>
      <p class="description">
        Press <b>Tab</b> or <b>Shift-Tab</b> to cycle focus through the items below.
        The scrollable container will automatically scroll to keep the focused element in view.
      </p>

      <div class="scroll-window">
        <div class="list-item" tabindex="0">Item 1 (Start)</div>
        <div class="list-item" tabindex="0">Item 2</div>
        <div class="list-item" tabindex="0">Item 3</div>
        <div class="list-item" tabindex="0">Item 4</div>
        <div class="list-item" tabindex="0">Item 5</div>
        <div class="list-item" tabindex="0">Item 6</div>
        <div class="list-item" tabindex="0">Item 7</div>
        <div class="list-item" tabindex="0">Item 8</div>
        <div class="list-item" tabindex="0">Item 9</div>
        <div class="list-item" tabindex="0">Item 10 (End)</div>
      </div>
    </div>

    <style>
      self {
        display: block;
        padding: 1 2;
        background-color: rgb(15, 23, 42); /* Deep dark slate background */
        color: rgb(241, 245, 249);
      }
      h2 {
        color: rgb(59, 130, 246); /* Bright blue */
        margin-bottom: 0;
      }
      .description {
        color: rgb(148, 163, 184); /* Muted gray text */
        margin-bottom: 2;
      }
      .scroll-window {
        display: block;
        height: 6;
        border: tall;
        border-color: rgb(71, 85, 105);
        overflow-y: scroll;
        scroll-speed: 1;
        width: 40;
      }
      .list-item {
        display: block;
        padding: 0 1;
        border: solid;
        border-color: rgb(30, 41, 59);
        background-color: rgb(30, 41, 59, 0.4);
        margin: 0;
      }
      .list-item:hover {
        background-color: rgb(30, 41, 59, 0.8);
      }
      .list-item:focus {
        border-color: rgb(16, 185, 129); /* Emerald Green focus */
        background-color: rgb(6, 95, 70, 0.5); /* Emerald background tint */
        color: rgb(255, 255, 255);
      }
    </style>
  )html";
};

int main() {
  auto app = Ref<FocusScrollDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
