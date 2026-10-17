// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// The <input> component with two-way binding.
//
// Editing the field writes straight back into the bound std::string, and the
// interpolated value below updates on the same frame.
#include <rtxui/rtxui.hpp>
#include <string>
#include <string_view>

using namespace rtxui;

class InputDemo : public Component<InputDemo> {
 public:
  std::string text = "Hello, RTXUI!";

  std::string_view view = R"html(
      <div class="card">
        <h1>Input</h1>
        <p>Two-way binding to a std::string. Ctrl moves and deletes by word.</p>

        <input value="{text}" />

        <div class="readout">
          <span>value</span>
          <span class="value">"{text}"</span>
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
        input {
          border: tall;
          border-color: var(--border);
          background-color: rgb(13, 17, 23);
          padding: 0 1;
          width: 100%;
        }
        input:focus {
          border-color: var(--accent);
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

  InputDemo() { Bind(text); }
};

int main() {
  auto app = Ref<InputDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
