// Copyright 2025 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.

#include "rtxui/geometry/LogicalSize.hpp"

namespace rtxui {

constexpr LogicalSize::LogicalSize(int inline_size, int block_size)
    : inline_size(inline_size), block_size(block_size) {}

constexpr LogicalSize LogicalSize::operator+(const LogicalSize& other) const {
  return LogicalSize(inline_size + other.inline_size,
                     block_size + other.block_size);
}

constexpr LogicalSize& LogicalSize::operator+=(const LogicalSize& other) {
  inline_size += other.inline_size;
  block_size += other.block_size;
  return *this;
}

constexpr LogicalSize LogicalSize::operator-(const LogicalSize& other) const {
  return LogicalSize(inline_size - other.inline_size,
                     block_size - other.block_size);
}

constexpr LogicalSize& LogicalSize::operator-=(const LogicalSize& other) {
  inline_size -= other.inline_size;
  block_size -= other.block_size;
  return *this;
}

}  // namespace rtxui
