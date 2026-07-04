// Copyright 2025 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <catch2/catch_test_macros.hpp>

#include "rtxui/internal/class_name.hpp"
#include "rtxui/internal/component.hpp"

namespace {
class A {};
namespace ns {
class B {};
}  // namespace ns
template <typename T>
class C {};

TEST_CASE("ClassName", "[reflection]") {
  // Test with a simple class.
  REQUIRE(rtxui::ClassName<A>() == "A");

  // Test with a class in a namespace.
  REQUIRE(rtxui::ClassName<ns::B>() == "B");

  // Test with a template class.
  REQUIRE(rtxui::ClassName<C<int>>() == "C<int>");
}

TEST_CASE("Reflection Float Serialization", "[reflection]") {
  SECTION("Float to_string") {
    float f1 = 0.7f;
    REQUIRE(rtxui::reflection::to_string(f1) == "0.7");

    float f2 = 0.0f;
    REQUIRE(rtxui::reflection::to_string(f2) == "0");

    double d1 = 123.456;
    REQUIRE(rtxui::reflection::to_string(d1) == "123.456");
  }

  SECTION("Float from_string") {
    float f1 = 0.0f;
    rtxui::reflection::from_string("0.7", f1);
    REQUIRE(f1 == 0.7f);

    float f2 = -1.0f;
    rtxui::reflection::from_string("0", f2);
    REQUIRE(f2 == 0.0f);

    double d1 = 0.0;
    rtxui::reflection::from_string("  123.456  ", d1);
    REQUIRE(d1 == 123.456);

    float f3 = 5.0f;
    rtxui::reflection::from_string("invalid", f3);
    REQUIRE(f3 == 0.0f);
  }
}

}  // namespace
