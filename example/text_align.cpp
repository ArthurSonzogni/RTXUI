// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// text-align: left, center and right.
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class TextAlignDemo : public Component<TextAlignDemo> {
 public:
  std::string_view view = R"html(
      <div>
        <h1>RTXUI Text Alignment Demo</h1>
        <p>Demonstrates text-align left, right, and center horizontal alignment.</p>
        
        <div class="card left-align">
          <div class="title">text-align: left</div>
          <div class="content">This text is aligned to the left of the container.</div>
        </div>

        <div class="card center-align">
          <div class="title">text-align: center</div>
          <div class="content">This text is aligned to the center of the container.</div>
        </div>

        <div class="card right-align">
          <div class="title">text-align: right</div>
          <div class="content">This text is aligned to the right of the container.</div>
        </div>
      </div>

      <style>
        self {
          --bg: rgb(13, 17, 23);
          --border: rgb(48, 54, 61);
          --muted: rgb(139, 148, 158);
          --accent-bright: rgb(121, 192, 255);

          display: block;
          padding: 1;
          background-color: var(--bg);
          color: white;
          width: 50;
          height: 18;
          overflow-y: scroll;
        }
        h1 {
          color: rgb(56, 189, 248);
          font-weight: bold;
          text-align: center;
        }
        p {
          margin-bottom: 1;
          color: var(--muted);
          text-align: center;
        }
        .card {
          display: block;
          margin-top: 1;
          padding: 1;
          border: tall;
          border-color: var(--border);
        }
        .title {
          font-weight: bold;
          color: var(--accent-bright);
          margin-bottom: 1;
        }
        .left-align {
          text-align: left;
        }
        .center-align {
          text-align: center;
        }
        .right-align {
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
