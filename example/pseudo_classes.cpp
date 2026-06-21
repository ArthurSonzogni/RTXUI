// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <iostream>
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class PseudoClassesDemo : public Component<PseudoClassesDemo> {
 public:
  std::string_view view = R"html(
      <div class="container">
        <h3>Interactive Pseudo-Classes</h3>
        <p>Hover/Click/Focus the button below:</p>
        <div class="btn" tabindex="0">Interactive Button</div>
      </div>

      <style>
        self {
          display: block;
          padding: 1 2;
          background-color: rgb(18, 18, 18); /* Deep dark slate */
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
        .btn {
          display: block;
          border: tall;
          border-color: rgb(30, 58, 138); /* Slate Blue border */
          background-color: rgb(17, 24, 39); /* Very dark blue-gray */
          color: rgb(191, 219, 254); /* Light blue text */
          padding: 1 3;
          text-align: center;
          width: 24;

          &:hover {
            background-color: rgb(30, 58, 138); /* Slate blue on hover */
            border-color: rgb(59, 130, 246); /* Bright blue border */
            color: rgb(255, 255, 255);
          }
          
          &:active {
            background-color: rgb(29, 78, 216); /* Intense blue on active */
            border-color: rgb(96, 165, 250);
          }
          
          &:focus {
            border-color: rgb(147, 197, 253); /* Accent border on focus */
          }
        }
      </style>
    )html";
};

int main() {
  auto app = Ref<PseudoClassesDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
