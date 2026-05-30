// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <iostream>
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class NestedScrollDemo : public Component<NestedScrollDemo> {
 public:
  std::string_view Setup() override {
    return R"html(
      <h1>RTXUI Nested Scrolling & Chaining Demo</h1>
      <p class="desc">
        This demo showcases nested scrollable elements. Reaching the boundaries (top or bottom)
        of an inner container will bubble the scroll event (mouse wheel or keyboard) to the outer container.
      </p>

      <div class="section-title">[Outer Start] Beginning of Outer List</div>
      <div>[Outer Item 1] Scroll down to discover inner containers...</div>
      <div>[Outer Item 2] Outer container can scroll independently.</div>
      <div>[Outer Item 3] Scroll speed configured in CSS.</div>
      <div>[Outer Item 4] Scrollbar dynamically renders on the right.</div>
      <div>[Outer Item 5] Preparing to load Inner Box A...</div>

      <div class="inner-title">Nested Scrollbox A (Light Blue Border)</div>
      <div class="inner-scroll box-a">
        <div>[Box A - Item 1] Focus me & press ArrowDown</div>
        <div>[Box A - Item 2] Or scroll with mouse wheel</div>
        <div>[Box A - Item 3] Standard scroll chaining is active</div>
        <div>[Box A - Item 4] Item 4</div>
        <div>[Box A - Item 5] Item 5</div>
        <div>[Box A - Item 6] Item 6</div>
        <div>[Box A - Item 7] Item 7</div>
        <div>[Box A - Item 8] Item 8</div>
        <div>[Box A - Item 9] Item 9</div>
        <div>[Box A - Item 10] Item 10</div>
        <div>[Box A - Item 11] Item 11</div>
        <div>[Box A - Item 12] Item 12</div>
        <div>[Box A - Item 13] Item 13</div>
        <div>[Box A - Item 14] Item 14</div>
        <div>[Box A - Item 15] Box A Bottom - Scroll down again to bubble up</div>
      </div>

      <div class="section-title">[Outer Middle] Content between inner containers</div>
      <div>[Outer Item 6] This is intermediate content.</div>
      <div>[Outer Item 7] Layout handles arbitrary nesting level.</div>
      <div>[Outer Item 8] Ready for Inner Box B...</div>

      <div class="inner-title">Nested Scrollbox B (Steel Blue Border)</div>
      <div class="inner-scroll box-b">
        <div>[Box B - Item 1] Welcome to Box B!</div>
        <div>[Box B - Item 2] Focus me & scroll to end</div>
        <div>[Box B - Item 3] Item 3</div>
        <div>[Box B - Item 4] Item 4</div>
        <div>[Box B - Item 5] Item 5</div>
        <div>[Box B - Item 6] Item 6</div>
        <div>[Box B - Item 7] Item 7</div>
        <div>[Box B - Item 8] Item 8</div>
        <div>[Box B - Item 9] Item 9</div>
        <div>[Box B - Item 10] Item 10</div>
        <div>[Box B - Item 11] Item 11</div>
        <div>[Box B - Item 12] Item 12</div>
        <div>[Box B - Item 13] Item 13</div>
        <div>[Box B - Item 14] Item 14</div>
        <div>[Box B - Item 15] Box B Bottom - Scroll down again to bubble up</div>
      </div>

      <div class="section-title">[Outer End] End of Outer List</div>
      <div>[Outer Item 9] Both nested boxes can bubble scroll events up.</div>
      <div>[Outer Item 10] Thank you for using RTXUI!</div>

      <style>
        self {
          display: block;
          width: 100%;
          height: 100%;
          overflow-y: scroll;
          scroll-speed: 1;
          border: tall;
          border-color: rgb(29, 78, 216);
          background-color: rgb(15, 23, 42);
          color: rgb(241, 245, 249);
          padding: 1;
        }
        h1 {
          color: rgb(147, 197, 253);
          font-weight: bold;
          margin-bottom: 1;
        }
        .desc {
          color: rgb(148, 163, 184);
          margin-bottom: 1;
        }
        .inner-scroll {
          display: block;
          height: 5;
          overflow-y: scroll;
          scroll-speed: 1;
          margin-top: 1;
          margin-bottom: 1;
        }
        .box-a {
          border: tall;
          border-color: rgb(147, 197, 253);
          background-color: rgb(30, 41, 59);
        }
        .box-b {
          border: tall;
          border-color: rgb(59, 130, 246);
          background-color: rgb(30, 41, 59);
        }
        .section-title {
          color: rgb(96, 165, 250);
          font-weight: bold;
          margin-top: 1;
          margin-bottom: 1;
        }
        .inner-title {
          color: rgb(191, 219, 254);
          font-weight: bold;
          margin-top: 1;
        }
      </style>
    )html";
  }
};

int main() {
  auto app = Ref<NestedScrollDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
