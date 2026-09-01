// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/style/style.hpp"

#include <iostream>
#include <catch2/catch_test_macros.hpp>
#include "rtxui/paint/color.hpp"
#include <cmath>
#include <string>

#include "rtxui/base/string.hpp"
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
  auto parsed = stylesheet.value()[0].parsed_selector;
  CHECK(parsed.base == "");
  CHECK(parsed.id == "id");
  REQUIRE(parsed.parents.size() == 2);
  CHECK(parsed.parents[0].base == "p");
  CHECK(parsed.parents[0].classes == std::vector<std::string>{"active"});
  CHECK(parsed.parents[0].combinator == ' ');
  CHECK(parsed.parents[1].base == "div");
  CHECK(parsed.parents[1].combinator == '>');
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

TEST_CASE("CSS attribute selectors", "[css]") {
  const std::string input = R"(
    div[attr="val"] {
      color: red;
    }
    span[data-test] {
      color: blue;
    }
  )";

  auto stylesheet = css::Parse(input);
  REQUIRE(stylesheet);
  REQUIRE(stylesheet.value().size() == 2);

  CHECK(stylesheet.value()[0].parsed_selector.base == "div");
  REQUIRE(stylesheet.value()[0].parsed_selector.attributes.size() == 1);
  CHECK(stylesheet.value()[0].parsed_selector.attributes[0].name == "attr");
  CHECK(stylesheet.value()[0].parsed_selector.attributes[0].value == "val");
  CHECK(stylesheet.value()[0].parsed_selector.attributes[0].has_value == true);

  CHECK(stylesheet.value()[1].parsed_selector.base == "span");
  REQUIRE(stylesheet.value()[1].parsed_selector.attributes.size() == 1);
  CHECK(stylesheet.value()[1].parsed_selector.attributes[0].name ==
        "data-test");
  CHECK(stylesheet.value()[1].parsed_selector.attributes[0].has_value == false);
}

TEST_CASE("CSS nesting", "[css]") {
  const std::string input = R"(
    div {
      color: red;
      &:hover {
        color: blue;
      }
      span {
        color: green;
      }
      &.active, &.focus {
        background-color: yellow;
      }
    }
  )";

  auto stylesheet = css::Parse(input);
  if (!stylesheet) {
    FAIL(stylesheet.error().message);
  }

  REQUIRE(stylesheet.value().size() == 5);

  CHECK(stylesheet.value()[0].selector == "div");
  CHECK(stylesheet.value()[0].declarations[0].property == "color");
  CHECK(stylesheet.value()[0].declarations[0].value == "red");

  CHECK(stylesheet.value()[1].selector == "div:hover");
  CHECK(stylesheet.value()[1].declarations[0].property == "color");
  CHECK(stylesheet.value()[1].declarations[0].value == "blue");

  CHECK(stylesheet.value()[2].selector == "div span");
  CHECK(stylesheet.value()[2].declarations[0].property == "color");
  CHECK(stylesheet.value()[2].declarations[0].value == "green");

  CHECK(stylesheet.value()[3].selector == "div.active");
  CHECK(stylesheet.value()[3].declarations[0].property == "background-color");
  CHECK(stylesheet.value()[3].declarations[0].value == "yellow");

  CHECK(stylesheet.value()[4].selector == "div.focus");
  CHECK(stylesheet.value()[4].declarations[0].property == "background-color");
  CHECK(stylesheet.value()[4].declarations[0].value == "yellow");
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

TEST_CASE("CSS with media queries", "[css][media]") {
  const std::string input = R"(
    div {
      color: red;
    }
    @media (max-width: 80) {
      span {
        color: blue;
      }
    }
    @media (min-width: 100) and (max-height: 50) {
      button {
        color: green;
      }
    }
  )";

  auto stylesheet = css::Parse(input);
  if (!stylesheet) {
    FAIL(stylesheet.error().message);
  }

  REQUIRE(stylesheet.value().size() == 3);

  CHECK(stylesheet.value()[0].selector == "div");
  CHECK(stylesheet.value()[0].media_query.empty());
  CHECK(stylesheet.value()[0].declarations.size() == 1);

  CHECK(stylesheet.value()[1].selector == "span");
  CHECK(stylesheet.value()[1].media_query == "(max-width: 80)");
  CHECK(stylesheet.value()[1].declarations.size() == 1);

  CHECK(stylesheet.value()[2].selector == "button");
  CHECK(stylesheet.value()[2].media_query ==
        "(min-width: 100) and (max-height: 50)");
  CHECK(stylesheet.value()[2].declarations.size() == 1);

  // Test evaluation of media queries
  css::g_terminal_width = 80;
  css::g_terminal_height = 24;

  CHECK(css::EvaluateMediaQuery(""));
  CHECK(css::EvaluateMediaQuery("(max-width: 80)"));
  CHECK(css::EvaluateMediaQuery("(min-width: 60)"));
  CHECK(css::EvaluateMediaQuery("(max-width: 120) and (min-width: 60)"));
  CHECK(css::EvaluateMediaQuery("(max-height: 30)"));
  CHECK(css::EvaluateMediaQuery("(min-height: 20)"));

  // False conditions
  CHECK_FALSE(css::EvaluateMediaQuery("(max-width: 79)"));
  CHECK_FALSE(css::EvaluateMediaQuery("(min-width: 81)"));
  CHECK_FALSE(css::EvaluateMediaQuery("(max-height: 20)"));
  CHECK_FALSE(css::EvaluateMediaQuery("(min-height: 30)"));
  CHECK_FALSE(css::EvaluateMediaQuery("(max-width: 120) and (min-width: 90)"));
}

