// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_BIND_HPP_
#define RTXUI_BIND_HPP_

// The binding macros, on their own: a C++ module cannot export macros, so
// code that does `import rtxui;` includes this header for them.
// <rtxui/rtxui.hpp> includes it already.

// What Bind() expands to needs these where it is used. Under `import rtxui;`
// nothing else makes them visible: GCC instantiates std::function in the
// importing file, and std::function uses typeid.
#include <functional>
#include <type_traits>
#include <typeinfo>

// Bind(x) registers state variables, computed properties, or callbacks.
#define Bind(x, ...) \
  this->Import(#x, &std::decay_t<decltype(*this)>::x, ##__VA_ARGS__)

// BindCollection(name, x) registers a collection with an explicit name.
#define BindCollection(name, ...) this->Import(name, ##__VA_ARGS__)

#endif  // RTXUI_BIND_HPP_
