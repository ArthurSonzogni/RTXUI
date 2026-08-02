// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <string>

#include "rtxui/component/default_components_internal.hpp"
#include "rtxui/rtxui.hpp"

using namespace rtxui;

namespace {

constexpr std::string_view kDefaultMarkdown = R"md(
# Markdown Playground

Edit the **Markdown** on the left. This preview re-renders on every
keystroke, using the same `<markdown>` component every RTXUI app can embed.

## Features

- **Bold** and _italic_ text
- [Links](https://github.com/ArthurSonzogni/RTXUI)
- Inline `code` and fenced code blocks:

```cpp
#include <rtxui/rtxui.hpp>
int main() {
  return 0;
}
```

- Ordered and unordered lists
- Blockquotes

> Style the generated tags in the Stylesheet editor below.

| Metric | Value |
| --- | --- |
| DOM Digest | 0.53 ms |
| Layout/Paint | 0.52 ms |
)md";

constexpr std::string_view kDefaultStylesheet = R"css(
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
  strong { color: #facc15; }
  em { color: #a78bfa; }
  code { color: #94a3b8; }
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
  ul, ol { margin-left: 2; color: #d1d5db; }
  a { color: #3b82f6; text-decoration: underline; }
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
  td { padding-left: 1; padding-right: 1; }
)css";

}  // namespace

// Left editor + right preview, mirroring example/playground.cpp: two
// `<textarea>`s hold the raw Markdown source and its stylesheet, and the
// `<markdown>` component on the right re-renders both reactively -- no
// manual reparse/HotReload plumbing needed, since `content`/`stylesheet` are
// ordinary bound props.
class MarkdownPlayground : public Component<MarkdownPlayground> {
 public:
  std::string markdown_source = std::string(kDefaultMarkdown);
  std::string stylesheet = std::string(kDefaultStylesheet);
  std::string status = "OK";

  std::string status_class() const { return status == "OK" ? "ok" : "error"; }

  void InitReflection() override {
    Bind(markdown_source);
    Bind(stylesheet);
    Bind(status);
    BindComputed(status_class);
    Import<rtxui::div>();
    Import<rtxui::span>();
    Import<rtxui::textarea>();
    Import<rtxui::markdown>();
    Component<MarkdownPlayground>::InitReflection();
  }

  // Resets `status` to "OK" right before the reactive re-render triggered by
  // an edit -- if the new stylesheet is malformed, SetCssErrorHandler's
  // callback (wired up in main()) overwrites it again with the real error
  // during that same render, same recovery pattern as playground.cpp.
  bool Digest() override {
    if (markdown_source != last_markdown_ || stylesheet != last_stylesheet_) {
      last_markdown_ = markdown_source;
      last_stylesheet_ = stylesheet;
      status = "OK";
    }
    return Component<MarkdownPlayground>::Digest();
  }

  std::string_view view = R"html(
    <div class="app">
      <div class="pane editor-pane">
        <div class="pane-title">Markdown
          <span class="hint">— live</span>
        </div>
        <textarea
          class="editor"
          value="{markdown_source}"
          linenumbers="absolute"
          highlight_current_line="true"
        />
        <div class="pane-title">Stylesheet
          <span class="hint">— CSS, live</span>
        </div>
        <textarea
          class="editor"
          value="{stylesheet}"
          linenumbers="absolute"
          highlight_current_line="true"
        />
        <div class="status {status_class}">{status}</div>
      </div>
      <div class="pane preview-pane">
        <div class="pane-title">Preview</div>
        <div class="preview-frame">
          <markdown content="{markdown_source}" stylesheet="{stylesheet}" />
        </div>
      </div>
    </div>

    <style>
      self {
        display: block;
        width: 100%;
        height: 100%;
        background-color: rgb(15, 23, 42);
        color: white;
      }
      .app {
        display: flex;
        flex-direction: row;
        width: 100%;
        height: 100%;
      }
      .pane {
        display: flex;
        flex-direction: column;
        width: 50%;
        height: 100%;
        padding: 1;
      }
      .editor-pane {
        border-right: solid;
        border-color: rgb(51, 65, 85);
      }
      .pane-title {
        font-weight: bold;
        color: rgb(129, 140, 248);
        margin-bottom: 1;
      }
      .hint {
        font-weight: normal;
        color: rgb(148, 163, 184);
      }
      .editor {
        flex-grow: 1;
        width: 100%;
        padding-left: 1;
        padding-right: 1;
        overflow-y: scroll;
        margin-bottom: 1;
      }
      /* Themes the gutter/current-line, which live inside the textarea's
         own template and so aren't reachable by an ordinary .editor .gutter
         selector -- see docs/guide/css/basics.md. */
      .editor::part(gutter) {
        color: rgb(71, 85, 105);
      }
      .editor::part(active) {
        color: rgb(129, 140, 248);
      }
      .editor::part(current-line) {
        background-color: rgb(49, 55, 79);
      }
      .editor::part(selection) {
        background-color: rgb(67, 56, 202);
      }
      .editor::part(cursor) {
        color: rgb(226, 232, 240);
      }
      .status {
        color: rgb(148, 163, 184);
      }
      .status.error {
        color: rgb(248, 113, 113);
      }
      .preview-frame {
        flex-grow: 1;
        width: 100%;
        overflow-y: scroll;
      }
    </style>
  )html";

 private:
  std::string last_markdown_ = std::string(kDefaultMarkdown);
  std::string last_stylesheet_ = std::string(kDefaultStylesheet);
};

int main() {
  auto app = Ref<MarkdownPlayground>::New();

  // Without this, a CSS error in the live-edited stylesheet (e.g.
  // mid-keystroke, before the user finishes typing a rule) would print
  // straight to stderr and corrupt the running frame -- Screen owns the
  // terminal in raw mode, so route it into the status line instead.
  SetCssErrorHandler([app](const CssError& error) {
    app->status = "CSS error, line " + std::to_string(error.line + 1) +
                  ": " + error.message;
  });

  // The <markdown> component wraps the stylesheet and rendered Markdown body
  // into one document and re-parses it as XML on every keystroke (see
  // markdown.cpp's Digest()); a malformed stylesheet (e.g. a literal <tag>
  // inside a CSS comment, same bug class as playground.cpp) can break that
  // parse. Route it into the status line, same recovery pattern as the CSS
  // handler above.
  SetXmlErrorHandler([app](const XmlError& error) {
    app->status = "HTML error: " + error.message;
  });

  Screen screen(app);
  screen.Loop();
  return 0;
}
