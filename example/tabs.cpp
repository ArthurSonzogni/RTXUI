// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// The built-in <tabs>/<tab-pane> components.
//
// The active pane is selected by the `value` attribute bound to a C++ string.
#include <rtxui/rtxui.hpp>
#include <string>
#include <string_view>

using namespace rtxui;

class TabsDemo : public Component<TabsDemo> {
 public:
  std::string current_tab = "home";

  std::string_view view = R"html(
      <div class="card">
        <h1>Tabs</h1>
        <p>The value attribute names the active pane. Click a tab, or focus it and use the arrows.</p>

        <tabs value="{current_tab}">
          <tab-pane label="Dashboard" name="home">
            <div class="pane">Active processes: 14</div>
          </tab-pane>
          <tab-pane label="Settings" name="settings">
            <div class="pane">Preferences and theme.</div>
          </tab-pane>
          <tab-pane label="About" name="about">
            <div class="pane">RTXUI, a terminal UI library for C++.</div>
          </tab-pane>
        </tabs>

        <div class="readout">
          <span>value</span>
          <span class="value">{current_tab}</span>
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
          width: 56;
        }
        h1 {
          color: var(--accent);
        }
        p {
          color: var(--muted);
        }
        .pane {
          border: round;
          border-color: var(--border);
          padding: 1 2;
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

  TabsDemo() { Bind(current_tab); }
};

int main() {
  auto app = Ref<TabsDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
