// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <iostream>
#include <memory>

#include "rtxui/component/default_components_internal.hpp"
#include "rtxui/internal/component.hpp"
#include "rtxui/internal/screen.hpp"

namespace {

class DetailsDemo : public rtxui::Component<DetailsDemo> {
 public:
  bool open1 = false;
  bool open2 = true;
  bool checkbox_state = false;

  std::string status_text() const {
    return checkbox_state ? "DEBUG ENABLED" : "DEBUG DISABLED";
  }

  void InitReflection() override {
    Bind(open1);
    Bind(open2);
    Bind(checkbox_state);
    Bind(status_text);
    Import<rtxui::details>();
    Import<rtxui::summary>();
    Import<rtxui::checkbox>();
    Import<rtxui::div>();
    Import<rtxui::p>();
    Import<rtxui::h1>();
    rtxui::Component<DetailsDemo>::InitReflection();
  }

  std::string_view Setup() override {
    return R"html(
      <div class="app">
        <h1>RTXUI Details &amp; Summary Demonstration</h1>

        <div class="row">
          <div class="col">
            <details open="{open1}">
              <summary>System Logs</summary>
              <p>[09:41:02] Application initialized.</p>
              <p>[09:41:05] Theme loaded: light.</p>
              <p>[09:42:15] User input detected.</p>
            </details>
          </div>

          <div class="col">
            <details open="{open2}">
              <summary>Preferences &amp; Settings</summary>
              <div class="settings-box">
                <checkbox checked="{checkbox_state}">Enable debug mode</checkbox>
                <p>Status: {status_text}</p>
              </div>
            </details>
          </div>
        </div>

        <div class="row info-row">
          <details>
            <p>This is a collapsible widget using the default summary header.</p>
            <p>Since no &lt;summary&gt; tag was defined, it automatically defaulted to "Details".</p>
          </details>
        </div>
      </div>

      <style>
        self {
          background-color: rgb(18, 18, 18);
          display: block;
          padding: 1;
        }
        .app {
          display: flex;
          flex-direction: column;
          gap: 1;
          border: solid;
          border-color: #3b82f6;
          padding: 1;
        }
        h1 {
          color: #3b82f6;
          margin-bottom: 1;
        }
        .row {
          display: flex;
          flex-direction: row;
          gap: 2;
        }
        .col {
          flex: 1;
          border: round;
          border-color: #555;
          padding: 1;
        }
        .settings-box {
          display: flex;
          flex-direction: column;
          gap: 1;
          padding-left: 1;
          border-left: solid;
          border-color: #f59e0b;
        }
        .info-row {
          margin-top: 1;
          padding: 1;
          background-color: lighten(5%);
          border: dashed;
          border-color: #888;
        }
      </style>
    )html";
  }
};

}  // namespace

int main() {
  auto component = rtxui::Ref<DetailsDemo>::New();
  rtxui::Screen screen(component);
  screen.Loop();
  return 0;
}
