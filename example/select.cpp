// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <iostream>
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class SelectDemo : public Component<SelectDemo> {
 public:
  std::string my_theme = "light";

  std::string_view view = R"html(
      <div class="container">
        <p class="title">Interactive Select & Option Elements</p>
        <p class="desc">Click the select dropdown or focus it with Tab and use Enter/Space to open. Navigate options using ArrowUp/ArrowDown, and select with Enter.</p>
        
        <select value="{my_theme}">
          <option value="dark">Dark Theme</option>
          <option value="light">Light Theme</option>
          <option value="solarized">Solarized</option>
        </select>
        
        <div class="output-box">
          <span class="label">Current Theme Value:</span>
          <span class="value">{my_theme}</span>
        </div>
      </div>

      <style>
        self {
          display: block;
          padding: 1;
          background-color: rgb(15, 23, 42);
          color: white;
          border: tall;
          border-color: rgb(56, 189, 248);
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
          color: rgb(244, 63, 94);
        }
      </style>
    )html";

  SelectDemo() { Bind(my_theme); }
};

int main() {
  auto app = Ref<SelectDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
