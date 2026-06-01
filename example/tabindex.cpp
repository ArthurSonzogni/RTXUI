// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <iostream>
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class TabIndexDemo : public Component<TabIndexDemo> {
 public:
  std::string_view view = R"html(
    <div class="container">
      <h2>Keyboard TabIndex Navigation</h2>
      <p class="description">
        Use <b>Tab</b> and <b>Shift-Tab</b> to move the focus.
        Notice that positive tabindexes are traversed first (ascending order),
        followed by tabindex 0, and tabindex -1 is skipped from tabbing.
      </p>

      <div class="list">
        <div class="item positive-two" tabindex="2">
          <span class="badge badge-pos">tabindex="2"</span>
          <span class="label">First item in HTML, but second in Tab order</span>
        </div>

        <div class="item zero-one" tabindex="0">
          <span class="badge badge-zero">tabindex="0"</span>
          <span class="label">Zero tabindex (navigated third)</span>
        </div>

        <div class="item positive-one" tabindex="1">
          <span class="badge badge-pos">tabindex="1"</span>
          <span class="label">First element in Tab order (tabindex=1)</span>
        </div>

        <div class="item minus-one" tabindex="-1">
          <span class="badge badge-neg">tabindex="-1"</span>
          <span class="label">Skipped during tab navigation (click-only)</span>
        </div>

        <div class="item implicit-zero" focusable="true">
          <span class="badge badge-implicit">focusable="true"</span>
          <span class="label">Implicitly tabindex 0 (navigated fourth)</span>
        </div>
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
        margin-bottom: 1;
      }
      .list {
        display: block;
      }
      .item {
        display: block;
        border: tall;
        border-color: rgb(30, 41, 59); /* Dark slate border */
        background-color: rgb(30, 41, 59, 0.4);
        padding: 0 1;
        margin-bottom: 0;
        width: 60;
      }
      .item:hover {
        background-color: rgb(30, 41, 59, 0.8);
        border-color: rgb(71, 85, 105);
      }
      .item:focus {
        border-color: rgb(59, 130, 246); /* Focus highlight */
        background-color: rgb(30, 58, 138, 0.5); /* Deep slate blue on focus */
      }
      .badge {
        display: inline;
        padding: 0 1;
        text-align: center;
        width: 18;
      }
      .badge-pos {
        background-color: rgb(16, 185, 129); /* Emerald Green */
        color: rgb(255, 255, 255);
      }
      .badge-zero {
        background-color: rgb(245, 158, 11); /* Amber Orange */
        color: rgb(255, 255, 255);
      }
      .badge-neg {
        background-color: rgb(239, 68, 68); /* Rose Red */
        color: rgb(255, 255, 255);
      }
      .badge-implicit {
        background-color: rgb(99, 102, 241); /* Indigo */
        color: rgb(255, 255, 255);
      }
      .label {
        display: inline;
        margin-left: 2;
        color: rgb(226, 232, 240);
      }
      .item:focus .label {
        color: rgb(255, 255, 255);
        font-weight: bold;
      }
    </style>
  )html";
};

int main() {
  auto app = Ref<TabIndexDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
