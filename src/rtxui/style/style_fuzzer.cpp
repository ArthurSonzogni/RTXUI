// Copyright 2025 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/style/style.hpp"

#include <fuzztest/fuzztest.h>
#include <gtest/gtest.h>

void TestStyle(const std::string& input) {
  auto stylesheet = css::Parse(input);
  if (stylesheet) {
    // If parsing is successful, ensure the stylesheet can be printed.
    (void)css::Print(stylesheet.value());
  }
}

FUZZ_TEST(Style, TestStyle);
