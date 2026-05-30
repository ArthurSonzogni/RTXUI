// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/style/style.hpp"

#include <catch2/catch_test_macros.hpp>
#include <string>

#include "rtxui/core/string.hpp"
#include "rtxui/layout/style.hpp"
#include "rtxui/style/apply_style.hpp"

TEST_CASE("CSS parser works correctly", "[css]") {
  const std::string input = R"(
    div {
      color: red;
      background-color: blue;
    }
  )";

  const std::string expected =
      "div {\n  color: red;\n  background-color: blue;\n}\n\n";

  auto stylesheet = css::Parse(input);
  if (!stylesheet) {
    FAIL(stylesheet.error().message + " at line " +
         std::to_string(stylesheet.error().line) + " column " +
         std::to_string(stylesheet.error().column));
  }

  CHECK(stylesheet.value().size() == 1);
  CHECK(stylesheet.value()[0].selector == "div");
  CHECK(stylesheet.value()[0].declarations.size() == 2);
  CHECK(stylesheet.value()[0].declarations[0].property == "color");
  CHECK(stylesheet.value()[0].declarations[0].value == "red");
  CHECK(stylesheet.value()[0].declarations[1].property == "background-color");
  CHECK(stylesheet.value()[0].declarations[1].value == "blue");

  CHECK(css::Print(stylesheet.value()) == expected);
}

TEST_CASE("CSS with multiple rules", "[css]") {
  const std::string input = R"(
    h1 {
      font-size: 20;
    }
    p {
      color: green;
    }
  )";

  auto stylesheet = css::Parse(input);
  if (!stylesheet) {
    FAIL(stylesheet.error().message);
  }

  CHECK(stylesheet.value().size() == 2);
  CHECK(stylesheet.value()[0].selector == "h1");
  CHECK(stylesheet.value()[1].selector == "p");
}

TEST_CASE("CSS with comments", "[css]") {
  const std::string input = R"(
    /* This is a comment */
    div {
      /* Another comment */
      color: red; /* Inline comment */
    }
  )";

  auto stylesheet = css::Parse(input);
  if (!stylesheet) {
    FAIL(stylesheet.error().message);
  }

  CHECK(stylesheet.value().size() == 1);
  CHECK(stylesheet.value()[0].selector == "div");
  CHECK(stylesheet.value()[0].declarations.size() == 1);
  CHECK(stylesheet.value()[0].declarations[0].property == "color");
  CHECK(stylesheet.value()[0].declarations[0].value == "red");
}

TEST_CASE("CSS with complex selectors", "[css]") {
  const std::string input = R"(
    div > p.active #id {
      color: red;
    }
  )";

  auto stylesheet = css::Parse(input);
  if (!stylesheet) {
    FAIL(stylesheet.error().message);
  }

  CHECK(stylesheet.value()[0].selector == "div > p.active #id");
}

TEST_CASE("CSS with reactive bindings", "[css]") {
  const std::string input = R"(
    button {
      width: {width_state};
      color: {color_state};
    }
  )";

  auto stylesheet = css::Parse(input);
  if (!stylesheet) {
    FAIL(stylesheet.error().message);
  }

  CHECK(stylesheet.value().size() == 1);
  CHECK(stylesheet.value()[0].declarations[0].value == "{width_state}");
  CHECK(stylesheet.value()[0].declarations[1].value == "{color_state}");
}

TEST_CASE("CSS error handling - Missing brace", "[css]") {
  const std::string input = R"(
    div {
      color: red;
  )";
  auto stylesheet = css::Parse(input);
  CHECK(!stylesheet);
  CHECK(stylesheet.error().message == "Expected '}', but got end of file");
}

TEST_CASE("CSS error handling - Missing colon", "[css]") {
  const std::string input = R"(
    div {
      color red;
    }
  )";
  auto stylesheet = css::Parse(input);
  CHECK(!stylesheet);
  CHECK(stylesheet.error().message == "Expected ':', but got 'r'");
}

TEST_CASE("CSS with trailing semicolon optional", "[css]") {
  const std::string input = R"(
    div {
      color: red
    }
  )";

  auto stylesheet = css::Parse(input);
  CHECK(stylesheet);
  CHECK(stylesheet.value()[0].declarations[0].value == "red");
}

