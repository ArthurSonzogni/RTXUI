// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// text-align: left, center and right.
#include <rtxui/rtxui.hpp>
#include <string_view>

using namespace rtxui;

class TextAlignDemo : public Component<TextAlignDemo> {
 public:
  std::string_view view = R"html(
      <div class="card">
        <h1>Text alignment</h1>
        <p>text-align places each line within its box.</p>

        <div class="box left">
          <span class="name">left</span>
          Lines start at the left edge, which is the default.
        </div>
        <div class="box center">
          <span class="name">center</span>
          Lines are centered between both edges.
        </div>
        <div class="box right">
          <span class="name">right</span>
          Lines end at the right edge.
        </div>
      </div>

      <style>
        self {
          --border: rgb(48, 54, 61);
          --muted: rgb(139, 148, 158);
          --accent: rgb(88, 166, 255);

          display: flex;
          align-items: center;
          justify-content: center;
          width: 100%;
          color: rgb(230, 237, 243);
        }
        .card {
          border: tall;
          border-color: var(--border);
          background-color: rgb(22, 27, 34);
          padding: 1 3;
          width: 46;
        }
        h1 {
          color: var(--accent);
        }
        p {
          color: var(--muted);
        }
        .box {
          border: round;
          border-color: var(--border);
          padding: 0 1;
          margin-top: 1;
        }
        .name {
          display: block;
          color: var(--accent);
        }
        .left {
          text-align: left;
        }
        .center {
          text-align: center;
        }
        .right {
          text-align: right;
        }
      </style>
    )html";
};

int main() {
  auto app = Ref<TextAlignDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