TEST_CASE("Text decoration parsing in ApplyStyle", "[style][text-decoration]") {
  rtxui::ComputedStyle style;

  SECTION("underline") {
    rtxui::ApplyStyle(style, {"text-decoration", "underline"});
    CHECK(style.underlined.has_value());
    CHECK(style.underlined.value() == true);
    CHECK(style.underlined_double.value_or(false) == false);
    CHECK(style.strikethrough.value_or(false) == false);
    CHECK(style.blink.value_or(false) == false);
  }

  SECTION("underlined") {
    rtxui::ApplyStyle(style, {"text-decoration", "underlined"});
    CHECK(style.underlined.has_value());
    CHECK(style.underlined.value() == true);
    CHECK(style.underlined_double.value_or(false) == false);
  }

  SECTION("double-underline") {
    rtxui::ApplyStyle(style, {"text-decoration", "double-underline"});
    CHECK(style.underlined.value_or(false) == false);
    CHECK(style.underlined_double.has_value());
    CHECK(style.underlined_double.value() == true);
  }

  SECTION("underlined-double") {
    rtxui::ApplyStyle(style, {"text-decoration", "underlined-double"});
    CHECK(style.underlined.value_or(false) == false);
    CHECK(style.underlined_double.has_value());
    CHECK(style.underlined_double.value() == true);
  }

  SECTION("double underline") {
    rtxui::ApplyStyle(style, {"text-decoration", "double underline"});
    CHECK(style.underlined.value_or(false) == false);
    CHECK(style.underlined_double.has_value());
    CHECK(style.underlined_double.value() == true);
  }

  SECTION("line-through / strikethrough") {
    rtxui::ApplyStyle(style, {"text-decoration", "line-through"});
    CHECK(style.strikethrough.has_value());
    CHECK(style.strikethrough.value() == true);

    rtxui::ApplyStyle(style, {"text-decoration", "strikethrough"});
    CHECK(style.strikethrough.has_value());
    CHECK(style.strikethrough.value() == true);
  }

  SECTION("blink") {
    rtxui::ApplyStyle(style, {"text-decoration", "blink"});
    CHECK(style.blink.has_value());
    CHECK(style.blink.value() == true);
  }

  SECTION("overline") {
    rtxui::ApplyStyle(style, {"text-decoration", "overline"});
    CHECK(style.overlined.has_value());
    CHECK(style.overlined.value() == true);
    // "overline" and "underline" are not substrings of one another, so the
    // one must not switch the other on.
    CHECK(style.underlined.value_or(false) == false);
    CHECK(style.underlined_double.value_or(false) == false);
  }

  SECTION("overline combines with other decorations") {
    rtxui::ApplyStyle(style, {"text-decoration", "overline underline"});
    CHECK(style.overlined.value_or(false) == true);
    CHECK(style.underlined.value_or(false) == true);
  }

  SECTION("none") {
    rtxui::ApplyStyle(style, {"text-decoration", "none"});
    CHECK(style.underlined.value_or(true) == false);
    CHECK(style.underlined_double.value_or(true) == false);
    CHECK(style.strikethrough.value_or(true) == false);
    CHECK(style.overlined.value_or(true) == false);
    CHECK(style.blink.value_or(true) == false);
  }
}

TEST_CASE("Letter spacing parsing in ApplyStyle", "[style][letter-spacing]") {
  auto spacing_of = [](std::string_view css_value) {
    rtxui::ComputedStyle style;
    rtxui::ApplyStyle(style, {"letter-spacing", css_value});
    return style.letter_spacing;
  };

  SECTION("integer cells") {
    REQUIRE(spacing_of("2").has_value());
    CHECK(spacing_of("2").value() == 2);
  }

  SECTION("normal is zero") {
    REQUIRE(spacing_of("normal").has_value());
    CHECK(spacing_of("normal").value() == 0);
  }

  SECTION("negative values clamp to zero") {
    REQUIRE(spacing_of("-3").has_value());
    CHECK(spacing_of("-3").value() == 0);
  }

  SECTION("unset by default") {
    rtxui::ComputedStyle style;
    CHECK(!style.letter_spacing.has_value());
  }
}

TEST_CASE("Text transform parsing in ApplyStyle", "[style][text-transform]") {
  rtxui::ComputedStyle style;

  SECTION("uppercase") {
    rtxui::ApplyStyle(style, {"text-transform", "uppercase"});
    REQUIRE(style.text_transform.has_value());
    CHECK(style.text_transform.value() == rtxui::TextTransform::Uppercase);
  }

  SECTION("lowercase") {
    rtxui::ApplyStyle(style, {"text-transform", "lowercase"});
    REQUIRE(style.text_transform.has_value());
    CHECK(style.text_transform.value() == rtxui::TextTransform::Lowercase);
  }

  SECTION("capitalize") {
    rtxui::ApplyStyle(style, {"text-transform", "capitalize"});
    REQUIRE(style.text_transform.has_value());
    CHECK(style.text_transform.value() == rtxui::TextTransform::Capitalize);
  }

  SECTION("none") {
    rtxui::ApplyStyle(style, {"text-transform", "none"});
    REQUIRE(style.text_transform.has_value());
    CHECK(style.text_transform.value() == rtxui::TextTransform::None);
  }

  SECTION("unset by default") {
    CHECK_FALSE(style.text_transform.has_value());
  }
}

TEST_CASE("CSS aspect-ratio parsing", "[style][aspect-ratio]") {
  rtxui::ComputedStyle style;

  SECTION("W / H with spaces") {
    rtxui::ApplyStyle(style, {"aspect-ratio", "4 / 2"});
    CHECK(style.aspect_ratio == 2.0f);
  }

  SECTION("W/H without spaces") {
    rtxui::ApplyStyle(style, {"aspect-ratio", "16/8"});
    CHECK(style.aspect_ratio == 2.0f);
  }

  SECTION("single number") {
    rtxui::ApplyStyle(style, {"aspect-ratio", "3"});
    CHECK(style.aspect_ratio == 3.0f);
  }

  SECTION("auto") {
    rtxui::ApplyStyle(style, {"aspect-ratio", "2"});
    rtxui::ApplyStyle(style, {"aspect-ratio", "auto"});
    CHECK(style.aspect_ratio == 0.0f);
  }

  SECTION("invalid") {
    rtxui::ApplyStyle(style, {"aspect-ratio", "4 / 0"});
    CHECK(style.aspect_ratio == 0.0f);
  }
}

TEST_CASE("CSS inset shorthand", "[style][inset]") {
  rtxui::ComputedStyle style;

  SECTION("one value") {
    rtxui::ApplyStyle(style, {"inset", "3"});
    CHECK(style.top == rtxui::Length::Cells(3));
    CHECK(style.right == rtxui::Length::Cells(3));
    CHECK(style.bottom == rtxui::Length::Cells(3));
    CHECK(style.left == rtxui::Length::Cells(3));
  }

  SECTION("two values") {
    rtxui::ApplyStyle(style, {"inset", "1 2"});
    CHECK(style.top == rtxui::Length::Cells(1));
    CHECK(style.bottom == rtxui::Length::Cells(1));
    CHECK(style.left == rtxui::Length::Cells(2));
    CHECK(style.right == rtxui::Length::Cells(2));
  }

  SECTION("four values") {
    rtxui::ApplyStyle(style, {"inset", "1 2 3 4"});
    CHECK(style.top == rtxui::Length::Cells(1));
    CHECK(style.right == rtxui::Length::Cells(2));
    CHECK(style.bottom == rtxui::Length::Cells(3));
    CHECK(style.left == rtxui::Length::Cells(4));
  }

  SECTION("calc value") {
    rtxui::ApplyStyle(style, {"inset", "calc(50% - 1)"});
    CHECK(style.top.Resolve(10) == 4);
    CHECK(style.left.Resolve(10) == 4);
  }
}

TEST_CASE("CSS nesting is bounded", "[style][limits]") {
  // ParseRuleset recurses once per level of nesting, so a deeply nested
  // stylesheet used to overflow the stack instead of reporting an error.
  const auto nested = [](int depth) {
    std::string css;
    for (int i = 0; i < depth; ++i) {
      css += "div { ";
    }
    css += "color: red;";
    for (int i = 0; i < depth; ++i) {
      css += "}";
    }
    return css;
  };

  SECTION("ordinary nesting still parses") {
    CHECK(css::Parse(nested(100)).has_value());
  }

  SECTION("excessive nesting is an error, not a crash") {
    CHECK_FALSE(css::Parse(nested(20000)).has_value());
  }
}

