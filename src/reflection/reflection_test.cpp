// Copyright 2025 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "class_name.hpp"

#include <catch2/catch_test_macros.hpp>

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

}  // namespace
