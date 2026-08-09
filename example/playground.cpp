// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// A live HTML/CSS editor and preview, side by side.
//
// The preview re-parses on every keystroke using the same template engine that
// powers every RTXUI app, which is also why this is the docs homepage demo.
#include <string>

#include "rtxui/dom/element.hpp"
#include "rtxui/rtxui.hpp"
#include "rtxui/xml/xml.hpp"

using namespace rtxui;

namespace {

constexpr std::string_view kDefaultCode = R"html(
  <div class="card">
    <h1>Welcome to the RTXUI Playground</h1>
    <p>Edit the HTML and CSS on the left. This preview re-parses on every
    keystroke, using the same template engine that powers every RTXUI app.</p>

    <div class="row">
      <button class="btn">Hover me</button>
      <span class="badge">Live</span>
    </div>

    <progress value="65" max="100" width="30" />

    <ul>
      <li>Flexbox layout</li>
      <li>CSS transitions</li>
      <li>Borders &amp; colors</li>
    </ul>
  </div>

  <style>
    .card {
      display: block;
      border: tall;
      border-color: rgb(129, 140, 248);
      padding: 1;
      background-color: rgb(30, 41, 59);
      color: rgb(226, 232, 240);
    }
    h1 {
      color: rgb(234, 179, 8);
      margin-bottom: 1;
    }
    p {
      color: rgb(148, 163, 184);
      margin-bottom: 1;
    }
    .row {
      display: flex;
      gap: 2;
      margin-bottom: 1;
    }
    .btn {
      background-color: rgb(59, 130, 246);
      color: white;
      padding-left: 2;
      padding-right: 2;
      transition: background-color 0.2s ease-in-out;
    }
    .btn:hover {
      background-color: rgb(37, 99, 235);
    }
    .badge {
      background-color: rgb(34, 197, 94);
      color: black;
      padding-left: 1;
      padding-right: 1;
    }
    progress {
      margin-bottom: 1;
    }
  </style>
)html";

}  // namespace

// The right-hand pane. It renders whatever template text `Playground` last
// handed it via `HotReload()` — see `Playground::Digest()` below. Every
// built-in tag a visitor might type has to be `Import`-ed here first, since
// imports are local to the component whose template is being parsed.
class LivePreview : public Component<LivePreview> {
 public:
  std::string_view view = kDefaultCode;
};

// The left-hand editor shell. It owns the raw template text and, on every
// change, reparses it and forwards it to the `LivePreview` child.
class Playground : public Component<Playground> {
 public:
  std::string code = std::string(kDefaultCode);
  std::string status = "OK";

  std::string status_class() const { return status == "OK" ? "ok" : "error"; }

  std::string_view view = R"html(
    <div class="app">
      <div class="pane editor-pane">
        <div class="pane-title">Editor
          <span class="hint">— HTML + CSS, live</span>
        </div>
        <textarea
          class="editor"
          value="{code}"
          linenumbers="absolute"
          highlight_current_line="true"
        />
        <div class="status {status_class}">{status}</div>
      </div>
      <div class="pane preview-pane">
        <div class="pane-title">Preview</div>
        <div class="preview-frame">
          <LivePreview id="preview" />
        </div>
      </div>
    </div>

    <style>
      self {
        --surface: rgb(22, 27, 34);
        --border: rgb(48, 54, 61);
        --text: rgb(230, 237, 243);
        --muted: rgb(139, 148, 158);
        --accent: rgb(88, 166, 255);
        --danger: rgb(248, 81, 73);

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
        border-color: var(--border);
      }
      .pane-title {
        font-weight: bold;
        color: rgb(129, 140, 248);
        margin-bottom: 1;
      }
      .hint {
        font-weight: normal;
        color: var(--muted);
      }
      .editor {
        flex-grow: 1;
        width: 100%;
        padding-left: 1;
        padding-right: 1;
        overflow-y: scroll;
        margin-bottom: 1;
      }
      /* Themes the gutter/current-line, which live inside <textarea>'s own
         template and so aren't reachable by an ordinary .editor .gutter
         selector -- see docs/guide/css/basics.md. */
      .editor::part(gutter) {
        color: var(--border);
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
        color: var(--text);
      }
      .editor::part(placeholder) {
        color: rgb(100, 116, 139);
      }
      .status {
        color: var(--muted);
      }
      .status.error {
        color: var(--danger);
      }
      .preview-frame {
        flex-grow: 1;
        width: 100%;
        overflow-y: scroll;
      }
    </style>
  )html";

  Playground() {
    Bind(code);
    Bind(status);
    BindComputed(status_class);
    Import<LivePreview>();
  }

  bool Digest() override {
    bool changed = Component<Playground>::Digest();
    if (code == last_code_) {
      return changed;
    }
    last_code_ = code;

    Expected<xml::Nodes, xml::Error> parsed = xml::Parse(code);
    if (!parsed) {
      status = "Parse error, line " +
                std::to_string(parsed.error().line + 1) + ": " +
                parsed.error().message;
      return true;
    }
    status = "OK";

    if (Element* root = Root()) {
      if (Element* preview_root = root->QuerySelector("#preview")) {
        auto* preview = dynamic_cast<LivePreview*>(
            const_cast<ComponentBase*>(preview_root->component()));
        if (preview) {
          preview->HotReload(code);
        }
      }
    }
    return true;
  }

 private:
  std::string last_code_ = std::string(kDefaultCode);
};

int main() {
  auto app = Ref<Playground>::New();

  // Without this, a CSS error in the live-edited code (e.g. mid-keystroke,
  // before the user finishes typing a rule) would print straight to stderr
  // and corrupt the running frame -- Screen owns the terminal in raw mode,
  // so that output lands in the middle of the app instead of a scrollback
  // the user could read anyway. Route it into the status line instead, same
  // as the XML parse errors already handled in Playground::Digest above.
  SetCssErrorHandler([app](const CssError& error) {
    app->status = "CSS error, line " + std::to_string(error.line + 1) +
                  ": " + error.message;
  });

  Screen screen(app);
  screen.Loop();
  return 0;
}