TEST_CASE("Extreme CSS numbers are bounded", "[style][limits]") {
  // A stylesheet is text, and hot reload and the playground both let one be
  // edited while the application is running. Unbounded, a single large number
  // was enough to take one out: layout's arithmetic overflowed (signed
  // overflow, so undefined behaviour rather than a wrong size) and
  // letter-spacing allocated until it was killed.
  auto width_of = [](std::string_view css_value) {
    rtxui::ComputedStyle style;
    rtxui::ApplyStyle(style, {"width", css_value});
    return style.width.Resolve(0);
  };

  SECTION("a huge value is clamped rather than kept") {
    const int clamped = width_of("2000000000");
    CHECK(clamped > 0);
    CHECK(clamped < 2000000000);
    // Still far larger than any terminal, so nothing real is affected.
    CHECK(clamped >= 10000);
  }

  SECTION("a value past INT_MAX is a large number, not zero") {
    // from_chars reports out_of_range without writing a value, so falling
    // through would silently read as 0 -- the opposite of what was written.
    CHECK(width_of("99999999999999999999") == width_of("2000000000"));
    CHECK(width_of("-99999999999999999999") == width_of("-2000000000"));
  }

  SECTION("multiplying two clamped operands still resolves") {
    // Each operand is bounded when parsed; their product is not, and the
    // float it produces does not fit in an int. Narrowing it is undefined
    // behaviour, and produced INT_MIN, which then poisoned every subtraction
    // downstream in layout.
    const int product = width_of("calc(2000000000 * 2000000000)");
    CHECK(product > 0);
    CHECK(product <= 1000000);
  }

  SECTION("negative extremes clamp too") {
    const int clamped = width_of("-2000000000");
    CHECK(clamped < 0);
    CHECK(clamped > -2000000000);
  }

  SECTION("letter-spacing is bounded more tightly than the rest") {
    // It is materialised as blank cells between every grapheme, so the work is
    // spacing x length. The general bound alone still let a long string ask
    // for gigabytes.
    rtxui::ComputedStyle style;
    rtxui::ApplyStyle(style, {"letter-spacing", "100000000"});
    REQUIRE(style.letter_spacing.has_value());
    CHECK(*style.letter_spacing <= 1000);
    CHECK(*style.letter_spacing > 0);
  }

  SECTION("ordinary values are untouched") {
    CHECK(width_of("40") == 40);
    CHECK(width_of("0") == 0);
    CHECK(width_of("-3") == -3);
    rtxui::ComputedStyle style;
    rtxui::ApplyStyle(style, {"letter-spacing", "2"});
    CHECK(*style.letter_spacing == 2);
  }
}

TEST_CASE("CSS calc() parsing and resolution", "[style][calc]") {
  auto width_of = [](std::string_view css_value) {
    rtxui::ComputedStyle style;
    rtxui::ApplyStyle(style, {"width", css_value});
    return style.width;
  };

  SECTION("percent minus cells") {
    rtxui::Length w = width_of("calc(100% - 4)");
    CHECK(w.unit == rtxui::Unit::Calc);
    CHECK(w.Resolve(80) == 76);
    CHECK(w.Resolve(10) == 6);
  }

  SECTION("percent plus cells") {
    CHECK(width_of("calc(50% + 2)").Resolve(20) == 12);
  }

  SECTION("pure cells folds to Cells") {
    rtxui::Length w = width_of("calc(2 * 10 + 5)");
    CHECK(w.unit == rtxui::Unit::Cells);
    CHECK(w.Resolve(0) == 25);
  }

  SECTION("pure percent folds to Percent") {
    rtxui::Length w = width_of("calc(100% / 4)");
    CHECK(w.unit == rtxui::Unit::Percent);
    CHECK(w.Resolve(80) == 20);
  }

  SECTION("parenthesized expression") {
    CHECK(width_of("calc((100% - 10) / 2)").Resolve(80) == 35);
  }

  SECTION("scalar times percent") {
    CHECK(width_of("calc(0.5 * 100%)").Resolve(50) == 25);
  }

  SECTION("nested calc") {
    CHECK(width_of("calc(100% - calc(2 + 3))").Resolve(100) == 95);
  }

  SECTION("invalid expressions resolve to auto") {
    CHECK(width_of("calc(10% * 20%)").unit == rtxui::Unit::Auto);
    CHECK(width_of("calc(10 / 0)").unit == rtxui::Unit::Auto);
    CHECK(width_of("calc(10 +)").unit == rtxui::Unit::Auto);
    CHECK(width_of("calc(abc)").unit == rtxui::Unit::Auto);
    CHECK(width_of("calc(10").unit == rtxui::Unit::Auto);
  }
}

TEST_CASE("CSS min()/max()/clamp() lengths", "[style][calc][minmax]") {
  auto width_of = [](std::string_view css_value) {
    rtxui::ComputedStyle style;
    rtxui::ApplyStyle(style, {"width", css_value});
    return style.width;
  };

  SECTION("min caps a percentage") {
    rtxui::Length w = width_of("min(100%, 6)");
    CHECK(w.unit == rtxui::Unit::MinMax);
    CHECK(w.Resolve(10) == 6);
    CHECK(w.Resolve(4) == 4);
  }

  SECTION("max enforces a floor") {
    rtxui::Length w = width_of("max(50%, 8)");
    CHECK(w.Resolve(10) == 8);
    CHECK(w.Resolve(30) == 15);
  }

  SECTION("clamp") {
    rtxui::Length w = width_of("clamp(4, 50%, 12)");
    CHECK(w.Resolve(4) == 4);    // preferred below the minimum
    CHECK(w.Resolve(16) == 8);   // preferred inside the range
    CHECK(w.Resolve(40) == 12);  // preferred above the maximum
  }

  SECTION("constant expressions fold to Cells") {
    rtxui::Length w = width_of("min(3, 7)");
    CHECK(w.unit == rtxui::Unit::Cells);
    CHECK(w.Resolve(0) == 3);
    CHECK(width_of("clamp(2, 10, 6)").Resolve(0) == 6);
  }

  SECTION("calc() inside an argument") {
    CHECK(width_of("min(calc(100% - 2), 20)").Resolve(10) == 8);
  }

  SECTION("identical expressions share an interned id") {
    rtxui::Length a = width_of("min(100%, 17)");
    rtxui::Length b = width_of("min(100%, 17)");
    CHECK(a == b);
  }

  SECTION("invalid expressions resolve to auto") {
    CHECK(width_of("min(1)").unit == rtxui::Unit::Auto);
    CHECK(width_of("clamp(1, 2)").unit == rtxui::Unit::Auto);
    CHECK(width_of("min(abc, 2)").unit == rtxui::Unit::Auto);
    CHECK(width_of("min(1, 2").unit == rtxui::Unit::Auto);
  }

  SECTION("min() nested inside max()") {
    rtxui::Length w = width_of("max(min(100%, 6), 4)");
    CHECK(w.Resolve(10) == 6);
    CHECK(w.Resolve(5) == 5);
    CHECK(w.Resolve(2) == 4);
  }

  SECTION("min() nested inside calc()") {
    rtxui::Length w = width_of("calc(min(100%, 6) + 2)");
    CHECK(w.unit == rtxui::Unit::MinMax);
    CHECK(w.Resolve(10) == 8);
    CHECK(w.Resolve(3) == 5);
  }

  SECTION("nested min() scaled and divided inside calc()") {
    CHECK(width_of("calc(2 * min(50%, 10))").Resolve(10) == 10);
    CHECK(width_of("calc(2 * min(50%, 10))").Resolve(40) == 20);
    CHECK(width_of("calc(min(100%, 8) / 2)").Resolve(10) == 4);
    CHECK(width_of("calc(100% - min(50%, 4))").Resolve(20) == 16);
  }

  SECTION("constant nested expressions fold to Cells") {
    rtxui::Length w = width_of("calc(min(3, 7) + 2)");
    CHECK(w.unit == rtxui::Unit::Cells);
    CHECK(w.Resolve(0) == 5);
    CHECK(width_of("max(min(1, 2), 3)").unit == rtxui::Unit::Cells);
  }

  SECTION("clamp() with nested basis-dependent bounds") {
    rtxui::Length w = width_of("clamp(4, 50%, min(100% - 2, 12))");
    CHECK(w.Resolve(4) == 4);    // preferred below the minimum
    CHECK(w.Resolve(16) == 8);   // preferred inside the range
    CHECK(w.Resolve(40) == 12);  // hi bound capped at 12
    CHECK(w.Resolve(22) == 11);  // hi bound is 100% - 2 = 20, preferred wins
  }

  SECTION("unsupported nested combinations resolve to auto") {
    // At most one basis-dependent min/max term per linear expression.
    CHECK(width_of("calc(min(100%, 5) + min(100%, 6))").unit ==
          rtxui::Unit::Auto);
    // A min/max term cannot be multiplied by a percentage.
    CHECK(width_of("calc(min(100%, 5) * 50%)").unit == rtxui::Unit::Auto);
  }
}

