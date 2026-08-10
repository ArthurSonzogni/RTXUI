// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Recipe: tabs built by hand.
//
// The same result as the built-in <tabs>, assembled from plain elements and a
// bound string -- useful when you want full control of the markup.
#include <rtxui/rtxui.hpp>

#include <string>

using namespace rtxui;

// Cookbook recipe: tabbed navigation built by hand.
//
// The active tab lives in a state variable. Buttons call a single
// parameterized method; each button's class and each pane's visibility come
// from computed methods. The <if>/<elif>/<else> chain shows one pane at a
// time.
class TabbedApp : public Component<TabbedApp> {
 public:
  std::string active_tab = "home";

  void Select(std::string tab) { active_tab = tab; }

  bool is_home() const { return active_tab == "home"; }
  bool is_settings() const { return active_tab == "settings"; }
  bool is_about() const { return active_tab == "about"; }

  std::string home_class() const { return TabClass("home"); }
  std::string settings_class() const { return TabClass("settings"); }
  std::string about_class() const { return TabClass("about"); }

  TabbedApp() {
    Bind(active_tab);
    Bind(Select);
    Bind(is_home);
    Bind(is_settings);
    Bind(is_about);
    Bind(home_class);
    Bind(settings_class);
    Bind(about_class);
  }

  std::string_view view = R"html(
    <div class="container">

      <div class="tabs-header">
        <button class="{home_class}" onclick="Select(home)">Home</button>
        <button class="{settings_class}" onclick="Select(settings)">Settings</button>
        <button class="{about_class}" onclick="Select(about)">About</button>
      </div>

      <div class="tab-content">
        <div if="{is_home}">Welcome to the home screen.</div>
        <div if="{is_settings}">Settings go here.</div>
        <div if="{is_about}">About this application.</div>
      </div>
    </div>

    <style>
      .container {
        display: flex;
        flex-direction: column;
        max-width: 50;
      }
      .tabs-header {
        display: flex;
        flex-direction: row;
        width: 100%;
        border-bottom: solid;
        border-color: #334155;
      }
      .tab-btn {
        padding: 0 2;
        cursor: pointer;
        background-color: black;
      }
      .tab-btn.active {
        background-color: #1e3a8a;
        color: #fff;
        font-weight: bold;
      }
      .tab-content {
        padding: 1;
        min-height: 5;
      }
    </style>
  )html";

 private:
  std::string TabClass(const std::string& tab) const {
    return active_tab == tab ? "tab-btn active" : "tab-btn";
  }
};

int main() {
  auto app = Ref<TabbedApp>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
