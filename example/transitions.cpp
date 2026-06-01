// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <iostream>
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class TransitionsDemo : public Component<TransitionsDemo> {
 public:
  std::string_view view = R"html(
      <div class="container">
        <h3>CSS Transitions</h3>
        <p>Hover over the box to trigger a smooth color transition:</p>
        <div class="box">Hover Me</div>
      </div>

      <style>
        self {
          display: block;
          padding: 1 2;
          background-color: rgb(15, 23, 42); /* Deep dark slate */
          color: rgb(241, 245, 249);
        }
        h3 {
          color: rgb(59, 130, 246); /* Bright blue */
          margin-bottom: 0;
        }
        p {
          color: rgb(148, 163, 184); /* Muted slate */
          margin-bottom: 1;
        }
        .box {
          display: block;
          border: solid;
          border-color: rgb(30, 58, 138); /* Slate Blue border */
          background-color: rgb(17, 24, 39); /* Very dark blue-gray */
          color: rgb(191, 219, 254); /* Light blue text */
          padding: 1 3;
          text-align: center;
          width: 24;
          transition: background-color 0.4s ease-in-out, border-color 0.3s ease-out, color 0.3s ease;
        }
        .box:hover {
          background-color: rgb(29, 78, 216); /* Bright Blue on hover */
          border-color: rgb(96, 165, 250); /* Light Blue border */
          color: rgb(255, 255, 255);
        }
        .box:active {
          background-color: rgb(30, 58, 138); /* Slate Blue on click */
          border-color: rgb(59, 130, 246); /* Bright Blue border */
          color: rgb(255, 255, 255);
        }
        .box:focus {
          background-color: rgb(30, 58, 138); /* Slate Blue on focus */
          border-color: rgb(59, 130, 246); /* Bright Blue border */
          color: rgb(255, 255, 255);
        }
      </style>
    )html";
};

int main() {
  auto app = Ref<TransitionsDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
