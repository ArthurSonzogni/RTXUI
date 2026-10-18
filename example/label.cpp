// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// <label> delegating clicks and focus to the control it names.
#include <rtxui/rtxui.hpp>
#include <string_view>

using namespace rtxui;

class LabelDemo : public Component<LabelDemo> {
 public:
  bool notify = false;
  bool sound = true;

  std::string_view view = R"html(
      <div class="card">
        <h1>Label</h1>
        <p>Clicking a label acts on the control it belongs to.</p>

        <h2>for="id" names the control</h2>
        <div class="row">
          <label for="notify">Click this label</label>
          <checkbox id="notify" checked="{notify}">Notifications</checkbox>
        </div>

        <h2>or the label wraps it</h2>
        <label class="wrapper">
          Click anywhere on this line
          <checkbox checked="{sound}">Sound</checkbox>
        </label>

        <div class="readout">
          <span>notifications <span class="value">{notify}</span></span>
          <span>sound <span class="value">{sound}</span></span>
        </div>
      </div>

      <style>
        self {
          --border: rgb(48, 54, 61);
          --muted: rgb(139, 148, 158);
          --accent: rgb(88, 166, 255);

          display: flex;
          align-items: center;
          justify-content: center;
          width: 100%;
          color: rgb(230, 237, 243);
        }
        .card {
          border: tall;
          border-color: var(--border);
          background-color: rgb(22, 27, 34);
          padding: 1 3;
          width: 54;
        }
        h1 {
          color: var(--accent);
        }
        p {
          color: var(--muted);
        }
        h2 {
          margin: 1 0 0 0;
          font-weight: normal;
          color: var(--muted);
        }
        .row {
          display: flex;
          gap: 3;
        }
        label {
          color: var(--accent);
        }
        label:hover {
          text-decoration: underline;
        }
        .wrapper {
          display: flex;
          justify-content: space-between;
        }
        .readout {
          display: flex;
          justify-content: space-between;
          border-top: solid;
          border-color: var(--border);
          margin-top: 1;
          padding-top: 1;
          color: var(--muted);
        }
        .value {
          color: var(--accent);
          font-weight: bold;
        }
      </style>
    )html";

  LabelDemo() {
    Bind(notify);
    Bind(sound);
  }
};

int main() {
  auto app = Ref<LabelDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
