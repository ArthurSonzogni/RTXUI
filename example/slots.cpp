// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Content projection with <slot>.
//
// A reusable Card component places its caller's markup into named slots, which
// is how every built-in tag is implemented too.
#include <rtxui/rtxui.hpp>

using namespace rtxui;

// A reusable Card component with named header/footer and a default body slot.
class Card : public Component<Card> {
 public:
  std::string_view view = R"html(
    <div class="card-border">
      <div class="card-header">
        <slot.header>Default Header</slot.header>
      </div>
      <div class="card-body">
        <slot></slot>
      </div>
      <div class="card-footer">
        <slot.footer></slot.footer>
      </div>
    </div>
    <style>
      self {
        --bg: rgb(13, 17, 23);

          background-color: var(--bg);
        display: block;
        margin: 1;
      }
      .card-border {
        display: block;
        border: solid;
        border-color: #475569;
        background-color: #0f172a;
      }
      .card-header {
        display: block;
        border-bottom: dashed;
        border-color: #334155;
        padding-left: 1;
        padding-right: 1;
        font-weight: bold;
        color: #38bdf8;
      }
      .card-body {
        display: block;
        padding: 1;
        color: #e2e8f0;
      }
      .card-footer {
        display: block;
        border-top: solid;
        border-color: #334155;
        padding-left: 1;
        padding-right: 1;
        color: #94a3b8;
      }
    </style>
  )html";
};

class SlotsDemo : public Component<SlotsDemo> {
 public:
  std::string_view view = R"html(
    <div class="container">
      <h1>Component Slots & Composition</h1>
      <p>This demo showcases how custom components can define slots for default and named child content.</p>

      <Card>
        <template.header>
          <span>Custom Card Title</span>
        </template.header>

        <div class="content-block">
          <p>This body content is passed into the default slot of the Card component.</p>
          <p>It can contain arbitrary elements, nested components, or text.</p>
        </div>

        <template.footer>
          <span>Footer: Page 1 of 1</span>
        </template.footer>
      </Card>
    </div>
    <style>
      self {
        display: block;
        padding: 1;
        background-color: var(--bg);
        color: white;
        width: 100%;
        height: 100%;
      }
      h1 {
        font-weight: bold;
        color: #38bdf8;
        margin-bottom: 1;
      }
      p {
        color: #94a3b8;
        margin-bottom: 1;
      }
      .content-block {
        display: block;
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