TEST_CASE("Color parsing in ApplyStyle", "[style][color]") {
  rtxui::ComputedStyle style;

  SECTION("Keywords") {
    rtxui::ApplyStyle(style, {"color", "red"});
    REQUIRE(style.foreground_color.has_value());
    CHECK(style.foreground_color->r == 255);
    CHECK(style.foreground_color->g == 0);
    CHECK(style.foreground_color->b == 0);
    CHECK(style.foreground_color->a == 255);

    rtxui::ApplyStyle(style, {"background-color", "white"});
    REQUIRE(style.background_color.has_value());
    CHECK(style.background_color->r == 255);
    CHECK(style.background_color->g == 255);
    CHECK(style.background_color->b == 255);
    CHECK(style.background_color->a == 255);

    rtxui::ApplyStyle(style, {"color", "silver"});
    REQUIRE(style.foreground_color.has_value());
    CHECK(style.foreground_color->r == 192);
    CHECK(style.foreground_color->g == 192);
    CHECK(style.foreground_color->b == 192);

    rtxui::ApplyStyle(style, {"color", "maroon"});
    REQUIRE(style.foreground_color.has_value());
    CHECK(style.foreground_color->r == 128);
    CHECK(style.foreground_color->g == 0);
    CHECK(style.foreground_color->b == 0);

    rtxui::ApplyStyle(style, {"color", "purple"});
    REQUIRE(style.foreground_color.has_value());
    CHECK(style.foreground_color->r == 128);
    CHECK(style.foreground_color->g == 0);
    CHECK(style.foreground_color->b == 128);

    rtxui::ApplyStyle(style, {"color", "lime"});
    REQUIRE(style.foreground_color.has_value());
    CHECK(style.foreground_color->r == 0);
    CHECK(style.foreground_color->g == 255);
    CHECK(style.foreground_color->b == 0);

    rtxui::ApplyStyle(style, {"color", "olive"});
    REQUIRE(style.foreground_color.has_value());
    CHECK(style.foreground_color->r == 128);
    CHECK(style.foreground_color->g == 128);
    CHECK(style.foreground_color->b == 0);

    rtxui::ApplyStyle(style, {"color", "navy"});
    REQUIRE(style.foreground_color.has_value());
    CHECK(style.foreground_color->r == 0);
    CHECK(style.foreground_color->g == 0);
    CHECK(style.foreground_color->b == 128);

    rtxui::ApplyStyle(style, {"color", "teal"});
    REQUIRE(style.foreground_color.has_value());
    CHECK(style.foreground_color->r == 0);
    CHECK(style.foreground_color->g == 128);
    CHECK(style.foreground_color->b == 128);
  }

  SECTION("rgb(...) function") {
    rtxui::ApplyStyle(style, {"color", "rgb(10, 20, 30)"});
    REQUIRE(style.foreground_color.has_value());
    CHECK(style.foreground_color->r == 10);
    CHECK(style.foreground_color->g == 20);
    CHECK(style.foreground_color->b == 30);
    CHECK(style.foreground_color->a == 255);
  }

  SECTION("rgba(...) function with scaled alpha") {
    rtxui::ApplyStyle(style, {"color", "rgba(10, 20, 30, 0.5)"});
    REQUIRE(style.foreground_color.has_value());
    CHECK(style.foreground_color->r == 10);
    CHECK(style.foreground_color->g == 20);
    CHECK(style.foreground_color->b == 30);
    CHECK(style.foreground_color->a == 127);
  }

  SECTION("Hex #RGB format") {
    rtxui::ApplyStyle(style, {"color", "#f0a"});
    REQUIRE(style.foreground_color.has_value());
    CHECK(style.foreground_color->r == 255);
    CHECK(style.foreground_color->g == 0);
    CHECK(style.foreground_color->b == 170);
    CHECK(style.foreground_color->a == 255);
  }

  SECTION("Hex #RGBA format") {
    rtxui::ApplyStyle(style, {"color", "#f0a8"});
    REQUIRE(style.foreground_color.has_value());
    CHECK(style.foreground_color->r == 255);
    CHECK(style.foreground_color->g == 0);
    CHECK(style.foreground_color->b == 170);
    CHECK(style.foreground_color->a == 136);
  }

  SECTION("Hex #RRGGBB format") {
    rtxui::ApplyStyle(style, {"color", "#ff00aa"});
    REQUIRE(style.foreground_color.has_value());
    CHECK(style.foreground_color->r == 255);
    CHECK(style.foreground_color->g == 0);
    CHECK(style.foreground_color->b == 170);
    CHECK(style.foreground_color->a == 255);
  }

  SECTION("Hex #RRGGBBAA format") {
    rtxui::ApplyStyle(style, {"color", "#ff00aa88"});
    REQUIRE(style.foreground_color.has_value());
    CHECK(style.foreground_color->r == 255);
    CHECK(style.foreground_color->g == 0);
    CHECK(style.foreground_color->b == 170);
    CHECK(style.foreground_color->a == 136);
  }
}
