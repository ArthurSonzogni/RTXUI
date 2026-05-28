// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/core/refcounted.hpp"

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

}  // namespace

}  // namespace rtxui