TEST_CASE("CSS !important parsing", "[css][important]") {
  auto stylesheet = css::Parse(R"(
    div {
      color: red !important;
      background-color: blue;
    }
  )");
  REQUIRE(stylesheet.has_value());
  REQUIRE(stylesheet.value().size() == 1);
  const auto& decls = stylesheet.value()[0].declarations;
  REQUIRE(decls.size() == 2);
  CHECK(decls[0].property == "color");
  CHECK(decls[0].value == "red");
  CHECK(decls[0].important == true);
  CHECK(decls[1].value == "blue");
  CHECK(decls[1].important == false);

  CHECK(css::Print(stylesheet.value()).find("color: red !important;") !=
        std::string::npos);
}

TEST_CASE("CSS var() substitution", "[css][var]") {
  css::CustomProperties props;
  props["--a"] = "red";
  props["--b"] = "var(--a)";

  CHECK(css::SubstituteVars("var(--a)", props).value() == "red");
  CHECK(css::SubstituteVars("1 var(--a) solid", props).value() ==
        "1 red solid");
  CHECK(css::SubstituteVars("var( --a )", props).value() == "red");
  CHECK(css::SubstituteVars("var(--missing, blue)", props).value() == "blue");
  CHECK(css::SubstituteVars("var(--missing, var(--a))", props).value() ==
        "red");
  CHECK(css::SubstituteVars("var(--b)", props).value() == "red");
  CHECK(css::SubstituteVars("plain", props).value() == "plain");

  CHECK_FALSE(css::SubstituteVars("var(--missing)", props).has_value());
  CHECK_FALSE(css::SubstituteVars("var(--a", props).has_value());

  css::CustomProperties cyclic;
  cyclic["--x"] = "var(--y)";
  cyclic["--y"] = "var(--x)";
  CHECK_FALSE(css::SubstituteVars("var(--x)", cyclic).has_value());
}

TEST_CASE("Font weight parsing in ApplyStyle", "[style][font-weight]") {
  rtxui::ComputedStyle style;

  SECTION("bold / bolder") {
    rtxui::ApplyStyle(style, {"font-weight", "bold"});
    CHECK(style.bold.value_or(false) == true);
    CHECK(style.dim.value_or(true) == false);

    rtxui::ApplyStyle(style, {"font-weight", "bolder"});
    CHECK(style.bold.value_or(false) == true);
  }

  SECTION("lighter renders dim") {
    rtxui::ApplyStyle(style, {"font-weight", "lighter"});
    CHECK(style.bold.value_or(true) == false);
    CHECK(style.dim.value_or(false) == true);
  }

  SECTION("numeric weights") {
    rtxui::ApplyStyle(style, {"font-weight", "700"});
    CHECK(style.bold.value_or(false) == true);
    CHECK(style.dim.value_or(true) == false);

    rtxui::ApplyStyle(style, {"font-weight", "200"});
    CHECK(style.bold.value_or(true) == false);
    CHECK(style.dim.value_or(false) == true);

    rtxui::ApplyStyle(style, {"font-weight", "400"});
    CHECK(style.bold.value_or(true) == false);
    CHECK(style.dim.value_or(true) == false);
  }

  SECTION("normal") {
    rtxui::ApplyStyle(style, {"font-weight", "normal"});
    CHECK(style.bold.value_or(true) == false);
    CHECK(style.dim.value_or(true) == false);
  }
}

TEST_CASE("Font style parsing in ApplyStyle", "[style][font-style]") {
  rtxui::ComputedStyle style;

  SECTION("italic") {
    rtxui::ApplyStyle(style, {"font-style", "italic"});
    CHECK(style.italic.has_value());
    CHECK(style.italic.value() == true);
  }

  SECTION("oblique") {
    rtxui::ApplyStyle(style, {"font-style", "oblique"});
    CHECK(style.italic.has_value());
    CHECK(style.italic.value() == true);
  }

  SECTION("normal") {
    rtxui::ApplyStyle(style, {"font-style", "normal"});
    CHECK(style.italic.has_value());
    CHECK(style.italic.value() == false);
  }

  SECTION("unset by default") {
    CHECK_FALSE(style.italic.has_value());
  }
}

