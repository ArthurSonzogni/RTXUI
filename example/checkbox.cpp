// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <iostream>
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class CheckboxDemo : public Component<CheckboxDemo> {
 public:
  bool checked = false;

  std::string_view view = R"html(
      <div class="container">
        <p class="title">Interactive Checkbox Element</p>
        <p class="desc">A binary toggle component. Click on the checkbox or focus it and press Space to toggle the state.

        Coucou les gens!
        </p>
        
        <checkbox checked="{checked}">Enable Notifications</checkbox>
        
        <div class="output-box">
          <span class="label">Live Checked State:</span>
          <span class="value">{checked}</span>
        </div>
      </div>

      <style>
        self {
          display: block;
          padding: 1;
          background-color: rgb(18, 18, 18);
          color: white;
        }
        .container {
          display: block;
        }
        .title {
          display: block;
          font-weight: bold;
          color: rgb(56, 189, 248);
          margin-bottom: 1;
        }
        .desc {
          display: block;
          color: rgb(156, 163, 175);
          margin-bottom: 2;
        }
        .output-box {
          display: flex;
          gap: 2;
          margin-top: 2;
        }
        .label {
          color: rgb(56, 189, 248);
        }
        .value {
          font-weight: bold;
          color: rgb(56, 189, 248);
        }
      </style>
    )html";

  CheckboxDemo() {
    Bind(checked);
    EnableHotReload();
  }
};

int main() {
  auto app = Ref<CheckboxDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
