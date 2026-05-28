// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <iostream>
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class BorderBox : public Component<BorderBox> {
 public:
  struct Props {
    std::string title = "Border";
    std::string border_class = "solid";
    std::string color = "white";
  } props;

  BorderBox() {
    Bind(props.title);
    Bind(props.border_class);
    Bind(props.color);
  }

  std::string_view Setup() override {
    Import<rtxui::div>();
    return R"html(
      <div class="box-wrapper">
        <div class="label">{title}</div>
      </div>

      <style>
        self {
          display: block;
          flex-grow: 1;
        }
        .box-wrapper {
          border: {border_class};
          border-color: {color};
          padding-left: 1;
          padding-right: 1;
          display: block;
        }
        .label {
          font-weight: bold;
          color: yellow;
          text-align: center;
        }
      </style>
    )html";
  }
};

class BordersDemo : public Component<BordersDemo> {
 public:
  std::string_view Setup() override {
    Import<rtxui::div>();
    Import<rtxui::h1>();
    Import<rtxui::p>();
    Import<BorderBox>();
    return R"html(
      <div>
        <h1>RTXUI Border Styles Gallery</h1>
        <p>This demo showcases the 12 different border styles supported by RTXUI.</p>

        <div class="row">
          <BorderBox title="ascii" border_class="ascii" color="rgb(200, 200, 200)"></BorderBox>
          <BorderBox title="blank" border_class="blank" color="gray"></BorderBox>
          <BorderBox title="dashed" border_class="dashed" color="cyan"></BorderBox>
          <BorderBox title="double" border_class="double" color="magenta"></BorderBox>
        </div>

        <div class="row">
          <BorderBox title="hkey" border_class="hkey" color="red"></BorderBox>
          <BorderBox title="heavy" border_class="heavy" color="green"></BorderBox>
          <BorderBox title="inner" border_class="inner" color="blue"></BorderBox>
          <BorderBox title="outer" border_class="outer" color="rgb(255, 128, 0)"></BorderBox>
        </div>

        <div class="row">
          <BorderBox title="panel" border_class="panel" color="rgb(0, 255, 128)"></BorderBox>
          <BorderBox title="round" border_class="round" color="rgb(128, 0, 255)"></BorderBox>
          <BorderBox title="solid" border_class="solid" color="white"></BorderBox>
          <BorderBox title="tall" border_class="tall" color="rgb(255, 255, 128)"></BorderBox>
        </div>
      </div>

      <style>
        self {
          display: block;
          padding: 1;
          border: double;
          border-color: gray;
          background-color: rgb(20, 20, 20);
          color: white;
          width: 100%;
        }
        h1 {
          color: rgb(255, 180, 0);
          font-weight: bold;
          margin-bottom: 1;
        }
        p {
          margin-bottom: 2;
        }
        .row {
          display: flex;
          width: 100%;
          gap: 2;
          margin-bottom: 1;
        }
      </style>
    )html";
  }
};

int main() {
  auto app = Ref<BordersDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
