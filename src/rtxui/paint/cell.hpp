// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef PAINT_CELL_HPP_
#define PAINT_CELL_HPP_

#include <cstdint>   // for uint8_t
#include <optional>  // for optional
#include <string>    // for string, basic_string, allocator

#include "rtxui/paint/color.hpp"  // for Color, Color::Default

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
  bool italic : 1 = false;
  bool inverted : 1 = false;
  bool underlined : 1 = false;
  bool underlined_double : 1 = false;
  bool strikethrough : 1 = false;
  bool overlined : 1 = false;
  bool automerge : 1 = false;
  // True for the second cell of a double-width (e.g. CJK) grapheme.
  // The terminal cursor is already past this cell; nothing should be printed.
  bool is_continuation : 1 = false;

  // The graphemes stored into the pixel. To support combining characters,
  // like: á, this can potentially contain multiple codepoints.
  std::string character = "";
};

inline bool operator==(const Cell& lhs, const Cell& rhs) noexcept {
  return lhs.background_color == rhs.background_color &&
         lhs.foreground_color == rhs.foreground_color &&
         lhs.blink == rhs.blink && lhs.bold == rhs.bold && lhs.dim == rhs.dim &&
         lhs.italic == rhs.italic && lhs.inverted == rhs.inverted &&
         lhs.underlined == rhs.underlined &&
         lhs.underlined_double == rhs.underlined_double &&
         lhs.strikethrough == rhs.strikethrough &&
         lhs.overlined == rhs.overlined &&
         lhs.is_continuation == rhs.is_continuation &&
         lhs.character == rhs.character;
}

inline bool operator!=(const Cell& lhs, const Cell& rhs) noexcept {
  return !(lhs == rhs);
}

extern Cell NullCell;

#endif  // PAINT_CELL_HPP_
