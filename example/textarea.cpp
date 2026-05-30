// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <iostream>
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

  TextareaDemo() {
    Bind(text);
    BindComputed(line_count);
    BindComputed(char_count);
  }

  std::string_view Setup() override {
    return R"html(
      <div class="container">
        <p class="title">Multi-line Textarea Editor</p>
        <p class="desc">
          Arrow keys navigate. Enter inserts a newline.
          Home/End jump to line boundaries.
          Ctrl+Backspace/Delete delete words.
        </p>

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
          display: block;
          padding: 1;
          background-color: rgb(15, 23, 42);
          color: white;
          border: tall;
          border-color: rgb(99, 102, 241);
        }
        .container {
          display: block;
        }
        .title {
          display: block;
          font-weight: bold;
          color: rgb(129, 140, 248);
          margin-bottom: 1;
        }
        .desc {
          display: block;
          color: rgb(156, 163, 175);
          margin-bottom: 2;
        }
        .editor {
          display: block;
          width: 70;
          height: 12;
          border-color: rgb(99, 102, 241);
          background-color: rgb(30, 41, 59);
          color: white;
          padding-left: 1;
          padding-right: 1;
          overflow-y: scroll;
          margin-bottom: 2;
        }
        .stats {
          display: flex;
          gap: 2;
        }
        .label {
          color: rgb(129, 140, 248);
        }
        .value {
          font-weight: bold;
          color: rgb(244, 63, 94);
          margin-right: 2;
        }
      </style>
    )html";
  }
};

int main() {
  auto app = Ref<TextareaDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
