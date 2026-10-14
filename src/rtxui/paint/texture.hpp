// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef PAINT_TEXTURE_HPP_
#define PAINT_TEXTURE_HPP_

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "rtxui/paint/cell.hpp"

namespace rtxui {

class Texture {
 public:
  Texture(int width, int height);
  Cell& operator[](int x, int y);
  const Cell& operator[](int x, int y) const;

  int width() const { return width_; }
  int height() const { return height_; }
  /// The escape sequences drawing the texture from the cursor, at the
  /// top-left. Each row ends where its last visible cell does: the rest is
  /// erased, not written as spaces, so a terminal rewrapping its lines on a
  /// resize sees only what is drawn there.
  std::string Render() const;
  std::string RenderDiff(const Texture& old_texture) const;
  /// What RenderDiff() returns from an identical texture, without comparing
  /// every cell: the cursor moved where it leaves it.
  std::string RenderUnchanged() const;

  /// The id for Cell::link of a hyperlink to `url`, which the terminal makes
  /// clickable (OSC 8). 0, meaning no link, for an empty URL or one holding a
  /// character outside printable ASCII, which the escape sequence cannot
  /// carry.
  uint16_t AddLink(std::string_view url);
  /// The URL of a Cell::link id; empty for 0.
  std::string_view LinkUrl(uint16_t link) const;

  /// The column after the last cell of row `y` that is not blank: one a
  /// terminal shows the same as an erased one.
  int RowEnd(int y) const;

 private:
  static bool IsBlank(const Cell& cell);

  const int width_;
  const int height_;
  std::vector<Cell> cells_;
  std::vector<std::string> links_;
};

}  // namespace rtxui

#endif  // PAINT_TEXTURE_HPP_
