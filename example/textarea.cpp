// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// The <textarea> component: multi-line editing with a line-number gutter.
#include <rtxui/rtxui.hpp>
#include <string>
#include <string_view>

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
      <div class="card">
        <h1>Textarea</h1>
        <p>Multi-line editing with a gutter, bound to a std::string.</p>

        <textarea value="{text}" />

        <div class="readout">
          <span>lines <span class="value">{line_count}</span></span>
          <span>chars <span class="value">{char_count}</span></span>
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
        textarea {
          border: tall;
          border-color: var(--border);
          background-color: rgb(13, 17, 23);
          width: 100%;
          height: 8;
        }
        textarea:focus {
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

  TextareaDemo() {
    Bind(text);
    Bind(line_count);
    Bind(char_count);
  }
};

int main() {
  auto app = Ref<TextareaDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
