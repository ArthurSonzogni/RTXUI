// Copyright 2025 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.

#pragma once

#include <compare>
#include <iosfwd>  // for std::ostream

namespace rtxui {

struct PhysicalSize {
  int width = 0;
  int height = 0;

  constexpr PhysicalSize() = default;
  constexpr PhysicalSize(int w, int h);

  constexpr auto operator<=>(const PhysicalSize&) const = default;

  constexpr PhysicalSize operator+(const PhysicalSize& other) const;
  constexpr PhysicalSize& operator+=(const PhysicalSize& other);

  constexpr PhysicalSize operator-(const PhysicalSize& other) const;
  constexpr PhysicalSize& operator-=(const PhysicalSize& other);
};

}  // namespace rtxui
