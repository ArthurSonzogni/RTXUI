// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <rtxui/rtxui.hpp>

#include "rtxui/component/default_components_internal.hpp"

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
            <p class="tab-body">RTXUI Version 1.2.0 - Created by Arthur Sonzogni.</p>
          </tab-pane>
        </tabs>
      </div>

      <style>
        self {
          display: block;
          padding: 1;
          background-color: rgb(18, 18, 18);
        }
        h1 {
          color: rgb(59, 130, 246);
          margin-bottom: 2;
        }
        .tab-body {
          border: solid;
          border-color: rgb(74, 85, 104);
          padding: 1;
          margin-top: 1;
          color: rgb(226, 232, 240);
        }
      </style>
    )html";

  void InitReflection() override {
    Bind(current_tab);
    Import<rtxui::tabs>();
    Import<rtxui::tab_pane>();
    Import<rtxui::div>();
    Import<rtxui::p>();
    Import<rtxui::h1>();
    Component<TabsDemo>::InitReflection();
  }
};

int main() {
  auto app = Ref<TabsDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
