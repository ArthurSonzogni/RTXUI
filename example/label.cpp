// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// <label> delegating clicks and focus to the control it names.
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class LabelDemo : public Component<LabelDemo> {
 public:
  bool cb1_checked = false;
  bool cb2_checked = false;

  std::string_view view = R"html(
      <div class="container">
        <p class="title">Interactive Label Element</p>
        <p class="desc">
          Clicking on a label delegates focus and actions to its associated input/checkbox.
        </p>

        <!-- Explicit Association using "for" -->
        <div class="section">
          <p class="section-title">1. Explicit Association (via "for" attribute)</p>
          <div class="row">
            <label class="btn-label" for="explicit-cb">Click Me (Explicit Label)</label>
            <checkbox id="explicit-cb" checked="{cb1_checked}">Checkbox 1</checkbox>
          </div>
          <p class="state-label">Checked state: <span class="value">{cb1_checked}</span></p>
        </div>

        <!-- Implicit Association using Nesting -->
        <div class="section">
          <p class="section-title">2. Implicit Association (via Nesting)</p>
          <label class="nest-label">
            Click Anywhere Here (Implicit Label)
            <checkbox checked="{cb2_checked}">Checkbox 2</checkbox>
          </label>
          <p class="state-label">Checked state: <span class="value">{cb2_checked}</span></p>
        </div>
      </div>

      <style>
        self {
          --bg: rgb(13, 17, 23);
          --surface: rgb(22, 27, 34);
          --muted: rgb(139, 148, 158);
          --accent: rgb(88, 166, 255);
          --success: rgb(63, 185, 80);

          display: block;
          padding: 1;
          background-color: var(--bg);
          color: white;
        }
        .container {
          display: block;
        }
        .title {
          display: block;
          font-weight: bold;
          color: rgb(56, 189, 248);
          margin-bottom: 1;
        }
        .desc {
          display: block;
          color: var(--muted);
          margin-bottom: 2;
        }
        .section {
          display: block;
          margin-bottom: 2;
          border: round;
          border-color: rgb(75, 85, 99);
          padding: 1;
        }
        .section-title {
          display: block;
          font-weight: bold;
          color: rgb(234, 179, 8);
          margin-bottom: 1;
        }
        .row {
          display: flex;
          align-items: center;
          gap: 4;
        }
        .btn-label {
          background-color: var(--surface);
          padding-left: 1;
          padding-right: 1;
          border: solid;
          border-color: var(--accent);
        }
        .nest-label {
          display: flex;
          align-items: center;
          gap: 2;
          background-color: var(--surface);
          padding: 1;
          border: dashed;
          border-color: var(--success);
        }
        .state-label {
          display: block;
          margin-top: 1;
          color: var(--muted);
        }
        .value {
          font-weight: bold;
          color: rgb(56, 189, 248);
        }
      </style>
    )html";

  LabelDemo() {
    Bind(cb1_checked);
    Bind(cb2_checked);
    EnableHotReload();
  }
};

int main() {
  auto app = Ref<LabelDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
