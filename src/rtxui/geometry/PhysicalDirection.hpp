// Copyright 2025 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.

#pragma once

#include <iosfwd>

namespace rtxui {

enum class PhysicalDirection {
  kLeft,
  kRight,
  kUp,
  kDown,
};

std::ostream& operator<<(std::ostream& os, PhysicalDirection direction);

}  // namespace rtxui
