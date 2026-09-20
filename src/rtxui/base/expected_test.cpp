// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/base/expected.hpp"

#include <catch2/catch_test_macros.hpp>
#include <string>

TEST_CASE("Expected template wrapper test", "[base][expected]") {
  SECTION("Value representation") {
    Expected<int, std::string> exp(42);
    CHECK(exp.has_value() == true);
    CHECK(static_cast<bool>(exp) == true);
    CHECK(exp.value() == 42);
  }

  SECTION("Error representation") {
    Expected<int, std::string> exp(std::string("failed"));
    CHECK(exp.has_value() == false);
    CHECK(static_cast<bool>(exp) == false);
    CHECK(exp.error() == "failed");
  }
}
