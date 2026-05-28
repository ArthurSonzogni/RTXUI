// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <iostream>
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class Box : public Component<Box> {
 public:
  struct Props {
    std::string text = "Box";
    std::string color = "white";
  } props;

  Box() {
    Bind(props.text);
    Bind(props.color);
  }

  std::string_view Setup() override {
    Import<rtxui::div>();
    return R"html(
      <div class="box-content">
        {text}
      </div>

      <style>
        self {
          display: block;
          flex-grow: 1;
        }
        .box-content {
          border: round;
          padding-left: 1;
          padding-right: 1;
          color: {color};
          border-color: {color};
        }
      </style>
    )html";
  }
};

class LayoutDemo : public Component<LayoutDemo> {
 public:
  std::string_view Setup() override {
    Import<rtxui::div>();
    Import<rtxui::h1>();
    Import<rtxui::p>();
    Import<Box>();
    return R"html(
      <div>
        <h1>RTXUI Layout Demonstration</h1>
        <p>This layout uses nested custom Box components inside a flex row container.</p>
        
        <div class="flex-row">
          <Box text="Red Box" color="red"></Box>
          <Box text="Green Box" color="green"></Box>
          <Box text="Blue Box" color="blue"></Box>
        </div>
      </div>

      <style>
        self {
          display: block;
          padding: 1;
          border: double;
          border-color: magenta;
          background-color: rgb(10, 10, 10);
          color: white;
        }
        h1 {
          color: yellow;
          font-weight: bold;
        }
        p {
          margin-top: 1;
          margin-bottom: 1;
        }
        .flex-row {
          display: flex;
          width: 100%;
          gap: 2;
          margin-top: 1;
        }
      </style>
    )html";
  }
};

int main() {
  auto app = Ref<LayoutDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
