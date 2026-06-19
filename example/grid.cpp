// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <iostream>
#include <rtxui/rtxui.hpp>
#include "rtxui/component/default_components_internal.hpp"

using namespace rtxui;

class GridDemo : public Component<GridDemo> {
 public:
  std::string_view view = R"html(
      <div class="content">
        <h1>RTXUI CSS Grid Layout Demo</h1>
        <p>This demo showcases the new CSS Grid layout support in RTXUI. The container uses display: grid, grid-template-columns: 1fr 2fr 1fr, and gaps.</p>

        <div class="grid-container">
          <div class="grid-item item-1">1 (1fr)</div>
          <div class="grid-item item-2">2 (2fr - wider)</div>
          <div class="grid-item item-3">3 (1fr)</div>
          <div class="grid-item item-4">4 (1fr)</div>
          <div class="grid-item item-5">5 (2fr - taller row basis)</div>
          <div class="grid-item item-6">6 (1fr)</div>
          <div class="grid-item item-7">7 (1fr)</div>
          <div class="grid-item item-8">8 (2fr)</div>
          <div class="grid-item item-9">9 (1fr)</div>
        </div>
      </div>
      
      <style>
        .content {
          display: block;
          padding: 1;
          width: 100%;
          height: 100%;
        }
        h1 {
          font-weight: bold;
          color: rgb(96, 165, 250);
          margin-bottom: 1;
        }
        p {
          margin-bottom: 2;
        }
        .grid-container {
          display: grid;
          grid-template-columns: 1fr 2fr 1fr;
          gap: 1 2;
          width: 90%;
          height: 14;
          border: solid;
          border-color: rgb(74, 85, 104);
          padding: 1;
        }
        .grid-item {
          display: block;
          padding: 1;
          border: tall;
        }
        .item-1 { background-color: rgb(239, 68, 68); color: white; border-color: rgb(185, 28, 28); }
        .item-2 { background-color: rgb(249, 115, 22); color: white; border-color: rgb(194, 65, 12); }
        .item-3 { background-color: rgb(234, 179, 8); color: black; border-color: rgb(161, 98, 7); }
        .item-4 { background-color: rgb(34, 197, 94); color: white; border-color: rgb(21, 128, 61); }
        .item-5 { background-color: rgb(59, 130, 246); color: white; border-color: rgb(29, 78, 216); }
        .item-6 { background-color: rgb(168, 85, 247); color: white; border-color: rgb(126, 34, 206); }
        .item-7 { background-color: rgb(236, 72, 153); color: white; border-color: rgb(190, 24, 74); }
        .item-8 { background-color: rgb(20, 184, 166); color: white; border-color: rgb(13, 148, 136); }
        .item-9 { background-color: rgb(100, 116, 139); color: white; border-color: rgb(71, 85, 105); }
      </style>
    )html";

  void InitReflection() override {
    Import<rtxui::div>();
    Import<rtxui::h1>();
    Import<rtxui::p>();
    Component<GridDemo>::InitReflection();
  }
};

int main() {
  auto app = Ref<GridDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
