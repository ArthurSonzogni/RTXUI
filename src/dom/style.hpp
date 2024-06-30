#ifndef STYLE_HPP
#define STYLE_HPP

#include <optional>

#include "paint/color.hpp"

struct Rect {
  int left = 0;
  int bottom = 0;
  int right = 0;
  int top = 0;
};

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

// Style associated with an HTML tag.
struct Style {
  // How the element participates in the layout (block, inline, etc.), and how
  // its children are displayed (flow, table, flex, grid).
  Display display;

  // Margin/Padding around the element
  Rect margin;
  Rect padding;

  // Border around the element.
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
};

#endif  // STYLE_HPP
