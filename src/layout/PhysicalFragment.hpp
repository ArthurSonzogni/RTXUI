// Copyright 2025 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.

#pragma once

#include <iosfwd>
#include "geometry/PhysicalSize.hpp"

namespace rtxui {

class Element;

class PhysicalFragment {
 public:
  enum class Kind {
    kBox,
    kText,
    kLine,
  };

  PhysicalFragment(Kind kind,
                   PhysicalSize size,
                   Element* element = nullptr);

  Kind kind() const;
  PhysicalSize size() const;
  const Element* element() const;

 private:
  Kind kind_;
  PhysicalSize size_;
  Element* element_;
};

std::ostream& operator<<(std::ostream& os, const PhysicalFragment&);

}  // namespace rtxui