TEST_CASE("Max-width, max-height and margin auto parsing in ApplyStyle",
          "[style][max-width][max-height][margin]") {
  rtxui::ComputedStyle style;

  SECTION("max-width") {
    rtxui::ApplyStyle(style, {"max-width", "50"});
    CHECK(style.max_width.unit == rtxui::Unit::Cells);
    CHECK(style.max_width.value == 50);

    rtxui::ApplyStyle(style, {"max-width", "80%"});
    CHECK(style.max_width.unit == rtxui::Unit::Percent);
    CHECK(style.max_width.value == 80);
  }

  SECTION("max-height") {
    rtxui::ApplyStyle(style, {"max-height", "40"});
    CHECK(style.max_height.unit == rtxui::Unit::Cells);
    CHECK(style.max_height.value == 40);

    rtxui::ApplyStyle(style, {"max-height", "60%"});
    CHECK(style.max_height.unit == rtxui::Unit::Percent);
    CHECK(style.max_height.value == 60);
  }

  SECTION("margin: auto shorthand") {
    rtxui::ApplyStyle(style, {"margin", "auto"});
    CHECK(style.margin_left_auto);
    CHECK(style.margin_right_auto);
    CHECK(style.margin.left == 0);
    CHECK(style.margin.right == 0);
  }

  SECTION("margin: 2 auto shorthand") {
    rtxui::ApplyStyle(style, {"margin", "2 auto"});
    CHECK(style.margin.top == 2);
    CHECK(style.margin.bottom == 2);
    CHECK(style.margin_left_auto);
    CHECK(style.margin_right_auto);
    CHECK(style.margin.left == 0);
    CHECK(style.margin.right == 0);
  }

  SECTION("margin-left: auto and margin-right: auto sub-properties") {
    rtxui::ApplyStyle(style, {"margin-left", "auto"});
    CHECK(style.margin_left_auto);
    CHECK(style.margin.left == 0);

    rtxui::ApplyStyle(style, {"margin-right", "auto"});
    CHECK(style.margin_right_auto);
    CHECK(style.margin.right == 0);
  }

  SECTION("opacity property") {
    rtxui::ApplyStyle(style, {"opacity", "0.5"});
    CHECK(style.opacity == 0.5f);

    rtxui::ApplyStyle(style, {"opacity", "1.5"});
    CHECK(style.opacity == 1.0f);

    rtxui::ApplyStyle(style, {"opacity", "-0.5"});
    CHECK(style.opacity == 0.0f);
  }

  SECTION("padding and border-width shorthand parsing") {
    rtxui::ApplyStyle(style, {"padding", "2"});
    CHECK(style.padding.top == 2);
    CHECK(style.padding.right == 2);
    CHECK(style.padding.bottom == 2);
    CHECK(style.padding.left == 2);

    rtxui::ApplyStyle(style, {"padding", "1 3"});
    CHECK(style.padding.top == 1);
    CHECK(style.padding.bottom == 1);
    CHECK(style.padding.right == 3);
    CHECK(style.padding.left == 3);

    rtxui::ApplyStyle(style, {"padding", "1 2 3 4"});
    CHECK(style.padding.top == 1);
    CHECK(style.padding.right == 2);
    CHECK(style.padding.bottom == 3);
    CHECK(style.padding.left == 4);

    rtxui::ApplyStyle(style, {"border-width", "5 6"});
    CHECK(style.border.top == 5);
    CHECK(style.border.bottom == 5);
    CHECK(style.border.right == 6);
    CHECK(style.border.left == 6);
  }

  SECTION("flex-grow and flex-shrink properties") {
    rtxui::ApplyStyle(style, {"flex-grow", "2.5"});
    CHECK(style.flex_grow == 2.5f);

    rtxui::ApplyStyle(style, {"flex-shrink", "0.0"});
    CHECK(style.flex_shrink == 0.0f);
  }
}

TEST_CASE("Color transformations in ApplyStyle", "[style][color]") {
  rtxui::ComputedStyle style;

  SECTION("lighten() color transformation") {
    // Without current background color (fallback to white overlay)
    rtxui::ApplyStyle(style, {"background-color", "lighten(10%)"});
    REQUIRE(style.background_color.has_value());
    CHECK(style.background_color->r == 255);
    CHECK(style.background_color->g == 255);
    CHECK(style.background_color->b == 255);
    CHECK(style.background_color->a == 25);

    // With current background color
    style.background_color = Color::RGB(100, 150, 200);
    rtxui::ApplyStyle(style, {"background-color", "lighten(0.1)"});
    REQUIRE(style.background_color.has_value());
    // A tenth of the way to white, per channel: 100 + 0.1 * (255 - 100).
    // Not a flat +25.5 on every channel, which moved the already-bright blue
    // as far as the dark red and clipped anything above 230.
    CHECK(style.background_color->r == 115);
    CHECK(style.background_color->g == 160);
    CHECK(style.background_color->b == 205);
  }

  SECTION("darken() color transformation") {
    // Without current color (fallback to black overlay)
    rtxui::ApplyStyle(style, {"color", "darken(20%)"});
    REQUIRE(style.foreground_color.has_value());
    CHECK(style.foreground_color->r == 0);
    CHECK(style.foreground_color->g == 0);
    CHECK(style.foreground_color->b == 0);
    CHECK(style.foreground_color->a == 51);

    // With current color
    style.foreground_color = Color::RGB(100, 150, 200);
    rtxui::ApplyStyle(style, {"color", "darken(0.2)"});
    REQUIRE(style.foreground_color.has_value());
    // A fifth of the way to black: 100 - 0.2 * 100.
    CHECK(style.foreground_color->r == 80);
    CHECK(style.foreground_color->g == 120);
    CHECK(style.foreground_color->b == 160);
  }

  SECTION("alpha() color transformation") {
    style.background_color = Color::RGB(100, 150, 200);
    rtxui::ApplyStyle(style, {"background-color", "alpha(50%)"});
    REQUIRE(style.background_color.has_value());
    CHECK(style.background_color->r == 100);
    CHECK(style.background_color->g == 150);
    CHECK(style.background_color->b == 200);
    CHECK(style.background_color->a == 127);
  }
}

TEST_CASE("Length auto parsing in ApplyStyle", "[style][length][auto]") {
  rtxui::ComputedStyle style;

  rtxui::ApplyStyle(style, {"width", "auto"});
  CHECK(style.width.unit == rtxui::Unit::Auto);

  rtxui::ApplyStyle(style, {"height", "auto"});
  CHECK(style.height.unit == rtxui::Unit::Auto);
}

TEST_CASE("Gap parsing in ApplyStyle", "[style][gap]") {
  rtxui::ComputedStyle style;

  SECTION("row-gap only") {
    rtxui::ApplyStyle(style, {"row-gap", "5"});
    CHECK(style.row_gap == rtxui::Length::Cells(5.0f));
    CHECK(style.column_gap == rtxui::Length::Cells(0.0f));
  }

  SECTION("column-gap only") {
    rtxui::ApplyStyle(style, {"column-gap", "8%"});
    CHECK(style.row_gap == rtxui::Length::Cells(0.0f));
    CHECK(style.column_gap == rtxui::Length::Pct(8.0f));
  }

  SECTION("gap shorthand single value") {
    rtxui::ApplyStyle(style, {"gap", "4"});
    CHECK(style.row_gap == rtxui::Length::Cells(4.0f));
    CHECK(style.column_gap == rtxui::Length::Cells(4.0f));
  }

  SECTION("gap shorthand two values") {
    rtxui::ApplyStyle(style, {"gap", "3 6"});
    CHECK(style.row_gap == rtxui::Length::Cells(3.0f));
    CHECK(style.column_gap == rtxui::Length::Cells(6.0f));
  }
}

