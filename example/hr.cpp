// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <iostream>
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class HrDemo : public Component<HrDemo> {
 public:
  std::string_view view = R"html(
      <div class="container">
        <p class="title">Horizontal Rule Element Demo</p>
        <p class="desc">The &lt;hr&gt; component renders a light box-drawing line horizontally, acting as a section separator.</p>
        
        <p class="section-title">Section 1: Introduction</p>
        <p class="content">This is the first section of the document. Below it is a horizontal separator rule.</p>
        
        <hr />
        
        <p class="section-title">Section 2: Layout & Separation</p>
        <p class="content">This is the second section of the document, neatly separated from the introduction above.</p>
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
        .section-title {
          display: block;
          font-weight: bold;
          color: rgb(56, 189, 248);
          margin-top: 1;
        }
        .content {
          display: block;
          color: rgb(229, 231, 235);
        }
      </style>
    )html";
};

int main() {
  auto app = Ref<HrDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
