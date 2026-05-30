// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#pragma once
#ifndef RTXUI_REFLECTION_CLASS_NAME_HPP_
#define RTXUI_REFLECTION_CLASS_NAME_HPP_
#include <string_view>

namespace rtxui {

template <typename T>
constexpr std::string_view ClassName() {
#if defined(__clang__)
  constexpr std::string_view full = __PRETTY_FUNCTION__;
  constexpr std::string_view prefix = "ClassName() [T = ";
  constexpr std::string_view suffix = "]";
#elif defined(__GNUC__)
  constexpr std::string_view full = __PRETTY_FUNCTION__;
  constexpr std::string_view prefix = "ClassName() [with T = ";
  constexpr std::string_view suffix = ";";
#elif defined(_MSC_VER)
  const std::string_view full = __FUNCSIG__;
  constexpr std::string_view prefix = "ClassName<";
  constexpr std::string_view suffix = ">(void)";
#endif

  const auto start = full.find(prefix) + prefix.size();
  const auto end = full.rfind(suffix);
  const auto typename_full = full.substr(start, end - start);

  // Strip namespaces
  const auto last_colon = typename_full.rfind("::");
  if (last_colon != std::string_view::npos) {
    return typename_full.substr(last_colon + 2);  // after last "::"
  } else {
    return typename_full;
  }
}

}  // namespace rtxui
#endif  // RTXUI_REFLECTION_CLASS_NAME_HPP_
