// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// The built-in <tabs>/<tab-pane> components.
//
// The active pane is selected by the `value` attribute bound to a C++ string.
#include <rtxui/rtxui.hpp>


using namespace rtxui;

class TabsDemo : public Component<TabsDemo> {
 public:
  std::string current_tab = "home";

  std::string_view view = R"html(
      <div class="content">
        <h1>RTXUI Tabbed Navigation Interface</h1>

        <tabs value="{current_tab}">
          <tab-pane label="Dashboard" name="home">
            <p class="tab-body">Welcome to your Dashboard. Active processes: 14</p>
          </tab-pane>
          <tab-pane label="Settings" name="settings">
            <p class="tab-body">System preferences and visual theme modifications.</p>
          </tab-pane>
          <tab-pane label="About" name="about">
            <p class="tab-body">RTXUI Version 5.0.0 - Created by Arthur Sonzogni.</p>
          </tab-pane>
        </tabs>
      </div>

      <style>
        self {
          --bg: rgb(13, 17, 23);
          --border: rgb(48, 54, 61);
          --text: rgb(230, 237, 243);
          --accent: rgb(88, 166, 255);

          display: block;
          padding: 1;
          background-color: var(--bg);
        }
        h1 {
          color: var(--accent);
          margin-bottom: 2;
        }
        .tab-body {
          border: solid;
          border-color: var(--border);
          padding: 1;
          margin-top: 1;
          color: var(--text);
        }
      </style>
    )html";

  TabsDemo() {
    Bind(current_tab);
  }
};

int main() {
  auto app = Ref<TabsDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
