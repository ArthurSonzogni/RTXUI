// Copyright 2025 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.

#include "rtxui/geometry/PhysicalDirection.hpp"

#include <format>
#include <ostream>

namespace rtxui {

std::ostream& operator<<(std::ostream& os, PhysicalDirection direction) {
  switch (direction) {
    case PhysicalDirection::kLeft:
      return os << "PhysicalDirection::kLeft";
    case PhysicalDirection::kRight:
      return os << "PhysicalDirection::kRight";
    case PhysicalDirection::kUp:
      return os << "PhysicalDirection::kUp";
    case PhysicalDirection::kDown:
      return os << "PhysicalDirection::kDown";
  }
  return os << "PhysicalDirection::(invalid)";
}

}  // namespace rtxui