TEST_CASE("Flex basis and shorthand parsing in ApplyStyle", "[style][flex]") {
  rtxui::ComputedStyle style;

  SECTION("flex-basis only") {
    rtxui::ApplyStyle(style, {"flex-basis", "15"});
    CHECK(style.flex_basis == rtxui::Length::Cells(15.0f));

    rtxui::ApplyStyle(style, {"flex-basis", "auto"});
    CHECK(style.flex_basis.unit == rtxui::Unit::Auto);
  }

  SECTION("flex shorthand grow only") {
    rtxui::ApplyStyle(style, {"flex", "2"});
    CHECK(style.flex_grow == 2.0f);
    CHECK(style.flex_shrink == 1.0f);
    CHECK(style.flex_basis == rtxui::Length::Cells(0.0f));
  }

  SECTION("flex shorthand grow and shrink") {
    rtxui::ApplyStyle(style, {"flex", "2 3"});
    CHECK(style.flex_grow == 2.0f);
    CHECK(style.flex_shrink == 3.0f);
    CHECK(style.flex_basis == rtxui::Length::Cells(0.0f));
  }

  SECTION("flex shorthand grow and basis") {
    rtxui::ApplyStyle(style, {"flex", "2 auto"});
    CHECK(style.flex_grow == 2.0f);
    CHECK(style.flex_shrink == 1.0f);
    CHECK(style.flex_basis.unit == rtxui::Unit::Auto);
  }

  SECTION("flex shorthand grow, shrink and basis") {
    rtxui::ApplyStyle(style, {"flex", "3 4 50%"});
    CHECK(style.flex_grow == 3.0f);
    CHECK(style.flex_shrink == 4.0f);
    CHECK(style.flex_basis == rtxui::Length::Pct(50.0f));
  }

  SECTION("flex shorthand none") {
    rtxui::ApplyStyle(style, {"flex", "none"});
    CHECK(style.flex_grow == 0.0f);
    CHECK(style.flex_shrink == 0.0f);
    CHECK(style.flex_basis.unit == rtxui::Unit::Auto);
  }

  SECTION("flex shorthand auto") {
    rtxui::ApplyStyle(style, {"flex", "auto"});
    CHECK(style.flex_grow == 1.0f);
    CHECK(style.flex_shrink == 1.0f);
    CHECK(style.flex_basis.unit == rtxui::Unit::Auto);
  }
}

TEST_CASE("Align self and content parsing in ApplyStyle", "[style][align]") {
  rtxui::ComputedStyle style;

  SECTION("align-self") {
    rtxui::ApplyStyle(style, {"align-self", "flex-end"});
    CHECK(style.align_self == rtxui::AlignSelf::FlexEnd);

    rtxui::ApplyStyle(style, {"align-self", "center"});
    CHECK(style.align_self == rtxui::AlignSelf::Center);

    rtxui::ApplyStyle(style, {"align-self", "auto"});
    CHECK(style.align_self == rtxui::AlignSelf::Auto);
  }

  SECTION("align-content") {
    rtxui::ApplyStyle(style, {"align-content", "space-around"});
    CHECK(style.align_content == rtxui::AlignContent::SpaceAround);

    rtxui::ApplyStyle(style, {"align-content", "center"});
    CHECK(style.align_content == rtxui::AlignContent::Center);
  }
}

TEST_CASE("Grid templates and placement parsing in ApplyStyle", "[style][grid]") {
  rtxui::ComputedStyle style;

  SECTION("grid-template-columns only") {
    rtxui::ApplyStyle(style, {"grid-template-columns", "1fr 2fr 50px"});
    REQUIRE(style.grid_template_columns.size() == 3);
    CHECK(style.grid_template_columns[0] == rtxui::Length::Fr(1.0f));
    CHECK(style.grid_template_columns[1] == rtxui::Length::Fr(2.0f));
    CHECK(style.grid_template_columns[2] == rtxui::Length::Cells(50.0f));
  }

  SECTION("grid-template-rows only") {
    rtxui::ApplyStyle(style, {"grid-template-rows", "auto 10%"});
    REQUIRE(style.grid_template_rows.size() == 2);
    CHECK(style.grid_template_rows[0].unit == rtxui::Unit::Auto);
    CHECK(style.grid_template_rows[1] == rtxui::Length::Pct(10.0f));
  }

  SECTION("grid-template shorthand") {
    rtxui::ApplyStyle(style, {"grid-template", "1fr 2fr / 100px 50px"});
    REQUIRE(style.grid_template_rows.size() == 2);
    CHECK(style.grid_template_rows[0] == rtxui::Length::Fr(1.0f));
    CHECK(style.grid_template_rows[1] == rtxui::Length::Fr(2.0f));

    REQUIRE(style.grid_template_columns.size() == 2);
    CHECK(style.grid_template_columns[0] == rtxui::Length::Cells(100.0f));
    CHECK(style.grid_template_columns[1] == rtxui::Length::Cells(50.0f));
  }

  SECTION("grid-template shorthand none") {
    style.grid_template_rows = {rtxui::Length::Fr(1.0f)};
    style.grid_template_columns = {rtxui::Length::Fr(2.0f)};
    rtxui::ApplyStyle(style, {"grid-template", "none"});
    CHECK(style.grid_template_rows.empty());
    CHECK(style.grid_template_columns.empty());
  }

  SECTION("grid-column and grid-column-end") {
    rtxui::ApplyStyle(style, {"grid-column", "span 3"});
    CHECK(style.grid_column_span == 3);

    rtxui::ApplyStyle(style, {"grid-column-end", "5"});
    CHECK(style.grid_column_span == 5);
  }

  SECTION("grid-row and grid-row-end") {
    rtxui::ApplyStyle(style, {"grid-row", "span 2"});
    CHECK(style.grid_row_span == 2);

    rtxui::ApplyStyle(style, {"grid-row-end", "4"});
    CHECK(style.grid_row_span == 4);
  }

  SECTION("grid-template with repeat()") {
    rtxui::ApplyStyle(style, {"grid-template-columns", "1fr repeat(3, 100px) 2fr"});
    REQUIRE(style.grid_template_columns.size() == 5);
    CHECK(style.grid_template_columns[0] == rtxui::Length::Fr(1.0f));
    CHECK(style.grid_template_columns[1] == rtxui::Length::Cells(100.0f));
    CHECK(style.grid_template_columns[2] == rtxui::Length::Cells(100.0f));
    CHECK(style.grid_template_columns[3] == rtxui::Length::Cells(100.0f));
    CHECK(style.grid_template_columns[4] == rtxui::Length::Fr(2.0f));
  }

  SECTION("grid-template with nested repeat() and spaces") {
    rtxui::ApplyStyle(style, {"grid-template-rows", "repeat(2, 1fr 2fr)"});
    REQUIRE(style.grid_template_rows.size() == 4);
    CHECK(style.grid_template_rows[0] == rtxui::Length::Fr(1.0f));
    CHECK(style.grid_template_rows[1] == rtxui::Length::Fr(2.0f));
    CHECK(style.grid_template_rows[2] == rtxui::Length::Fr(1.0f));
    CHECK(style.grid_template_rows[3] == rtxui::Length::Fr(2.0f));
  }

  SECTION("grid gap aliases") {
    rtxui::ApplyStyle(style, {"grid-gap", "5px 10px"});
    CHECK(style.row_gap == rtxui::Length::Cells(5.0f));
    CHECK(style.column_gap == rtxui::Length::Cells(10.0f));

    rtxui::ApplyStyle(style, {"grid-row-gap", "8%"});
    CHECK(style.row_gap == rtxui::Length::Pct(8.0f));

    rtxui::ApplyStyle(style, {"grid-column-gap", "2fr"});
    CHECK(style.column_gap == rtxui::Length::Fr(2.0f));
  }
}

