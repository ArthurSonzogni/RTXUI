// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// The <select> dropdown with <option> children.
//
// Try it: focus it and press Enter or Space to open, then arrows to choose.
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class SelectDemo : public Component<SelectDemo> {
 public:
  std::string my_theme = "light";

  std::string_view view = R"html(
      <div class="container">
        <p class="title">Interactive Select & Option Elements</p>
        <p class="desc">Focus it and press Enter or Space to open, arrows to choose.</p>
        
        <select value="{my_theme}">
          <option value="dark">Dark Theme</option>
          <option value="light">Light Theme</option>
          <option value="solarized">Solarized</option>
        </select>
        
        <div class="output-box">
          <span class="label">Current Theme Value:</span>
          <span class="value">{my_theme}</span>
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

          display: flex;
          align-items: center;
          justify-content: center;
          width: 100%;
          height: 100%;
          background-color: var(--bg);
          color: var(--text);
        }
        .container {
          display: block;
          border: tall;
          border-color: var(--border);
          background-color: var(--surface);
          padding: 1 3;
          width: 62;
        }
        .title {
          color: var(--accent);
          font-weight: bold;
        }
        .desc {
          color: var(--muted);
          margin-bottom: 1;
        }
        .output-box {
          display: flex;
          justify-content: space-between;
          width: 100%;
          border-top: solid;
          border-color: var(--border);
          margin-top: 1;
          padding-top: 1;
        }
        .label {
          color: var(--muted);
        }
        .value {
          color: var(--accent);
          font-weight: bold;
        }
        .slider-wrapper, .progress-wrapper {
          display: flex;
          gap: 2;
          align-items: center;
        }
        .stats {
          color: var(--muted);
          margin-top: 1;
        }
        input, textarea, select, .styled-input, .editor {
          border: tall;
          border-color: var(--border);
          background-color: var(--bg);
          color: var(--text);
          padding: 0 1;
          width: 100%;
        }
        input:focus, textarea:focus, select:focus,
        .styled-input:focus, .editor:focus {
          border-color: var(--accent);
        }
      </style>
    )html";

  SelectDemo() {
    Bind(my_theme);
    EnableHotReload();
  }
};

int main() {
  auto app = Ref<SelectDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
