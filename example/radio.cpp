// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// <radio> buttons sharing a `name`, with the selection bound to a C++ string.
#include <rtxui/rtxui.hpp>
#include <string>
#include <string_view>

using namespace rtxui;

class RadioDemo : public Component<RadioDemo> {
 public:
  bool select_a = false;
  bool select_b = false;
  bool select_c = false;

  std::string selected_tech() const {
    if (select_a) {
      return "PostgreSQL";
    }
    if (select_b) {
      return "MongoDB";
    }
    if (select_c) {
      return "Redis";
    }
    return "none";
  }

  std::string_view view = R"html(
      <div class="card">
        <h1>Radio</h1>
        <p>Radios sharing a name form one group: one is checked at a time.</p>

        <div class="group">
          <radio name="db" checked="{select_a}">PostgreSQL</radio>
          <radio name="db" checked="{select_b}">MongoDB</radio>
          <radio name="db" checked="{select_c}">Redis</radio>
        </div>

        <div class="readout">
          <span>selected</span>
          <span class="value">{selected_tech}</span>
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
          width: 50;
        }
        h1 {
          color: var(--accent);
        }
        p {
          color: var(--muted);
        }
        .group {
          display: flex;
          flex-direction: column;
        }
        radio:focus, radio:checked {
          color: var(--accent);
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

  RadioDemo() {
    Bind(select_a);
    Bind(select_b);
    Bind(select_c);
    Bind(selected_tech);
  }
};

int main() {
  auto app = Ref<RadioDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
