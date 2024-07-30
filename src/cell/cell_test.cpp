// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.

#include "cell/cell.hpp"

#include <catch2/catch_test_macros.hpp>

namespace rtxui {
namespace {

template <typename T>
auto State(T&& value) {
  return Ref<TypedCell<T>>::New(std::forward<T>(value));
}

template <typename T>
auto Computed(std::function<T()> f) {
  return Ref<ComputedTypedCell<T>>::New(std::move(f));
}

auto Watch(std::function<void()> f) {
  return Ref<WatcherCaptureCell>::New(std::move(f));
}

auto Watch(Ref<Cell> cell, std::function<void()> f) {
  auto out = Ref<WatcherCell>::New(std::move(f));
  out->DependsOn(cell.get());
  return out;
}

TEST_CASE("Computed", "[cell]") {
  auto a = State(42);
  auto b = State(23);
  REQUIRE(a->Value() == 42);
  REQUIRE(b->Value() == 23);

  a->Value(43);
  REQUIRE(a->Value() == 43);
  REQUIRE(b->Value() == 23);

  // auto c = a + b;
  auto c = Computed<int>([&] { return a->Value() + b->Value(); });
  REQUIRE(a->Value() == 43);
  REQUIRE(b->Value() == 23);
  REQUIRE(c->Value() == 66);

  a->Value(44);
  REQUIRE(a->Value() == 44);
  REQUIRE(b->Value() == 23);
  REQUIRE(c->Value() == 67);

  b->Value(24);
  REQUIRE(a->Value() == 44);
  REQUIRE(b->Value() == 24);
  REQUIRE(c->Value() == 68);
}

TEST_CASE("Watch", "[cell]") {
  auto a = State(42);
  auto b = State(23);
  auto c = Computed<int>([&] { return a->Value() + b->Value(); });

  int count_a = 0;
  int count_b = 0;
  int count_c = 0;

  auto watcher_1 = Watch(a, [&] { ++count_a; });
  auto watcher_2 = Watch(b, [&] { ++count_b; });
  auto watcher_3 = Watch(c, [&] { ++count_c; });

  REQUIRE(count_a == 0);
  REQUIRE(count_b == 0);
  REQUIRE(count_c == 0);

  a->Value(43);
  REQUIRE(count_a == 1);
  REQUIRE(count_b == 0);
  REQUIRE(count_c == 1);

  b->Value(24);
  REQUIRE(count_a == 1);
  REQUIRE(count_b == 1);
  REQUIRE(count_c == 2);

  a->Value(43);
  REQUIRE(count_a == 1);
  REQUIRE(count_b == 1);
  REQUIRE(count_c == 2);
}

}  // namespace
}  // namespace rtxui
