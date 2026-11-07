// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/base/charconv.hpp"

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <charconv>
#include <clocale>
#include <cmath>
#include <string>
#include <string_view>
#include <system_error>

namespace rtxui {
namespace {

constexpr std::string_view kInputs[] = {
    "0",     "1",     "-1",    "1.5",   ".5",    "-.5",         "10px",
    "50%",   "1e3",   "1e3px", "1.5em", "2e",    "2e+",         "-0",
    "1e-3",  "1e400", "",      "-",     ".",     "+1",          " 1",
    "abc",   "0x10",  "-0x1",  "inf",   "-inf",  "nan",         "1,5",
    "1.2.3", "007",   "12deg", "255)",  "0.25s", "3.40282e+38",
};

template <typename T>
void CheckSameAsStandard(std::string_view input) {
  CAPTURE(input);
  T expected = 42;
  T actual = 42;
  const char* first = input.data();
  const char* last = input.data() + input.size();
  const auto want = std::from_chars(first, last, expected);
  const auto got = StrtoFromChars(first, last, actual);
  CHECK(got.ec == want.ec);
  CHECK(got.ptr - first == want.ptr - first);
  if (std::isnan(expected)) {
    CHECK(std::isnan(actual));
  } else {
    CHECK(actual == expected);
  }
  CHECK(std::signbit(actual) == std::signbit(expected));
}

}  // namespace

#if !defined(_WIN32) && defined(__cpp_lib_to_chars)
TEST_CASE("StrtoFromChars reads numbers as std::from_chars does",
          "[base][charconv]") {
  for (std::string_view input : kInputs) {
    CheckSameAsStandard<float>(input);
    CheckSameAsStandard<double>(input);
    CheckSameAsStandard<long double>(input);
  }
}

TEST_CASE("StrtoFromChars ignores the application's decimal comma",
          "[base][charconv]") {
  const std::string previous = std::setlocale(LC_NUMERIC, nullptr);
  if (!std::setlocale(LC_NUMERIC, "fr_FR.UTF-8") &&
      !std::setlocale(LC_NUMERIC, "de_DE.UTF-8")) {
    SKIP("no locale with a decimal comma is installed");
  }
  float value = 0;
  const std::string_view text = "0.5";
  const auto result =
      StrtoFromChars(text.data(), text.data() + text.size(), value);
  std::setlocale(LC_NUMERIC, previous.c_str());
  CHECK(result.ec == std::errc());
  CHECK(result.ptr == text.data() + text.size());
  CHECK(value == 0.5f);
}
#endif

TEST_CASE("FloatFromChars reads a CSS number", "[base][charconv]") {
  float value = 0;
  const std::string_view text = "1.5em";
  const auto result =
      FloatFromChars(text.data(), text.data() + text.size(), value);
  CHECK(result.ec == std::errc());
  CHECK(result.ptr == text.data() + 3);
  CHECK(value == 1.5f);
}

}  // namespace rtxui
