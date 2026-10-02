// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/internal/refcounted.hpp"

#include <catch2/catch_test_macros.hpp>

namespace rtxui {

namespace {

class Printer final : public RefCounted {
 public:
  // NOLINTNEXTLINE
  Printer(std::string name, std::vector<std::string>& out)
      : name_(std::move(name)), out_(out) {
    out_.push_back(name_ + " constructed");
  }

  ~Printer() final { out_.push_back(name_ + " destructed"); }

 private:
  std::string name_;
  std::vector<std::string>& out_;
};

TEST_CASE("basic", "[refcount]") {
  std::vector<std::string> out;
  {
    auto a = Ref<Printer>::New("a", out);
    auto b = Ref<Printer>::New("b", out);
  }
  REQUIRE(out == std::vector<std::string>{
                     "a constructed",
                     "b constructed",
                     "b destructed",
                     "a destructed",
                 });
}

TEST_CASE("copy", "[refcount]") {
  std::vector<std::string> out;
  {
    auto a = Ref<Printer>::New("a", out);
    auto b = Ref<Printer>::New("b", out);
    a = b;
  }
  REQUIRE(out == std::vector<std::string>{
                     "a constructed",
                     "b constructed",
                     "a destructed",
                     "b destructed",
                 });
}

TEST_CASE("move", "[refcount]") {
  std::vector<std::string> out;
  {
    auto a = Ref<Printer>::New("a", out);
    auto b = std::move(a);
  }
  REQUIRE(out == std::vector<std::string>{
                     "a constructed",
                     "a destructed",
                 });
}

TEST_CASE("capture", "[refcount]") {
  std::vector<std::string> out;
  {
    auto a = Ref<Printer>::New("a", out);
    auto b = Ref<Printer>::New("b", out);
    std::swap(a, b);
  }
  REQUIRE(out == std::vector<std::string>{
                     "a constructed",
                     "b constructed",
                     "a destructed",
                     "b destructed",
                 });
}

TEST_CASE("more references than a 16-bit count holds", "[refcount]") {
  std::vector<std::string> out;
  {
    auto a = Ref<Printer>::New("a", out);
    std::vector<Ref<Printer>> copies(70000, a);
    copies.resize(4464);  // 70000 - 65536: a 16-bit count would reach 0 here.
    REQUIRE(out == std::vector<std::string>{"a constructed"});
  }
  REQUIRE(out == std::vector<std::string>{"a constructed", "a destructed"});
}

TEST_CASE("move assignment releases the previous object", "[refcount]") {
  std::vector<std::string> out;
  auto a = Ref<Printer>::New("a", out);
  auto b = Ref<Printer>::New("b", out);
  a = std::move(b);
  REQUIRE(out == std::vector<std::string>{
                     "a constructed",
                     "b constructed",
                     "a destructed",
                 });
  REQUIRE(b.get() == nullptr);  // NOLINT(bugprone-use-after-move): the point.
}

static_assert(std::is_nothrow_move_constructible_v<Ref<Printer>>);
static_assert(std::is_nothrow_move_assignable_v<Ref<Printer>>);
static_assert(!std::is_convertible_v<Printer*, Ref<Printer>>,
              "adopting a raw pointer must be spelled out");

}  // namespace

}  // namespace rtxui
