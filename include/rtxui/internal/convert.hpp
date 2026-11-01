// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_INTERNAL_CONVERT_HPP_
#define RTXUI_INTERNAL_CONVERT_HPP_

// Not part of the API: how bound values are converted to and from the text a
// template reads and writes. In a public header only because the templates
// that bind the application's own types instantiate it.

#include <ostream>
#include <rtxui/rtxui_export.hpp>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>

namespace rtxui::internal {

RTXUI_EXPORT int ParseInt(std::string_view str);

// Floating-point conversions live in the library: on macOS, the standard
// library marks std::to_chars and std::from_chars on floating-point types
// unavailable before recent versions, which would otherwise constrain every
// application's deployment target. ParseFloat sets 0 on invalid text.
RTXUI_EXPORT std::string FormatFloat(float value);
RTXUI_EXPORT std::string FormatFloat(double value);
RTXUI_EXPORT std::string FormatFloat(long double value);
RTXUI_EXPORT void ParseFloat(std::string_view str, float& value);
RTXUI_EXPORT void ParseFloat(std::string_view str, double& value);
RTXUI_EXPORT void ParseFloat(std::string_view str, long double& value);

template <typename T>
std::string to_string(const T& value) {
  if constexpr (std::is_same_v<T, bool>) {
    return value ? "true" : "false";
  } else if constexpr (std::is_same_v<T, char>) {
    return std::string(1, value);
  }
  if constexpr (std::is_floating_point_v<T>) {
    return FormatFloat(value);
  } else if constexpr (std::is_convertible_v<T, std::string>) {
    return static_cast<std::string>(value);
  } else if constexpr (requires { std::to_string(value); }) {
    return std::to_string(value);
  } else if constexpr (requires(std::ostream& os, const T& v) { os << v; }) {
    std::stringstream ss;
    ss << value;
    return ss.str();
  } else {
    return "";
  }
}

template <typename T>
void from_string(std::string_view str, T& value) {
  if constexpr (std::is_same_v<T, std::string>) {
    value = std::string(str);
  } else if constexpr (std::is_same_v<T, int>) {
    value = ParseInt(str);
  } else if constexpr (std::is_same_v<T, bool>) {
    value = (str == "true" || str == "1");
  } else if constexpr (std::is_floating_point_v<T>) {
    ParseFloat(str, value);
  } else {
    std::stringstream ss;
    ss << str;
    ss >> value;
  }
}

}  // namespace rtxui::internal

#endif  // RTXUI_INTERNAL_CONVERT_HPP_