TEST_CASE("Opacity parsing in ApplyStyle", "[style][opacity]") {
  rtxui::ComputedStyle style;

  SECTION("Standard opacity values") {
    rtxui::ApplyStyle(style, {"opacity", "0.7"});
    CHECK(style.opacity == 0.7f);

    rtxui::ApplyStyle(style, {"opacity", "0.0"});
    CHECK(style.opacity == 0.0f);

    rtxui::ApplyStyle(style, {"opacity", "1.0"});
    CHECK(style.opacity == 1.0f);

    rtxui::ApplyStyle(style, {"opacity", "1"});
    CHECK(style.opacity == 1.0f);

    rtxui::ApplyStyle(style, {"opacity", "0"});
    CHECK(style.opacity == 0.0f);
  }

  SECTION("Clamping out of bounds values") {
    rtxui::ApplyStyle(style, {"opacity", "-0.5"});
    CHECK(style.opacity == 0.0f);

    rtxui::ApplyStyle(style, {"opacity", "1.5"});
    CHECK(style.opacity == 1.0f);
  }

  SECTION("Whitespace and parsing format") {
    rtxui::ApplyStyle(style, {"opacity", "  0.35  "});
    CHECK(style.opacity == 0.35f);

    rtxui::ApplyStyle(style, {"opacity", ".5"});
    CHECK(style.opacity == 0.5f);
  }
}

TEST_CASE("CSS style parsing handles invalid values robustly", "[style][robustness]") {
  rtxui::ComputedStyle style;

  SECTION("Invalid color values") {
    style.foreground_color = std::nullopt;
    rtxui::ApplyStyle(style, {"color", "rgb(256, 12)"});
    CHECK(!style.foreground_color.has_value());

    rtxui::ApplyStyle(style, {"color", "rgb(abc, 12, 34)"});
    CHECK(!style.foreground_color.has_value());

    rtxui::ApplyStyle(style, {"background-color", "rgba(1, 2, abc, 0.5)"});
    CHECK(!style.background_color.has_value());

    rtxui::ApplyStyle(style, {"background-color", "rgba(1, 2, 3, xyz)"});
    CHECK(!style.background_color.has_value());
  }

  SECTION("Invalid grid properties") {
    style.grid_column_span = 1;
    rtxui::ApplyStyle(style, {"grid-column", "span abc"});
    CHECK(style.grid_column_span == 1);

    rtxui::ApplyStyle(style, {"grid-column", "xyz"});
    CHECK(style.grid_column_span == 1);

    style.grid_row_span = 1;
    rtxui::ApplyStyle(style, {"grid-row", "span "});
    CHECK(style.grid_row_span == 1);

    style.grid_template_columns.clear();
    rtxui::ApplyStyle(style, {"grid-template-columns", "repeat(abc, 10px)"});
    CHECK(style.grid_template_columns.empty());
  }

  SECTION("Invalid border width") {
    style.border = {0, 0, 0, 0};
    style.border_style = rtxui::BorderStyle::None;
    rtxui::ApplyStyle(style, {"border", "abc"});
    CHECK(style.border.top == 0);
    CHECK(style.border_style == rtxui::BorderStyle::None);
  }
}

TEST_CASE("border-<side> accepts a style keyword as well as a thickness",
          "[style][border]") {
  // Regression: only the `border` shorthand parsed a style keyword, so
  // `border-bottom: solid` fell through to the integer parse, yielded 0 and
  // silently drew nothing.
  rtxui::ComputedStyle style;

  SECTION("A keyword sets the style and a one-cell thickness") {
    rtxui::ApplyStyle(style, {"border-bottom", "solid"});
    CHECK(style.border.bottom == 1);
    CHECK(style.border_style == rtxui::BorderStyle::Solid);
    // Only the named side is affected.
    CHECK(style.border.top == 0);
    CHECK(style.border.left == 0);
    CHECK(style.border.right == 0);
  }

  SECTION("Each side is settable by keyword") {
    rtxui::ApplyStyle(style, {"border-top", "dashed"});
    CHECK(style.border.top == 1);
    rtxui::ApplyStyle(style, {"border-left", "solid"});
    CHECK(style.border.left == 1);
    rtxui::ApplyStyle(style, {"border-right", "solid"});
    CHECK(style.border.right == 1);
  }

  SECTION("An explicit thickness still wins") {
    rtxui::ApplyStyle(style, {"border-top", "3"});
    CHECK(style.border.top == 3);
  }

  SECTION("none clears the side") {
    rtxui::ApplyStyle(style, {"border-bottom", "solid"});
    REQUIRE(style.border.bottom == 1);
    rtxui::ApplyStyle(style, {"border-bottom", "none"});
    CHECK(style.border.bottom == 0);
  }
}

TEST_CASE("CSS ::part() selector parsing", "[css][part]") {
  SECTION("tag base") {
    auto stylesheet = css::Parse("textarea::part(gutter) { color: red; }");
    REQUIRE(stylesheet.has_value());
    REQUIRE(stylesheet.value().size() == 1);
    const auto& sel = stylesheet.value()[0].parsed_selector;
    CHECK(sel.base == "textarea");
    CHECK(sel.part == "gutter");
    CHECK(sel.pseudo_classes.empty());
  }

  SECTION("class base") {
    auto stylesheet = css::Parse(".editor::part(gutter) { color: red; }");
    REQUIRE(stylesheet.has_value());
    const auto& sel = stylesheet.value()[0].parsed_selector;
    CHECK(sel.base.empty());
    REQUIRE(sel.classes.size() == 1);
    CHECK(sel.classes[0] == "editor");
    CHECK(sel.part == "gutter");
  }

  SECTION("no base") {
    auto stylesheet = css::Parse("::part(active) { color: red; }");
    REQUIRE(stylesheet.has_value());
    CHECK(stylesheet.value()[0].parsed_selector.part == "active");
  }

  SECTION("plain pseudo-class selectors are unaffected") {
    auto stylesheet = css::Parse("self:hover { color: red; }");
    REQUIRE(stylesheet.has_value());
    const auto& sel = stylesheet.value()[0].parsed_selector;
    CHECK(sel.part.empty());
    REQUIRE(sel.pseudo_classes.size() == 1);
    CHECK(sel.pseudo_classes[0] == "hover");
  }
}



