// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Horizontal overflow.
//
// overflow-x on a container that is narrower than its content produces a
// horizontal scrollbar.
//
// Try it: scroll with Shift+wheel, or drag the scrollbar.
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class HorizontalScrollDemo : public Component<HorizontalScrollDemo> {
 public:
  std::string_view view = R"html(
      <div class="header">
        <h1>RTXUI Horizontal Scrolling Demo</h1>
        <p class="desc">
          This demo showcases horizontal scrollable elements. Scroll horizontally using Shift + mouse wheel,
          WheelLeft/Right events, or ArrowLeft/ArrowRight keys when focused.
        </p>
      </div>

      <div class="item first">[Outer Start]</div>
      <div class="item">[Scroll right to find nested scrollboxes...]</div>
      <div class="item">[Item A]</div>
      <div class="item">[Item B]</div>

      <div class="nested-container">
        <div class="inner-title">Nested Box A (Light Blue Border)</div>
        <div class="inner-scroll box-a">
          <div class="inner-item first">[A-1]</div>
          <div class="inner-item">[A-2]</div>
          <div class="inner-item">[A-3]</div>
          <div class="inner-item">[A-4]</div>
          <div class="inner-item">[A-5]</div>
          <div class="inner-item">[A-6]</div>
          <div class="inner-item">[A-7]</div>
          <div class="inner-item">[A-8]</div>
          <div class="inner-item">[A-9]</div>
          <div class="inner-item last">[A-End]</div>
        </div>
      </div>

      <div class="item">[Middle Item]</div>

      <div class="nested-container">
        <div class="inner-title">Nested Box B (Steel Blue Border)</div>
        <div class="inner-scroll box-b">
          <div class="inner-item first">[B-1]</div>
          <div class="inner-item">[B-2]</div>
          <div class="inner-item">[B-3]</div>
          <div class="inner-item">[B-4]</div>
          <div class="inner-item">[B-5]</div>
          <div class="inner-item">[B-6]</div>
          <div class="inner-item">[B-7]</div>
          <div class="inner-item">[B-8]</div>
          <div class="inner-item">[B-9]</div>
          <div class="inner-item last">[B-End]</div>
        </div>
      </div>

      <div class="item last">[Outer End]</div>

      <style>
        self {
          --bg: rgb(13, 17, 23);
          --surface: rgb(22, 27, 34);
          --border: rgb(48, 54, 61);
          --text: rgb(230, 237, 243);
          --muted: rgb(139, 148, 158);
          --accent: rgb(88, 166, 255);
          --accent-bright: rgb(121, 192, 255);
          --danger: rgb(248, 81, 73);

          display: flex;
          flex-direction: row;
          width: 100%;
          height: 100%;
          overflow-x: scroll;
          scroll-speed-x: 2;
          background-color: var(--bg);
          color: var(--text);
          padding: 1;
        }
        .header {
          display: block;
          width: 35;
          height: 8;
          flex-shrink: 0;
          margin-right: 2;
          border: tall;
          border-color: var(--surface);
          padding: 1;
        }
        h1 {
          color: var(--accent-bright);
          font-weight: bold;
          margin-bottom: 1;
        }
        .desc {
          color: var(--muted);
          margin-bottom: 1;
        }
        .item {
          display: block;
          width: 25;
          height: 8;
          margin-right: 2;
          background-color: var(--surface);
          color: rgb(219, 234, 254);
          border: tall;
          border-color: var(--border);
          padding: 1;
        }
        .nested-container {
          display: block;
          width: 40;
          height: 8;
          margin-right: 2;
        }
        .inner-title {
          color: rgb(191, 219, 254);
          font-weight: bold;
          margin-bottom: 1;
        }
        .inner-scroll {
          display: flex;
          flex-direction: row;
          width: 38;
          height: 5;
          overflow-x: scroll;
          scroll-speed-x: 1;
        }
        .box-a {
          border: tall;
          border-color: var(--accent-bright);
          background-color: var(--surface);
        }
        .box-b {
          border: tall;
          border-color: var(--accent);
          background-color: var(--surface);
        }
        .inner-item {
          display: block;
          width: 10;
          margin-right: 1;
          background-color: rgb(15, 23, 42);
          color: var(--accent-bright);
          border: tall;
          border-color: var(--border);
          padding: 0 1;
        }
        .first {
          color: rgb(74, 222, 128);
        }
        .last {
          color: var(--danger);
        }
      </style>
    )html";
};

int main() {
  auto app = Ref<HorizontalScrollDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
