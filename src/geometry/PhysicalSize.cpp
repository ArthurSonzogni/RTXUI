// Copyright 2025 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.

#include "PhysicalSize.hpp"

#include <format>
#include <ostream>

namespace rtxui {

constexpr PhysicalSize::PhysicalSize(int w, int h) : width(w), height(h) {}

constexpr PhysicalSize PhysicalSize::operator+(
    const PhysicalSize& other) const {
  return {
      width + other.width,
      height + other.height,
  };
}

constexpr PhysicalSize PhysicalSize::operator-(
    const PhysicalSize& other) const {
  return {
      width - other.width,
      height - other.height,
  };
}

constexpr PhysicalSize& PhysicalSize::operator+=(const PhysicalSize& other) {
  width += other.width;
  height += other.height;
  return *this;
}

constexpr PhysicalSize& PhysicalSize::operator-=(const PhysicalSize& other) {
  width -= other.width;
  height -= other.height;
  return *this;
}

std::ostream& operator<<(std::ostream& os, const PhysicalSize& size) {
  return os << std::format("PhysicalSize({}, {})", size.width, size.height);
}

}  // namespace rtxui
