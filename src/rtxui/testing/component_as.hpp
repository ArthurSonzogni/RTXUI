// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_TESTING_COMPONENT_AS_HPP_
#define RTXUI_TESTING_COMPONENT_AS_HPP_

#include "rtxui/component.hpp"
#include "rtxui/dom/element.hpp"

namespace rtxui {

// The component rendering `element`, as a `T`, or nullptr. For tests that
// read or change a built-in component's state, e.g. an <input>'s value.
template <typename T>
T* ComponentAs(const Element* element) {
  if (!element) {
    return nullptr;
  }
  // Tests own the whole tree, so they may change what an element shows.
  return dynamic_cast<T*>(const_cast<ComponentBase*>(element->component()));
}

}  // namespace rtxui

#endif  // RTXUI_TESTING_COMPONENT_AS_HPP_
