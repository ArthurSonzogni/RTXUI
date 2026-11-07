// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_BASE_CHARCONV_HPP_
#define RTXUI_BASE_CHARCONV_HPP_

#include <charconv>

namespace rtxui {

// std::from_chars for floating-point numbers, in its general format. libc++
// has none before version 20 (Xcode 16 included), so the library calls this
// instead: where the standard library does not advertise one, StrtoFromChars
// stands in.
std::from_chars_result FloatFromChars(const char* first,
                                      const char* last,
                                      float& value);
std::from_chars_result FloatFromChars(const char* first,
                                      const char* last,
                                      double& value);
std::from_chars_result FloatFromChars(const char* first,
                                      const char* last,
                                      long double& value);

#if !defined(_WIN32)
// The stand-in: std::from_chars's results, from strto*_l in the C locale.
// Built wherever it can be, so that it is tested on every platform.
std::from_chars_result StrtoFromChars(const char* first,
                                      const char* last,
                                      float& value);
std::from_chars_result StrtoFromChars(const char* first,
                                      const char* last,
                                      double& value);
std::from_chars_result StrtoFromChars(const char* first,
                                      const char* last,
                                      long double& value);
#endif

}  // namespace rtxui

#endif  // RTXUI_BASE_CHARCONV_HPP_
