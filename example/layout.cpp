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

  std::string_view view = R"html(
      <div class="box-content">
        {text}
      </div>

      <style>
        self {
          display: block;
          flex-grow: 1;
        }
        .box-content {
          border: tall;
          padding-left: 1;
          padding-right: 1;
          color: {color};
          border-color: {color};
        }
      </style>
    )html";

  Box() {
    Bind(props.text);
    Bind(props.color);
  }
};

class LayoutDemo : public Component<LayoutDemo> {
 public:
  std::string_view view = R"html(
      <div>
        <h1>RTXUI Layout Demonstration</h1>
        <p>This layout uses nested custom Box components inside a flex row container.</p>
        
        <div class="flex-row">
          <Box text="Blue-300" color="rgb(147, 197, 253)"></Box>
          <Box text="Blue-500" color="rgb(59, 130, 246)"></Box>
          <Box text="Blue-700" color="rgb(29, 78, 216)"></Box>
        </div>
      </div>

      <style>
        self {
          display: block;
          padding: 1;
          background-color: rgb(15, 23, 42);
          color: white;
        }
        h1 {
          color: rgb(59, 130, 246);
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

  LayoutDemo() { Import<Box>(); }
};

int main() {
  auto app = Ref<LayoutDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
