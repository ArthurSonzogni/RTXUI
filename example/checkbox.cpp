// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// The <checkbox> component bound to a bool.
//
// Try it: click the box, or focus it and press Space.
#include <rtxui/rtxui.hpp>
#include <string_view>

using namespace rtxui;

class CheckboxDemo : public Component<CheckboxDemo> {
 public:
  bool checked = false;

  std::string_view view = R"html(
      <div class="card">
        <h1>Checkbox</h1>
        <p>Click it, or focus it and press Space.</p>

        <checkbox checked="{checked}">Enable notifications</checkbox>

        <div class="readout">
          <span>checked</span>
          <span class="value">{checked}</span>
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
          height: 100%;
          background-color: rgb(13, 17, 23);
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
        checkbox:focus {
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

  CheckboxDemo() { Bind(checked); }
};

int main() {
  auto app = Ref<CheckboxDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
