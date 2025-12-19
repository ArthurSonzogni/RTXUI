// Copyright 2025 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.

#include "layout/PhysicalFragment.hpp"
#include <format>
#include <ostream>

namespace rtxui {

PhysicalFragment::PhysicalFragment(Kind kind,
                                   PhysicalSize size,
                                   Element* element)
    : kind_(kind), size_(size), element_(element) {}

PhysicalFragment::Kind PhysicalFragment::kind() const {
  return kind_;
}

PhysicalSize PhysicalFragment::size() const {
  return size_;
}

const Element* PhysicalFragment::element() const {
  return element_;
}

}  // namespace rtxui
