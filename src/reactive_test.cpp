#include "reactive.hpp"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

namespace {

TEST_CASE("Reactive null", "[reactive]") {
  reactive::Reactive r;
  REQUIRE(r.IsNull());
  REQUIRE(!r.IsBool());
  REQUIRE(!r.IsInt());
  REQUIRE(!r.IsDouble());
  REQUIRE(!r.IsString());
  REQUIRE(!r.IsArray());
  REQUIRE(!r.IsMap());
  REQUIRE(!r.IsSet());
  REQUIRE(r.Type() == reactive::Type::kNull);
}

TEST_CASE("Reactive bool", "[reactive]") {
  reactive::Reactive r(true);
  REQUIRE(!r.IsNull());
  REQUIRE(r.IsBool());
  REQUIRE(!r.IsInt());
  REQUIRE(!r.IsDouble());
  REQUIRE(!r.IsString());
  REQUIRE(!r.IsArray());
  REQUIRE(!r.IsMap());
  REQUIRE(!r.IsSet());
  REQUIRE(r.AsBool());
  REQUIRE(r.Type() == reactive::Type::kBool);
}

TEST_CASE("Reactive int", "[reactive]") {
  reactive::Reactive r(42);
  REQUIRE(!r.IsNull());
  REQUIRE(!r.IsBool());
  REQUIRE(r.IsInt());
  REQUIRE(!r.IsDouble());
  REQUIRE(!r.IsString());
  REQUIRE(!r.IsArray());
  REQUIRE(!r.IsMap());
  REQUIRE(!r.IsSet());
  REQUIRE(r.AsInt() == 42);
  REQUIRE(r.Type() == reactive::Type::kInt);
}

TEST_CASE("Reactive double", "[reactive]") {
  reactive::Reactive r(3.14);
  REQUIRE(!r.IsNull());
  REQUIRE(!r.IsBool());
  REQUIRE(!r.IsInt());
  REQUIRE(r.IsDouble());
  REQUIRE(!r.IsString());
  REQUIRE(!r.IsArray());
  REQUIRE(!r.IsMap());
  REQUIRE(!r.IsSet());
  REQUIRE(r.AsDouble() == 3.14);
  REQUIRE(r.Type() == reactive::Type::kDouble);
}

TEST_CASE("Reactive string", "[reactive]") {
  reactive::Reactive r("hello");
  REQUIRE(!r.IsNull());
  REQUIRE(!r.IsBool());
  REQUIRE(!r.IsInt());
  REQUIRE(!r.IsDouble());
  REQUIRE(r.IsString());
  REQUIRE(!r.IsArray());
  REQUIRE(!r.IsMap());
  REQUIRE(!r.IsSet());
  REQUIRE(r.AsString() == "hello");
  REQUIRE(r.Type() == reactive::Type::kString);
}

TEST_CASE("Reactive array", "[reactive]") {
  auto r = reactive::Array();
  REQUIRE(!r.IsNull());
  REQUIRE(!r.IsBool());
  REQUIRE(!r.IsInt());
  REQUIRE(!r.IsDouble());
  REQUIRE(!r.IsString());
  REQUIRE(r.IsArray());
  REQUIRE(!r.IsMap());
  REQUIRE(!r.IsSet());
  REQUIRE(r.Type() == reactive::Type::kArray);
}

TEST_CASE("Reactive map", "[reactive]") {
  auto r = reactive::Map();
  REQUIRE(!r.IsNull());
  REQUIRE(!r.IsBool());
  REQUIRE(!r.IsInt());
  REQUIRE(!r.IsDouble());
  REQUIRE(!r.IsString());
  REQUIRE(!r.IsArray());
  REQUIRE(r.IsMap());
  REQUIRE(!r.IsSet());
  REQUIRE(r.Type() == reactive::Type::kMap);
}

TEST_CASE("Reactive set", "[reactive]") {
  auto r = reactive::Set();
  REQUIRE(!r.IsNull());
  REQUIRE(!r.IsBool());
  REQUIRE(!r.IsInt());
  REQUIRE(!r.IsDouble());
  REQUIRE(!r.IsString());
  REQUIRE(!r.IsArray());
  REQUIRE(!r.IsMap());
  REQUIRE(r.IsSet());
  REQUIRE(r.empty());
  REQUIRE(r.Type() == reactive::Type::kSet);
}

TEST_CASE("Reactive copy", "[reactive]") {
  auto r1 = reactive::Reactive(42);
  auto r2 = r1;
  REQUIRE(r1 == r2);
  REQUIRE(r1.AsInt() == 42);
  REQUIRE(r2.AsInt() == 42);
}

TEST_CASE("Reactive move", "[reactive]") {
  auto r1 = reactive::Reactive(42);
  auto r2 = std::move(r1);
  REQUIRE(r1.IsNull());
  REQUIRE(!r2.IsNull());
  REQUIRE(r2.AsInt() == 42);
}

TEST_CASE("Reactive assign", "[reactive]") {
  auto r1 = reactive::Reactive(42);
  auto r2 = reactive::Reactive(43);
  r1 = r2;
  REQUIRE(r1 == r2);
  REQUIRE(r1.AsInt() == 43);
  REQUIRE(r2.AsInt() == 43);
}

TEST_CASE("Reactive assign move", "[reactive]") {
  auto r1 = reactive::Reactive(42);
  auto r2 = reactive::Reactive(43);
  r1 = std::move(r2);
  REQUIRE(r2.IsNull());
  REQUIRE(!r1.IsNull());
  REQUIRE(r1.AsInt() == 43);
}

TEST_CASE("Reactive equal", "[reactive]") {
  auto r1 = reactive::Reactive(42);
  auto r2 = reactive::Reactive(42);
  auto r3 = reactive::Reactive(43);
  REQUIRE(r1 != r2);
  REQUIRE(r1 != r3);
}

TEST_CASE("Reactive bool assignment", "[reactive]") {
  auto r = reactive::Reactive();
  r = true;
  REQUIRE(r.IsBool());
  REQUIRE(r.AsBool());
}

TEST_CASE("Reactive addition int", "[reactive]") {
  auto r1 = reactive::Reactive(42);
  auto r2 = reactive::Reactive(43);
  auto r3 = r1 + r2;
  REQUIRE(r3.IsInt());
  REQUIRE(r3.AsInt() == 85);
}

TEST_CASE("Reactive addition double", "[reactive]") {
  auto r1 = reactive::Reactive(3.14);
  auto r2 = reactive::Reactive(2.71);
  auto r3 = r1 + r2;
  REQUIRE(r3.IsDouble());
  REQUIRE(r3.AsDouble() == 5.85);
}

TEST_CASE("Reactive subtraction int", "[reactive]") {
  auto r1 = reactive::Reactive(42);
  auto r2 = reactive::Reactive(43);
  auto r3 = r1 - r2;
  REQUIRE(r3.IsInt());
  REQUIRE(r3.AsInt() == -1);
}

TEST_CASE("Reactive subtraction double", "[reactive]") {
  auto r1 = reactive::Reactive(3.14);
  auto r2 = reactive::Reactive(2.71);
  auto r3 = r1 - r2;
  REQUIRE(r3.IsDouble());
  REQUIRE(r3.AsDouble() == Catch::Approx(0.43));
}

TEST_CASE("Reactive multiplication int", "[reactive]") {
  auto r1 = reactive::Reactive(42);
  auto r2 = reactive::Reactive(43);
  auto r3 = r1 * r2;
  REQUIRE(r3.IsInt());
  REQUIRE(r3.AsInt() == 1806);
}

TEST_CASE("Reactive multiplication double", "[reactive]") {
  auto r1 = reactive::Reactive(3.14);
  auto r2 = reactive::Reactive(2.71);
  auto r3 = r1 * r2;
  REQUIRE(r3.IsDouble());
  REQUIRE(r3.AsDouble() == Catch::Approx(8.5094));
}

TEST_CASE("Reactive division int", "[reactive]") {
  auto r1 = reactive::Reactive(42);
  auto r2 = reactive::Reactive(43);
  auto r3 = r1 / r2;
  REQUIRE(r3.IsInt());
  REQUIRE(r3.AsInt() == 0);
}

TEST_CASE("Reactive division double", "[reactive]") {
  auto r1 = reactive::Reactive(3.14);
  auto r2 = reactive::Reactive(2.71);
  auto r3 = r1 / r2;
  REQUIRE(r3.IsDouble());
  REQUIRE(r3.AsDouble() == Catch::Approx(1.15867158672));
}

TEST_CASE("Reactive modulo", "[reactive]") {
  auto r1 = reactive::Reactive(42);
  auto r2 = reactive::Reactive(43);
  auto r3 = r1 % r2;
  REQUIRE(r3.IsInt());
  REQUIRE(r3.AsInt() == 42);
}

TEST_CASE("Reactive logical and", "[reactive]") {
  auto r1 = reactive::Reactive(true);
  auto r2 = reactive::Reactive(false);
  auto r3 = r1 && r2;
  REQUIRE(r3.IsBool());
  REQUIRE(!r3.AsBool());
}

TEST_CASE("Reactive logical or", "[reactive]") {
  auto r1 = reactive::Reactive(true);
  auto r2 = reactive::Reactive(false);
  auto r3 = r1 || r2;
  REQUIRE(r3.IsBool());
  REQUIRE(r3.AsBool());
}

TEST_CASE("Reactive logical not", "[reactive]") {
  auto r1 = reactive::Reactive(true);
  auto r2 = !r1;
  REQUIRE(r2.IsBool());
  REQUIRE(!r2.AsBool());
}

TEST_CASE("Reactive array index", "[reactive]") {
  auto r = reactive::Array();
  r.push_back(42);
  r.push_back(43);

  REQUIRE(r[0].Type() == reactive::Type::kInt);
  REQUIRE(r[0].IsInt());
  REQUIRE(r[0].AsInt() == 42);
  REQUIRE(r[1].IsInt());
  REQUIRE(r[1].AsInt() == 43);
}

TEST_CASE("Reactive array push_back", "[reactive]") {
  auto r = reactive::Array();
  r.push_back(42);
  r.push_back(43);
  REQUIRE(r.size() == 2);
  REQUIRE(!r.empty());
}

TEST_CASE("Reactive map insert", "[reactive]") {
  auto r = reactive::Map();
  r.insert("a", 42);
  r.insert("b", 43);
  REQUIRE(r.size() == 2);
  REQUIRE(!r.empty());
}

TEST_CASE("Reactive set insert", "[reactive]") {
  auto r = reactive::Set();
  r.insert(42);
  r.insert(43);
  REQUIRE(r.size() == 2);
  REQUIRE(!r.empty());
}

TEST_CASE("Reactive array erase", "[reactive]") {
  auto r = reactive::Array();
  r.push_back(42);
  r.push_back(43);
  r.erase(0);
  REQUIRE(r.size() == 1);
  REQUIRE(!r.empty());
  REQUIRE(r[0].AsInt() == 43);
}

TEST_CASE("Reactive map erase", "[reactive]") {
  auto r = reactive::Map();
  r.insert("a", 42);
  r.insert("b", 43);
  r.erase("a");
  REQUIRE(r.size() == 1);
  REQUIRE(!r.empty());
  REQUIRE(r["b"].AsInt() == 43);
}

TEST_CASE("Reactive clear", "[reactive]") {
  auto r = reactive::Array();
  r.push_back(42);
  r.push_back(43);
  r.clear();
  REQUIRE(r.size() == 0);
  REQUIRE(r.empty());
}

TEST_CASE("Reactive array comparison", "[reactive]") {
  auto r1 = reactive::Array();
  r1.push_back(42);
  r1.push_back(43);

  auto r2 = reactive::Array();
  r2.push_back(42);
  r2.push_back(43);

  auto r3 = reactive::Array();
  r3.push_back(42);
  r3.push_back(44);

  REQUIRE(r1 != r2);
  REQUIRE(r1 != r3);
}

TEST_CASE("Reactive array size", "[reactive]") {
  auto r = reactive::Array();
  REQUIRE(r.size() == 0);
  REQUIRE(r.empty());

  r.push_back(42);
  REQUIRE(r.size() == 1);
  REQUIRE(!r.empty());

  r.push_back(43);
  REQUIRE(r.size() == 2);
  REQUIRE(!r.empty());
}

TEST_CASE("Reactive empty array", "[reactive]") {
  auto r = reactive::Array();
  REQUIRE(r.empty());
}

TEST_CASE("Reactive empty map", "[reactive]") {
  auto r = reactive::Map();
  REQUIRE(r.empty());
}

TEST_CASE("Reactive empty set", "[reactive]") {
  auto r = reactive::Set();
  REQUIRE(r.empty());
}

}  // namespace
