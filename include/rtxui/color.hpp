// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_COLOR_HPP_
#define RTXUI_COLOR_HPP_

#include <compare>
#include <cstdint>
#include <rtxui/rtxui_export.hpp>

struct RTXUI_EXPORT Color {
  std::uint8_t r = 0;
  std::uint8_t g = 0;
  std::uint8_t b = 0;
  std::uint8_t a = 0;

  Color() = default;
  Color(Color const&) = default;

  static Color RGB(std::uint8_t red, std::uint8_t green, std::uint8_t blue);
  static Color RGBA(std::uint8_t red,
                    std::uint8_t green,
                    std::uint8_t blue,
                    std::uint8_t alpha);
  std::strong_ordering operator<=>(const Color&) const = default;
};

RTXUI_EXPORT Color Blend(Color over, Color under);

#endif  // RTXUI_COLOR_HPP_
