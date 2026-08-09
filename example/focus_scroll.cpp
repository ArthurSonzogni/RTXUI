// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Scroll-into-view on keyboard focus.
//
// Tabbing to an element that is outside its scroll container scrolls it into
// view automatically.
//
// Try it: hold Tab and watch the list follow the focus ring.
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
        --bg: rgb(13, 17, 23);
        --surface: rgb(22, 27, 34);
        --border: rgb(48, 54, 61);
        --text: rgb(230, 237, 243);
        --muted: rgb(139, 148, 158);
        --accent: rgb(88, 166, 255);

        display: block;
        padding: 1 2;
        background-color: var(--bg); /* Deep dark slate background */
        color: var(--text);
      }
      h2 {
        color: var(--accent); /* Bright blue */
        margin-bottom: 0;
      }
      .description {
        color: var(--muted); /* Muted gray text */
        margin-bottom: 2;
      }
      .scroll-window {
        display: block;
        height: 6;
        border: tall;
        border-color: var(--border);
        overflow-y: scroll;
        scroll-speed: 1;
        width: 40;
      }
      .list-item {
        display: block;
        padding: 0 1;
        border: solid;
        border-color: var(--surface);
        background-color: rgb(30, 41, 59, 0.4);
        margin: 0;
      }
      .list-item:hover {
        background-color: rgb(30, 41, 59, 0.8);
      }
      .list-item:focus {
        border-color: var(--accent); /* Blue focus */
        background-color: rgb(30, 58, 138, 0.5); /* Blue background tint */
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
