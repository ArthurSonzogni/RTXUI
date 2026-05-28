// Copyright 2025 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.

#include "rtxui/geometry/PhysicalOffset.hpp"

#include <format>
#include <ostream>

namespace rtxui {

constexpr PhysicalOffset::PhysicalOffset(int x, int y) : x(x), y(y) {}

constexpr PhysicalOffset PhysicalOffset::operator+(
    const PhysicalOffset& other) const {
  return {x + other.x, y + other.y};
}

constexpr PhysicalOffset& PhysicalOffset::operator+=(
    const PhysicalOffset& other) {
  x += other.x;
  y += other.y;
  return *this;
}

constexpr PhysicalOffset PhysicalOffset::operator-(
    const PhysicalOffset& other) const {
  return {x - other.x, y - other.y};
}

constexpr PhysicalOffset& PhysicalOffset::operator-=(
    const PhysicalOffset& other) {
  x -= other.x;
  y -= other.y;
  return *this;
}

std::ostream& operator<<(std::ostream& os, const PhysicalOffset& offset) {
  return os << std::format("PhysicalOffset({}, {})", offset.x, offset.y);
}

}  // namespace rtxui
