// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef PAINT_COLOR_HPP_
#define PAINT_COLOR_HPP_

#include <compare>
#include <cstdint>  // for uint8_t

struct Color {
  std::uint8_t r = 0;
  std::uint8_t g = 0;
  std::uint8_t b = 0;
  std::uint8_t a = 0;

  Color() = default;
  std::strong_ordering operator<=>(const Color&) const = default;
};

#endif  // PAINT_COLOR_HPP_