TEST_CASE("place-content sets both content-distribution axes",
          "[style][flex][grid]") {
  // place-items and place-self existed, and both of place-content's longhands
  // did, but the shorthand itself fell through to the unknown-property path
  // and silently did nothing.
  rtxui::ComputedStyle style;

  SECTION("One value sets both axes") {
    rtxui::ApplyStyle(style, {"place-content", "center"});
    CHECK(style.align_content == rtxui::AlignContent::Center);
    CHECK(style.justify_content == rtxui::JustifyContent::Center);
  }

  SECTION("Two values are block axis then inline axis") {
    rtxui::ApplyStyle(style, {"place-content", "flex-end space-between"});
    CHECK(style.align_content == rtxui::AlignContent::FlexEnd);
    CHECK(style.justify_content == rtxui::JustifyContent::SpaceBetween);
  }

  SECTION("A value valid on only one axis leaves the other alone") {
    // `stretch` is an align-content value with no justify-content counterpart.
    rtxui::ApplyStyle(style, {"justify-content", "center"});
    rtxui::ApplyStyle(style, {"place-content", "stretch"});
    CHECK(style.align_content == rtxui::AlignContent::Stretch);
    CHECK(style.justify_content == rtxui::JustifyContent::Center);
  }

  SECTION("An unknown keyword changes nothing") {
    rtxui::ApplyStyle(style, {"place-content", "nonsense"});
    CHECK(style.align_content == rtxui::AlignContent::Stretch);
    CHECK(style.justify_content == rtxui::JustifyContent::FlexStart);
  }
}

TEST_CASE("justify-content and align-content accept start/end",
          "[style][flex]") {
  // align-items/align-self took the CSS box-alignment spellings `start`/`end`
  // but the content-distribution properties only took `flex-start`/`flex-end`,
  // so `justify-content: start` parsed to nothing and left the default.
  rtxui::ComputedStyle style;

  SECTION("justify-content") {
    rtxui::ApplyStyle(style, {"justify-content", "end"});
    CHECK(style.justify_content == rtxui::JustifyContent::FlexEnd);
    rtxui::ApplyStyle(style, {"justify-content", "start"});
    CHECK(style.justify_content == rtxui::JustifyContent::FlexStart);
  }

  SECTION("align-content") {
    rtxui::ApplyStyle(style, {"align-content", "end"});
    CHECK(style.align_content == rtxui::AlignContent::FlexEnd);
    rtxui::ApplyStyle(style, {"align-content", "start"});
    CHECK(style.align_content == rtxui::AlignContent::FlexStart);
  }

  SECTION("The flex- spellings still work") {
    rtxui::ApplyStyle(style, {"justify-content", "flex-end"});
    CHECK(style.justify_content == rtxui::JustifyContent::FlexEnd);
    rtxui::ApplyStyle(style, {"align-content", "space-evenly"});
    CHECK(style.align_content == rtxui::AlignContent::SpaceEvenly);
  }
}

TEST_CASE("Numeric values tolerate surrounding whitespace",
          "[style][robustness]") {
  // Regression (786b9835): std::from_chars refuses a leading space rather
  // than skipping it, so any declaration whose value arrived with whitespace
  // still attached parsed as 0 and silently became "no padding", "no width".
  // Multi-value shorthands are where this bites, since splitting `1 2` leaves
  // the separator on one side.
  rtxui::ComputedStyle style;

  SECTION("a single value padded on both sides") {
    rtxui::ApplyStyle(style, {"padding-top", "  5  "});
    CHECK(style.padding.top == 5);
  }

  SECTION("a shorthand whose parts carry the separator") {
    rtxui::ApplyStyle(style, {"padding", " 1  2   3 4 "});
    CHECK(style.padding.top == 1);
    CHECK(style.padding.right == 2);
    CHECK(style.padding.bottom == 3);
    CHECK(style.padding.left == 4);
  }

  SECTION("floats too") {
    rtxui::ApplyStyle(style, {"flex-grow", "  2.5 "});
    CHECK(style.flex_grow == 2.5f);
  }

  SECTION("whitespace alone is not a number") {
    rtxui::ApplyStyle(style, {"padding-left", "   "});
    CHECK(style.padding.left == 0);
  }
}

TEST_CASE("lighten() and darken() mix toward a color", "[style][color]") {
  // Regression: the two code paths computed different things. With a base
  // color resolved it added a flat `255 * amount` to each channel; with none
  // it deferred to paint time as an alpha overlay, which composites to
  // `c + amount * (target - c)`. So the same declaration produced one color or
  // another depending on whether something upstream happened to set one.
  //
  // Mixing is the right one of the two: it is what `color-mix(in srgb, ...)`
  // means, it cannot clip, and it is what the deferred form has always done.
  auto resolved = [](std::string_view base, std::string_view transform) {
    rtxui::ComputedStyle style;
    rtxui::ApplyStyle(style, {"background-color", base});
    rtxui::ApplyStyle(style, {"background-color", transform});
    REQUIRE(style.background_color.has_value());
    return *style.background_color;
  };
  auto deferred = [](std::string_view transform) {
    rtxui::ComputedStyle style;
    rtxui::ApplyStyle(style, {"background-color", transform});
    REQUIRE(style.background_color.has_value());
    return *style.background_color;
  };

  SECTION("halfway to white, not a flat shift") {
    // 100 + 0.5 * (255 - 100) = 177. The old formula gave 100 + 127 = 227.
    CHECK(resolved("rgb(100, 100, 100)", "lighten(50%)") ==
          Color::RGB(177, 177, 177));
    // A bright color moves less, and cannot clip. The old formula saturated
    // everything above 128 to pure white.
    CHECK(resolved("rgb(200, 200, 200)", "lighten(50%)") ==
          Color::RGB(227, 227, 227));
  }

  SECTION("halfway to black") {
    CHECK(resolved("rgb(100, 100, 100)", "darken(50%)") ==
          Color::RGB(50, 50, 50));
  }

  SECTION("both paths agree") {
    // With no base the result is deferred as an overlay; compositing it over
    // the base has to land where resolving against that base directly does.
    const Color base = Color::RGB(100, 100, 100);
    const Color direct = resolved("rgb(100, 100, 100)", "lighten(50%)");
    const Color composited = Blend(deferred("lighten(50%)"), base);
    // One unit of slack for the two roundings, which happen in different orders.
    CHECK(std::abs(int(direct.r) - int(composited.r)) <= 1);
    CHECK(std::abs(int(direct.g) - int(composited.g)) <= 1);
    CHECK(std::abs(int(direct.b) - int(composited.b)) <= 1);
  }

  SECTION("0% and 100% are the endpoints") {
    CHECK(resolved("rgb(60, 70, 80)", "lighten(0%)") ==
          Color::RGB(60, 70, 80));
    CHECK(resolved("rgb(60, 70, 80)", "lighten(100%)") ==
          Color::RGB(255, 255, 255));
    CHECK(resolved("rgb(60, 70, 80)", "darken(100%)") ==
          Color::RGB(0, 0, 0));
  }
}
