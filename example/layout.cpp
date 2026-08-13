// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Flexbox basics.
//
// A flex row distributing three child components, each its own component with
// bound props.
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
          --bg: rgb(13, 17, 23);
          --accent: rgb(88, 166, 255);

          background-color: var(--bg);
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
          <Box text="Blue-300" color="rgb(121, 192, 255)"></Box>
          <Box text="Blue-500" color="var(--accent)"></Box>
          <Box text="Blue-700" color="rgb(29, 78, 216)"></Box>
        </div>
      </div>

      <style>
        self {
          display: block;
          padding: 1;
          background-color: var(--bg);
          color: white;
        }
        h1 {
          color: var(--accent);
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
