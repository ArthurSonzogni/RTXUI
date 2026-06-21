// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <iostream>
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class StickyDemo : public Component<StickyDemo> {
 public:
  std::string_view view = R"html(
    <div class="container">
      <h2>Sticky Layout Features Demo</h2>
      <p class="description">
        Use your mouse wheel to scroll the list below.
        Notice how category headers stay pinned at the top until they are pushed out of the way by the next category.
      </p>

      <div class="scroll-window">
        <div class="section">
          <div class="sticky-header">Fruit (Category A)</div>
          <div class="item">Apple</div>
          <div class="item">Banana</div>
          <div class="item">Cherry</div>
          <div class="item">Date</div>
          <div class="item">Elderberry</div>
        </div>

        <div class="section">
          <div class="sticky-header">Vegetables (Category B)</div>
          <div class="item">Artichoke</div>
          <div class="item">Broccoli</div>
          <div class="item">Cabbage</div>
          <div class="item">Dill</div>
          <div class="item">Eggplant</div>
        </div>

        <div class="section">
          <div class="sticky-header">Desserts (Category C)</div>
          <div class="item">Apple Pie</div>
          <div class="item">Brownie</div>
          <div class="item">Cake</div>
          <div class="item">Donut</div>
          <div class="item">Eclair</div>
        </div>
      </div>
    </div>

    <style>
      self {
        display: block;
        padding: 1 2;
        background-color: rgb(18, 18, 18); /* Deep dark slate background */
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
        height: 10;
        border: tall;
        border-color: rgb(71, 85, 105);
        overflow-y: scroll;
        scroll-speed: 1;
        width: 45;
      }
      .section {
        display: block;
      }
      .sticky-header {
        position: sticky;
        top: 0;
        background-color: rgb(59, 130, 246);
        color: white;
        font-weight: bold;
        padding: 0 1;
        z-index: 10;
      }
      .item {
        display: block;
        padding: 0 2;
        border-bottom: solid;
        border-color: rgb(30, 41, 59);
        background-color: rgb(30, 41, 59, 0.4);
      }
      .item:hover {
        background-color: rgb(30, 41, 59, 0.8);
      }
    </style>
  )html";
};

int main() {
  auto app = Ref<StickyDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
