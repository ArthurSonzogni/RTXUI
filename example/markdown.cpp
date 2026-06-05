// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <rtxui/rtxui.hpp>
#include "rtxui/dom/element.hpp"
#include "rtxui/component/default_components_internal.hpp"
#include <fstream>
#include <sstream>

using namespace rtxui;

class MarkdownDemo : public Component<MarkdownDemo> {
 public:
  MarkdownDemo() {
    // Try to load the documentation file from various relative paths
    std::vector<std::string> paths = {
        "docs/guide/markdown.md",      // From project root
        "../docs/guide/markdown.md",   // From build directory
        "../../docs/guide/markdown.md" // From deep build directory
    };

    bool loaded = false;
    for (const auto& path : paths) {
      std::ifstream file(path);
      if (file.is_open()) {
        std::stringstream ss;
        ss << file.rdbuf();
        markdown_content = ss.str();
        loaded = true;
        break;
      }
    }
    
    if (!loaded) {
      markdown_content = "# Error\nCould not find `docs/guide/markdown.md`.\nMake sure you run this from the project root or build directory.";
    }
  }

  std::string markdown_content = R"md(
# Markdown in RTXUI

RTXUI now supports **Markdown** natively via the `<markdown>` component!

## Supported Features

- **Bold** and _Italic_ text
- [Links](https://github.com/ArthurSonzogni/RTXUI)
- Inline `code` and fenced code blocks:
```cpp
#include <rtxui/rtxui.hpp>
int main() {
  return 0;
}
```
- Unordered and Ordered lists
- Blockquotes
- Tables

| Metric | Baseline | Optimized |
| --- | --- | --- |
| DOM Digest | 11.70 ms | 0.53 ms |
| Layout/Paint | 3.85 ms | 0.52 ms |

> "Markdown is a lightweight markup language for creating formatted text."

## Custom Styling

You can style the generated HTML tags using the `stylesheet` property.
)md";

  std::string custom_css = R"css(
    h1 {
      color: #3b82f6;
      border-bottom: solid;
      border-color: #3b82f6;
      margin-bottom: 1;
    }

    h2 {
      color: #60a5fa;
      margin-top: 1;
      border-bottom: solid;
      border-color: #334155;
    }

    h3 {
      color: #93c5fd;
    }

    strong {
      color: #facc15;
    }

    em {
      color: #a78bfa;
    }

    code {
      color: #94a3b8;
    }

    pre {
      background-color: #1e293b;
      border: tall;
      border-color: #334155;
      display: block;
    }

    blockquote {
      border-left: heavy;
      border-color: #4b5563;
      padding-left: 2;
      font-style: italic;
      color: #9ca3af;
    }

    ul, ol {
      margin-left: 2;
      color: #d1d5db;
    }

    li {
      margin-bottom: 0;
    }

    a {
      color: #3b82f6;
      text-decoration: underline;
    }

    table {
      border: solid;
      border-color: #334155;
      margin-top: 1;
      margin-bottom: 1;
    }

    th {
      font-weight: bold;
      color: #60a5fa;
      border-bottom: solid;
      border-color: #334155;
      padding-left: 1;
      padding-right: 1;
    }

    td {
      padding-left: 1;
      padding-right: 1;
    }
  )css";

  void InitReflection() override {
    Import<rtxui::div>();
    Import<rtxui::h1>();
    Import<rtxui::p>();
    Import<rtxui::markdown>();
    Bind(markdown_content);
    Bind(custom_css);
    Component<MarkdownDemo>::InitReflection();
  }

  std::string_view view = R"html(
    <div class="container">
      <div class="preview-pane">
        <h1 class="preview-title">Preview</h1>
        <markdown
          class="md-preview"
          content="{markdown_content}"
          stylesheet="{custom_css}"
        ></markdown>
      </div>
    </div>

    <style>
      self {
        display: flex;
        flex-direction: row;
        width: 100%;
        height: 100%;
        background-color: #0f172a;
        color: #f1f5f9;
        overflow-y: scroll;
      }
      .container {
        display: flex;
        flex-direction: row;
        flex: 1;
      }
      .sidebar {
        flex: 1;
        display: flex;
        flex-direction: column;
        border-right: solid;
        border-color: #334155;
        padding: 1;
      }
      .preview-pane {
        flex: 1;
        display: flex;
        flex-direction: column;
        padding: 1;
      }
      .sidebar-title, .preview-title {
        color: #3b82f6;
        margin-bottom: 1;
      }
      .editor, .css-editor {
        flex: 1;
        margin-bottom: 1;
        border: tall;
        border-color: #334155;
        background-color: #1e293b;
      }
      .editor { min-height: 10; }
      .css-editor { min-height: 5; }
    </style>
  )html";
};

int main() {
  auto app = Ref<MarkdownDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
