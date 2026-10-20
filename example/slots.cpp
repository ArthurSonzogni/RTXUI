// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Content projection with <slot>.
//
// A reusable Card component places its caller's markup into named slots, which
// is how every built-in tag is implemented too.
#include <rtxui/rtxui.hpp>
#include <string_view>

using namespace rtxui;

// A card with a header, a body and a footer, each filled by the caller.
class Card : public Component<Card> {
 public:
  std::string_view view = R"html(
    <div class="header">
      <slot name="header">Untitled</slot>
    </div>
    <slot></slot>
    <div class="footer">
      <slot name="footer"></slot>
    </div>

    <style>
      self {
        display: block;
        width: 50;
        border: tall;
        border-color: rgb(48, 54, 61);
        background-color: rgb(22, 27, 34);
        padding: 1 3;
      }
      .header {
        color: rgb(88, 166, 255);
        font-weight: bold;
        margin-bottom: 1;
      }
      .footer {
        border-top: solid;
        border-color: rgb(48, 54, 61);
        margin-top: 1;
        color: rgb(139, 148, 158);
      }
    </style>
  )html";
};

class SlotsDemo : public Component<SlotsDemo> {
 public:
  std::string_view view = R"html(
    <Card>
      <span slot="header">Slots</span>

      <div>Card places what it is given into its slots: this text in the default slot, the title and this footer in named ones.</div>

      <template slot="footer">Card's own markup draws the frame and the rule.</template>
    </Card>

    <style>
      self {
        display: flex;
        align-items: center;
        justify-content: center;
        width: 100%;
        color: rgb(230, 237, 243);
      }
    </style>
  )html";

  SlotsDemo() { Import<Card>(); }
};

int main() {
  auto app = Ref<SlotsDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
