// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// <radio> buttons sharing a `name`, with the selection bound to a C++ string.
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class RadioDemo : public Component<RadioDemo> {
 public:
  bool select_a = false;
  bool select_b = false;
  bool select_c = false;

  std::string selected_tech() const {
    if (select_a) {
      return "PostgreSQL";
    }
    if (select_b) {
      return "MongoDB";
    }
    if (select_c) {
      return "Redis";
    }
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
          background-color: rgb(13, 17, 23);
        }
        h1 {
          color: rgb(88, 166, 255);
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
          border-color: rgb(48, 54, 61);
          padding-left: 2;
        }
        .status {
          font-weight: bold;
          color: rgb(234, 179, 8);
        }
      </style>
    )html";

  RadioDemo() {
    Bind(select_a);
    Bind(select_b);
    Bind(select_c);
    Bind(selected_tech);
  }
};

int main() {
  auto app = Ref<RadioDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
