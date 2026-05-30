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
  } props;

  BorderBox() {
    Bind(props.title);
    Bind(props.border_class);
  }

  std::string_view Setup() override {
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
          border-color: rgb(100, 200, 255);
          padding: 1;
          margin: 1;
          display: block;
        }
        .label {
          font-weight: bold;
          color: rgb(100, 200, 255);
          text-align: center;
        }
      </style>
    )html";
  }
};

class BordersDemo : public Component<BordersDemo> {
 public:
  std::string_view Setup() override {
    Import<BorderBox>();
    return R"html(
      <div>
        <h1>RTXUI Border Styles Gallery</h1>
        <p>This demo showcases the 24 different border styles supported by RTXUI.</p>

        <div class="row">
          <BorderBox title="ascii" border_class="ascii"></BorderBox>
          <BorderBox title="blank" border_class="blank"></BorderBox>
          <BorderBox title="dashed" border_class="dashed"></BorderBox>
          <BorderBox title="double" border_class="double"></BorderBox>
        </div>

        <div class="row">
          <BorderBox title="hkey" border_class="hkey"></BorderBox>
          <BorderBox title="heavy" border_class="heavy"></BorderBox>
          <BorderBox title="inner" border_class="inner"></BorderBox>
          <BorderBox title="none" border_class="none"></BorderBox>
        </div>

        <div class="row">
          <BorderBox title="outer" border_class="outer"></BorderBox>
          <BorderBox title="panel" border_class="panel"></BorderBox>
          <BorderBox title="round" border_class="round"></BorderBox>
          <BorderBox title="solid" border_class="solid"></BorderBox>
        </div>

        <div class="row">
          <BorderBox title="tall" border_class="tall"></BorderBox>
          <BorderBox title="thick" border_class="thick"></BorderBox>
          <BorderBox title="vkey" border_class="vkey"></BorderBox>
          <BorderBox title="wide" border_class="wide"></BorderBox>
        </div>

        <div class="row">
          <BorderBox title="dotted" border_class="dotted"></BorderBox>
          <BorderBox title="double-horiz" border_class="double-horizontal"></BorderBox>
          <BorderBox title="double-vert" border_class="double-vertical"></BorderBox>
          <BorderBox title="shadow (3d)" border_class="shadow"></BorderBox>
        </div>

        <div class="row">
          <BorderBox title="shade-light" border_class="shade-light"></BorderBox>
          <BorderBox title="shade-med" border_class="shade-medium"></BorderBox>
          <BorderBox title="shade-dark" border_class="shade-dark"></BorderBox>
          <BorderBox title="squiggle" border_class="squiggle"></BorderBox>
        </div>
      </div>

      <style>
        self {
          display: block;
          padding: 1;
          border: double;
          border-color: rgb(50, 100, 200);
          background-color: rgb(5, 10, 30);
          color: white;
          width: 100%;
          height: 100%;
          overflow-y: scroll;
        }
        h1 {
          font-weight: bold;
          margin-bottom: 1;
          color: rgb(100, 200, 255);
        }
        p {
          margin-bottom: 2;
          color: rgb(170, 200, 255);
        }
        .row {
          display: flex;
          width: 100%;
          gap: 2;
          margin-bottom: 2;
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
