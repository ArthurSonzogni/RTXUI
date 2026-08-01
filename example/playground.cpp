// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <string>

#include "rtxui/component/default_components_internal.hpp"
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
      border: round;
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

  void InitReflection() override {
    Import<rtxui::b>();
    Import<rtxui::button>();
    Import<rtxui::checkbox>();
    Import<rtxui::code>();
    Import<rtxui::details>();
    Import<rtxui::div>();
    Import<rtxui::fieldset>();
    Import<rtxui::h1>();
    Import<rtxui::hr>();
    Import<rtxui::i>();
    Import<rtxui::input>();
    Import<rtxui::label>();
    Import<rtxui::legend>();
    Import<rtxui::li>();
    Import<rtxui::ol>();
    Import<rtxui::option>();
    Import<rtxui::p>();
    Import<rtxui::pre>();
    Import<rtxui::progress>();
    Import<rtxui::radio>();
    Import<rtxui::s>();
    Import<rtxui::select>();
    Import<rtxui::slider>();
    Import<rtxui::span>();
    Import<rtxui::strong>();
    Import<rtxui::summary>();
    Import<rtxui::tab_pane>();
    Import<rtxui::tabs>();
    Import<rtxui::tooltip>();
    Import<rtxui::u>();
    Import<rtxui::ul>();
    Component<LivePreview>::InitReflection();
  }
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
        <textarea class="editor" value="{code}" linenumbers="relative"
                  highlight_current_line="true" />
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

  void InitReflection() override {
    Bind(code);
    Bind(status);
    BindComputed(status_class);
    Import<LivePreview>();
    Import<rtxui::div>();
    Import<rtxui::span>();
    Import<rtxui::textarea>();
    Component<Playground>::InitReflection();
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
  Screen screen(app);
  screen.Loop();
  return 0;
}
