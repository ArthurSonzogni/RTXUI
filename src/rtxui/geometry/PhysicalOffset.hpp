// Copyright 2025 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.

#pragma once

#include <compare>
#include <iosfwd>

namespace rtxui {

struct PhysicalOffset {
  int x = 0;
  int y = 0;

  constexpr PhysicalOffset() = default;
  constexpr PhysicalOffset(int x, int y);

  constexpr auto operator<=>(const PhysicalOffset&) const = default;

  constexpr PhysicalOffset operator+(const PhysicalOffset& other) const;
  constexpr PhysicalOffset& operator+=(const PhysicalOffset& other);

  constexpr PhysicalOffset operator-(const PhysicalOffset& other) const;
  constexpr PhysicalOffset& operator-=(const PhysicalOffset& other);
};

std::ostream& operator<<(std::ostream& os, const PhysicalOffset& offset);

}  // namespace rtxui
