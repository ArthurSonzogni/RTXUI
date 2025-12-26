// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "style/style.hpp"

#include <catch2/catch_test_macros.hpp>
#include <string>

#include "core/string.hpp"

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
