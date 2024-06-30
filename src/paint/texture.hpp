// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef PAINT_TEXTURE_HPP_
#define PAINT_TEXTURE_HPP_

#include <cstdint>  // for uint8_t
#include <vector>

#include "cell.hpp"

class Texture {
 public:
  Texture(std::uint8_t width, std::uint8_t height);
  Cell& At(int x, int y);

  std::uint8_t width() const { return width_; }
  std::uint8_t height() const { return height_; }
  std::string Render() const;

 private:
  std::uint8_t width_ = 0;
  std::uint8_t height_ = 0;
  std::vector<Cell> cells_;
};

#endif  // PAINT_TEXTURE_HPP_
