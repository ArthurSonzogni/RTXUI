// Copyright 2025 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.

#pragma once

#include <compare>
#include <cstdint>

namespace rtxui {

struct LogicalSize {
  int inline_size = 0;
  int block_size = 0;

  constexpr LogicalSize() = default;
  constexpr LogicalSize(int inline_size, int block_size);

  constexpr auto operator<=>(const LogicalSize&) const = default;

  constexpr LogicalSize operator+(const LogicalSize& other) const;
  constexpr LogicalSize& operator+=(const LogicalSize& other);

  constexpr LogicalSize operator-(const LogicalSize& other) const;
  constexpr LogicalSize& operator-=(const LogicalSize& other);
};

}  // namespace rtxui
