// Copyright 2025 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.

#pragma once

#include "geometry/PhysicalOffset.hpp"
#include "layout/PhysicalFragment.hpp"

namespace rtxui {

struct PhysicalFragmentLink {
  const PhysicalFragment* fragment = nullptr;
  PhysicalOffset offset;

  constexpr PhysicalOffset Offset() const { return offset; }
  constexpr const PhysicalFragment* get() const { return fragment; }

  constexpr explicit operator bool() const { return fragment != nullptr; }
  constexpr const PhysicalFragment& operator*() const { return *fragment; }
  constexpr const PhysicalFragment* operator->() const { return fragment; }
};

}  // namespace rtxui
