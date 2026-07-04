// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <rtxui/rtxui.hpp>

#include "rtxui/component/default_components_internal.hpp"

using namespace rtxui;

class RadioDemo : public Component<RadioDemo> {
 public:
  bool select_a = false;
  bool select_b = false;
  bool select_c = false;

  std::string selected_tech() const {
    if (select_a) return "PostgreSQL";
    if (select_b) return "MongoDB";
    if (select_c) return "Redis";
    return "";
  }

  std::string_view view = R"html(
      <div class="content">
        <h1>RTXUI Radio Button Group Demonstration</h1>
        <p>Please select your favorite database technology:</p>
        
        <div class="group">
          <radio name="db" checked="{select_a}">PostgreSQL (Robust, relational)</radio>
          <radio name="db" checked="{select_b}">MongoDB (Document-based)</radio>
          <radio name="db" checked="{select_c}">Redis (In-memory cache)</radio>
        </div>

        <p class="status">
          Selected Tech: {selected_tech}
        </p>
      </div>

      <style>
        self {
          display: block;
          padding: 1;
          background-color: rgb(18, 18, 18);
        }
        h1 {
          color: rgb(59, 130, 246);
          margin-bottom: 1;
        }
        p {
          margin-bottom: 1;
        }
        .group {
          display: flex;
          flex-direction: column;
          gap: 1;
          margin-bottom: 1;
          border-left: solid;
          border-color: rgb(74, 85, 104);
          padding-left: 2;
        }
        .status {
          font-weight: bold;
          color: rgb(234, 179, 8);
        }
      </style>
    )html";

  void InitReflection() override {
    Bind(select_a);
    Bind(select_b);
    Bind(select_c);
    Bind(selected_tech);
    Import<rtxui::radio>();
    Import<rtxui::div>();
    Import<rtxui::p>();
    Import<rtxui::h1>();
    Component<RadioDemo>::InitReflection();
  }
};

int main() {
  auto app = Ref<RadioDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
