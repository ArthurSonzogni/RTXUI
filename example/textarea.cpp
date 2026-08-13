// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// The <textarea> component: multi-line editing with a line-number gutter.
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class TextareaDemo : public Component<TextareaDemo> {
 public:
  std::string text =
      "This is a multi-line\n"
      "text editor powered by RTXUI.\n"
      "\n"
      "Try editing:\n"
      "  - Arrow keys to move\n"
      "  - Enter to insert newlines\n"
      "  - Home/End to jump to line start/end\n"
      "  - Ctrl+Backspace/Delete to delete words";

  int line_count() const {
    int n = 1;
    for (char c : text) {
      if (c == '\n') {
        ++n;
      }
    }
    return n;
  }
  int char_count() const { return static_cast<int>(text.size()); }

  std::string_view view = R"html(
      <div class="container">
        <p class="title">Multi-line Textarea Editor</p>
        <p class="desc">Multi-line editing with a gutter, bound to a std::string.</p>

        <textarea class="editor" value="{text}" />

        <div class="stats">
          <span class="label">Lines:</span>
          <span class="value">{line_count}</span>
          <span class="label">Chars:</span>
          <span class="value">{char_count}</span>
        </div>
      </div>

      <style>
        self {
          --bg: rgb(13, 17, 23);
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
          background-color: rgb(22, 27, 34);
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

  TextareaDemo() {
    Bind(text);
    BindComputed(line_count);
    BindComputed(char_count);
  }
};

int main() {
  auto app = Ref<TextareaDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
