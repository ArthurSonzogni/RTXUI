// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// The <details>/<summary> disclosure widget.
//
// Try it: click a summary row, or focus it and press Enter.
#include <rtxui/rtxui.hpp>
#include <string_view>

using namespace rtxui;

class DetailsDemo : public Component<DetailsDemo> {
 public:
  bool logs_open = false;
  bool settings_open = true;
  bool debug = false;

  std::string_view view = R"html(
      <div class="card">
        <h1>Details</h1>
        <p>Click a summary, or focus it and press Enter.</p>

        <details open="{logs_open}">
          <summary>System logs</summary>
          <div class="body">
            <div>09:41:02  Application initialized.</div>
            <div>09:41:05  Theme loaded.</div>
            <div>09:42:15  User input detected.</div>
          </div>
        </details>

        <details open="{settings_open}">
          <summary>Settings</summary>
          <div class="body">
            <checkbox checked="{debug}">Debug mode</checkbox>
          </div>
        </details>

        <details>
          <div class="body">Without a &lt;summary&gt;, the header reads "Details".</div>
        </details>
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
          color: rgb(230, 237, 243);
        }
        .card {
          border: tall;
          border-color: var(--border);
          background-color: rgb(22, 27, 34);
          padding: 1 3;
          width: 56;
        }
        h1 {
          color: var(--accent);
        }
        p {
          color: var(--muted);
        }
        summary:focus {
          color: var(--accent);
        }
        .body {
          border-left: solid;
          border-color: var(--border);
          padding-left: 2;
          margin-left: 1;
          color: var(--muted);
        }
      </style>
    )html";

  DetailsDemo() {
    Bind(logs_open);
    Bind(settings_open);
    Bind(debug);
  }
};

int main() {
  auto app = Ref<DetailsDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
