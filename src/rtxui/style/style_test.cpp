// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/style/style.hpp"

#include <catch2/catch_test_macros.hpp>
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

  SECTION("none") {
    rtxui::ApplyStyle(style, {"text-decoration", "none"});
    CHECK(style.underlined.value_or(true) == false);
    CHECK(style.underlined_double.value_or(true) == false);
    CHECK(style.strikethrough.value_or(true) == false);
    CHECK(style.blink.value_or(true) == false);
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
    CHECK(style.background_color->r == 125);  // 100 + 25.5 -> 125
    CHECK(style.background_color->g == 175);
    CHECK(style.background_color->b == 225);
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
    CHECK(style.foreground_color->r == 49);  // 100 - 51
    CHECK(style.foreground_color->g == 99);
    CHECK(style.foreground_color->b == 149);
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


