// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef STYLE_HPP
#define STYLE_HPP

#include <optional>

#include "rtxui/paint/color.hpp"

// Style associated with an HTML tag.
struct ComputedStyle {
  // Margin and padding around the element.
  struct Rect {
    int left = 0;
    int bottom = 0;
    int right = 0;
    int top = 0;
  };
  Rect margin;
  Rect padding;

  // How the element participates in the layout (block, inline, etc.), and how
  // its children are displayed (flow, table, flex, grid).
  struct Display {
    enum class Outside {
      Block,
      Inline,
    };
    Outside outside = Outside::Block;

    enum class Inside {
      Flow,
      Table,
      Flex,
      Grid,
    };
    Inside inside = Inside::Flow;
  };
  Display display;

  // Border around the element.
  struct Border {
    enum Style {
      None,
      Ascii,
      Blank,
      Dashed,
      Double,
      Heavy,
      Hidden,
      Hkey,
      Inner,
      Outer,
      Panel,
      Round,
      Solid,
      Tall,
      Thick,
      Vkey,
      Wide,
    };
    Style style = Style::None;
    Color color;
  };
  Border border;

  // Dimensions:
  std::optional<int> width;
  std::optional<int> height;
  std::optional<int> min_width;
  std::optional<int> min_height;
  std::optional<int> max_width;
  std::optional<int> max_height;

  // Colors:
  std::optional<Color> background_color;  // Background color of the element
  std::optional<Color> foreground_color;  // Text color of the element
  std::optional<bool> bold;
  std::optional<bool> underlined;
  std::optional<bool> underlined_double;
  std::optional<bool> strikethrough;
  std::optional<bool> blink;
};

#endif  // STYLE_HPP
