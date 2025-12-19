// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef PAINT_CELL_HPP_
#define PAINT_CELL_HPP_

#include <cstdint>  // for uint8_t
#include <string>   // for string, basic_string, allocator

#include "color.hpp"  // for Color, Color::Default

/// @brief A Unicode character and its associated style.
/// @ingroup screen
struct Cell {
  // Colors:
  Color background_color;
  Color foreground_color;

  // A bit field representing the style:
  bool blink : 1 = false;
  bool bold : 1 = false;
  bool dim : 1 = false;
  bool inverted : 1 = false;
  bool underlined : 1 = false;
  bool underlined_double : 1 = false;
  bool strikethrough : 1 = false;
  bool automerge : 1 = false;

  // The graphemes stored into the pixel. To support combining characters,
  // like: a?, this can potentially contain multiple codepoints.
  std::string character = "";
};

extern Cell NullCell;

#endif  // PAINT_CELL_HPP_
