// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/base/charconv.hpp"

#include <charconv>

#if !defined(_WIN32)
#include <locale.h>  // NOLINT(modernize-deprecated-headers): newlocale.
#include <stdlib.h>  // NOLINT(modernize-deprecated-headers): strto*_l.

#include <cctype>
#include <cerrno>
#include <string>
#include <system_error>
#include <type_traits>
#if defined(__APPLE__) || defined(__FreeBSD__)
#include <xlocale.h>  // strto*_l
#endif
#endif

namespace rtxui {

#if !defined(_WIN32)
namespace {

template <typename T>
std::from_chars_result Strto(const char* first, const char* last, T& value) {
  // strto* also read leading spaces, a '+' and hexadecimal: from_chars does
  // not.
  const char* digits = first;
  if (digits != last && *digits == '-') {
    ++digits;
  }
  if (digits == last || std::isspace(static_cast<unsigned char>(*digits)) ||
      *digits == '+') {
    return {first, std::errc::invalid_argument};
  }
  if (*digits == '0' && digits + 1 != last &&
      (digits[1] == 'x' || digits[1] == 'X')) {
    value = digits == first ? T(0) : -T(0);
    return {digits + 1, std::errc()};
  }

  // The decimal point is '.' whatever locale the application set.
  static const locale_t c_locale = newlocale(LC_ALL_MASK, "C", locale_t{});
  const std::string text(first, last);
  char* end = nullptr;
  errno = 0;
  T parsed = 0;
  if constexpr (std::is_same_v<T, float>) {
    parsed = strtof_l(text.c_str(), &end, c_locale);
  } else if constexpr (std::is_same_v<T, double>) {
    parsed = strtod_l(text.c_str(), &end, c_locale);
  } else {
    parsed = strtold_l(text.c_str(), &end, c_locale);
  }
  if (end == text.c_str()) {
    return {first, std::errc::invalid_argument};
  }
  const char* ptr = first + (end - text.c_str());
  if (errno == ERANGE) {
    return {ptr, std::errc::result_out_of_range};
  }
  value = parsed;
  return {ptr, std::errc()};
}

}  // namespace

std::from_chars_result StrtoFromChars(const char* first,
                                      const char* last,
                                      float& value) {
  return Strto(first, last, value);
}
std::from_chars_result StrtoFromChars(const char* first,
                                      const char* last,
                                      double& value) {
  return Strto(first, last, value);
}
std::from_chars_result StrtoFromChars(const char* first,
                                      const char* last,
                                      long double& value) {
  return Strto(first, last, value);
}
#endif

#if defined(__cpp_lib_to_chars)
std::from_chars_result FloatFromChars(const char* first,
                                      const char* last,
                                      float& value) {
  return std::from_chars(first, last, value);
}
std::from_chars_result FloatFromChars(const char* first,
                                      const char* last,
                                      double& value) {
  return std::from_chars(first, last, value);
}
std::from_chars_result FloatFromChars(const char* first,
                                      const char* last,
                                      long double& value) {
  return std::from_chars(first, last, value);
}
#else
std::from_chars_result FloatFromChars(const char* first,
                                      const char* last,
                                      float& value) {
  return StrtoFromChars(first, last, value);
}
std::from_chars_result FloatFromChars(const char* first,
                                      const char* last,
                                      double& value) {
  return StrtoFromChars(first, last, value);
}
std::from_chars_result FloatFromChars(const char* first,
                                      const char* last,
                                      long double& value) {
  return StrtoFromChars(first, last, value);
}
#endif

}  // namespace rtxui
