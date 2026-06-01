// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <iostream>
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class TextDecorationDemo : public Component<TextDecorationDemo> {
 public:
  std::string_view view = R"html(
      <div>
        <h1>RTXUI Text Decoration Demo</h1>
        <p>Demonstrates various text-decoration styling attributes supported by the layout and painting engine.</p>
        
        <div class="card">
          <div class="title">text-decoration: underline</div>
          <div class="content underline-text">This text has a single underline.</div>
        </div>

        <div class="card">
          <div class="title">text-decoration: double-underline</div>
          <div class="content double-underline-text">This text has a double underline.</div>
        </div>

        <div class="card">
          <div class="title">text-decoration: line-through / strikethrough</div>
          <div class="content strikethrough-text">This text has a horizontal line running through it.</div>
        </div>

        <div class="card">
          <div class="title">text-decoration: blink</div>
          <div class="content blink-text">This text is blinking.</div>
        </div>

        <div class="card">
          <div class="title">Combined Styles</div>
          <div class="content combined-text">This text is bold, strikethrough, and double-underlined!</div>
        </div>
      </div>

      <style>
        self {
          display: block;
          padding: 1;
          border: tall;
          border-color: fuchsia;
          background-color: rgb(15, 10, 20);
          color: white;
          width: 50;
          height: 24;
          overflow-y: scroll;
        }
        h1 {
          color: magenta;
          font-weight: bold;
          text-align: center;
        }
        p {
          margin-bottom: 1;
          color: gray;
          text-align: center;
        }
        .card {
          display: block;
          margin-top: 1;
          padding: 1;
          border: round;
          border-color: rgb(80, 80, 100);
        }
        .title {
          font-weight: bold;
          color: yellow;
          margin-bottom: 1;
        }
        .content {
          color: aqua;
        }
        .underline-text {
          text-decoration: underline;
        }
        .double-underline-text {
          text-decoration: double-underline;
        }
        .strikethrough-text {
          text-decoration: strikethrough;
        }
        .blink-text {
          text-decoration: blink;
        }
        .combined-text {
          font-weight: bold;
          text-decoration: double-underline strikethrough;
        }
      </style>
    )html";
};

int main() {
  auto app = Ref<TextDecorationDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
