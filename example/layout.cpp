// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Flexbox basics.
//
// A flex row distributing three child components, each its own component with
// bound props.
#include <rtxui/rtxui.hpp>
#include <string>
#include <string_view>

using namespace rtxui;

class Box : public Component<Box> {
 public:
  struct Props {
    std::string text = "Box";
    std::string color = "white";
  } props;

  std::string_view view = R"html(
      {text}

      <style>
        self {
          display: block;
          flex-grow: 1;
          border: tall;
          border-color: {color};
          background-color: rgb(22, 27, 34);
          color: {color};
          padding: 1 2;
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
      <h1>Flexbox</h1>
      <p>A flex row shares its width among three Box components, each given its text and color as props.</p>

      <div class="row">
        <Box text="Sky" color="rgb(121, 192, 255)"></Box>
        <Box text="Blue" color="rgb(88, 166, 255)"></Box>
        <Box text="Navy" color="rgb(56, 117, 230)"></Box>
      </div>

      <style>
        self {
          display: block;
          padding: 1 2;
          background-color: rgba(13, 17, 23, 0.8);
          color: rgb(230, 237, 243);
        }
        h1 {
          color: rgb(88, 166, 255);
        }
        p {
          color: rgb(139, 148, 158);
        }
        .row {
          display: flex;
          gap: 2;
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
