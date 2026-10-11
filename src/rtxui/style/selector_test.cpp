// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "catch2/catch_message.hpp"
#include "rtxui/base/clock.hpp"
#include "rtxui/color.hpp"
#include "rtxui/component.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/event.hpp"
#include "rtxui/headless.hpp"
#include "rtxui/layout/style.hpp"
#include "rtxui/refcounted.hpp"
#include "rtxui/screen.hpp"
#include "rtxui/task.hpp"
#include "rtxui/terminal/terminal_device.hpp"
#include "rtxui/testing/component_as.hpp"

using namespace rtxui;

namespace {

class PseudoVarTestComponent : public rtxui::Component<PseudoVarTestComponent> {
 public:
  std::string_view view = R"(
    <div id="box">Box</div>
    <style>
      #box {
        --tone: rgb(1, 2, 3);
        background-color: var(--tone);
      }
      #box:hover {
        --tone: rgb(7, 8, 9);
        background-color: var(--tone);
      }
    </style>
  )";
};

TEST_CASE("Custom properties in pseudo-class rules",
          "[component][css][var][pseudo]") {
  auto container = rtxui::Ref<PseudoVarTestComponent>::New();
  container->Mount();

  auto* box = container->Root()->QuerySelector("#box");
  REQUIRE(box != nullptr);
  CHECK(box->base_style.background_color.value_or(Color()) ==
        Color::RGB(1, 2, 3));

  box->set_hovered(true);
  container->ResolveTargetStyles(rtxui::time::GetTimeMs() + 200.0);
  CHECK(box->style.background_color.value_or(Color()) == Color::RGB(7, 8, 9));

  box->set_hovered(false);
  container->ResolveTargetStyles(rtxui::time::GetTimeMs() + 400.0);
  CHECK(box->style.background_color.value_or(Color()) == Color::RGB(1, 2, 3));
}

class ComplexSelectorTestApp : public Component<ComplexSelectorTestApp> {
 public:
  std::string_view view = R"(
    <div>
      <div>
        <span id="direct">Direct child</span>
        <p>
          <span id="nested">Nested inside p</span>
        </p>
      </div>
    </div>
    <style>
      div span {
        background-color: rgb(255, 0, 0);
      }
      div > span {
        color: rgb(0, 255, 0);
      }
    </style>
  )";
};

}  // namespace

TEST_CASE("CSS Child and Descendant Combinator Matching", "[component][css]") {
  auto app = Ref<ComplexSelectorTestApp>::New();
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  device->TriggerResize(80, 24);
  rtxui::Screen screen(app, device);

  auto* root_div = app->Root();
  REQUIRE(root_div != nullptr);

  auto* direct_span = root_div->QuerySelector("#direct");
  REQUIRE(direct_span != nullptr);
  // It is a descendant of div (div span -> bg red) AND a direct child of div
  // (div > span -> color green)
  CHECK(direct_span->style.background_color.value() == Color::RGB(255, 0, 0));
  CHECK(direct_span->style.foreground_color.value() == Color::RGB(0, 255, 0));

  auto* nested_span = root_div->QuerySelector("#nested");
  REQUIRE(nested_span != nullptr);
  // It is a descendant of div (div span -> bg red) but NOT a direct child (div
  // > span should NOT match, color is default/unset)
  CHECK(nested_span->style.background_color.value() == Color::RGB(255, 0, 0));
  CHECK(!nested_span->style.foreground_color.has_value());
}

namespace {

class AdvancedCSSFeaturesTestApp
    : public Component<AdvancedCSSFeaturesTestApp> {
 public:
  std::string_view view = R"(
    <div>
      <div>
        <span id="child1">First</span>
        <span id="child2">Second</span>
        <span id="child3">Third</span>
        <span id="child4">Fourth</span>
      </div>
    </div>
    <style>
      span:first-child {
        background-color: rgb(255, 0, 0);
      }
      span:last-child {
        background-color: rgb(0, 255, 0);
      }
      span:nth-child(even) {
        color: rgb(0, 0, 255);
      }
      span#child2 + span {
        display: none;
      }
      span#child1 ~ span {
        margin-left: 10px;
      }
    </style>
  )";
};

}  // namespace

TEST_CASE("CSS Sibling Combinators and Structural Pseudo-classes",
          "[component][css]") {
  auto app = Ref<AdvancedCSSFeaturesTestApp>::New();
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  device->TriggerResize(80, 24);
  rtxui::Screen screen(app, device);

  auto* root_div = app->Root();
  REQUIRE(root_div != nullptr);

  auto* child1 = root_div->QuerySelector("#child1");
  auto* child2 = root_div->QuerySelector("#child2");
  auto* child3 = root_div->QuerySelector("#child3");
  auto* child4 = root_div->QuerySelector("#child4");

  REQUIRE(child1 != nullptr);
  REQUIRE(child2 != nullptr);
  REQUIRE(child3 != nullptr);
  REQUIRE(child4 != nullptr);

  // Test first-child
  CHECK(child1->style.background_color.value() == Color::RGB(255, 0, 0));
  CHECK(!child2->style.background_color.has_value());

  // Test last-child
  CHECK(child4->style.background_color.value() == Color::RGB(0, 255, 0));
  CHECK(!child3->style.background_color.has_value());

  // Test nth-child(even)
  CHECK(!child1->style.foreground_color.has_value());
  CHECK(child2->style.foreground_color.value() == Color::RGB(0, 0, 255));
  CHECK(!child3->style.foreground_color.has_value());
  CHECK(child4->style.foreground_color.value() == Color::RGB(0, 0, 255));

  // Test adjacent sibling combinator (+)
  CHECK(child3->style.display_none == true);
  CHECK(child2->style.display_none == false);

  // Test general sibling combinator (~)
  CHECK(child1->style.margin.left == 0);
  CHECK(child2->style.margin.left == 10);
  CHECK(child3->style.margin.left == 10);
  CHECK(child4->style.margin.left == 10);
}

namespace {

class StructuralPseudoTestApp : public Component<StructuralPseudoTestApp> {
 public:
  std::string_view view = R"(
    <div>
      <div id="many">
        <span id="a">A</span>
        <span id="b">B</span>
        <span id="c">C</span>
      </div>
      <div id="solo">
        <span id="only">Only</span>
      </div>
      <div id="blank"></div>
      <div id="ws">
      </div>
      <div id="filled">text</div>
    </div>
    <style>
      span:only-child { background-color: rgb(1, 2, 3); }
      span:nth-last-child(1) { color: rgb(4, 5, 6); }
      span:nth-last-child(odd) { margin-left: 7; }
      div:empty { background-color: rgb(9, 9, 9); }
    </style>
  )";
};

}  // namespace

TEST_CASE("Structural pseudo-classes: only-child, empty, nth-last-child",
          "[component][css]") {
  auto app = Ref<StructuralPseudoTestApp>::New();
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  device->TriggerResize(80, 24);
  rtxui::Screen screen(app, device);
  auto* root = app->Root();
  REQUIRE(root != nullptr);

  auto at = [&](const char* sel) {
    auto* e = root->QuerySelector(sel);
    REQUIRE(e != nullptr);
    return e;
  };

  SECTION(":only-child needs to be the sole structural sibling") {
    CHECK(at("#only")->style.background_color.value() == Color::RGB(1, 2, 3));
    CHECK_FALSE(at("#a")->style.background_color.has_value());
    CHECK_FALSE(at("#b")->style.background_color.has_value());
    CHECK_FALSE(at("#c")->style.background_color.has_value());
  }

  SECTION(":nth-last-child counts from the end") {
    // C is last, so nth-last-child(1); A is third from the end.
    CHECK(at("#c")->style.foreground_color.value() == Color::RGB(4, 5, 6));
    CHECK_FALSE(at("#a")->style.foreground_color.has_value());
    CHECK_FALSE(at("#b")->style.foreground_color.has_value());
    // Positions from the end: C=1, B=2, A=3 -> the odd ones are C and A.
    CHECK(at("#c")->style.margin.left == 7);
    CHECK(at("#a")->style.margin.left == 7);
    CHECK(at("#b")->style.margin.left == 0);
  }

  SECTION(":empty ignores whitespace-only content") {
    CHECK(at("#blank")->style.background_color.value() == Color::RGB(9, 9, 9));
    // Indentation alone must not make an element non-empty, or :empty would
    // never match anything written across multiple lines.
    CHECK(at("#ws")->style.background_color.value() == Color::RGB(9, 9, 9));
    CHECK_FALSE(at("#filled")->style.background_color.has_value());
    // An element with element children is not empty either.
    CHECK_FALSE(at("#many")->style.background_color.has_value());
  }
}

namespace {

class NotSelectorApp : public rtxui::Component<NotSelectorApp> {
 public:
  std::string_view view = R"(
    <div>
      <span id="a" class="keep">a</span>
      <span id="b" class="skip">b</span>
      <span id="c">c</span>
      <div id="d" class="skip">d</div>
    </div>
    <style>
      span:not(.skip) { color: rgb(1, 1, 1); }
      span:not(#b) { margin-left: 3; }
      :not(span) { padding-left: 4; }
      /* Malformed arguments must parse, match nothing, and not crash. */
      span:not() { color: rgb(9, 9, 9); }
      span:not(   ) { color: rgb(9, 9, 9); }
    </style>
  )";
};

}  // namespace

TEST_CASE("The :not() selector", "[component][css][selector]") {
  auto app = rtxui::Ref<NotSelectorApp>::New();
  app->Mount();

  auto at = [&](const char* sel) {
    auto* e = app->Root()->QuerySelector(sel);
    REQUIRE(e != nullptr);
    return e;
  };

  SECTION("negating a class") {
    CHECK(at("#a")->style.foreground_color == Color::RGB(1, 1, 1));
    CHECK(at("#c")->style.foreground_color == Color::RGB(1, 1, 1));
    CHECK_FALSE(at("#b")->style.foreground_color.has_value());
  }

  SECTION("negating an id") {
    CHECK(at("#a")->style.margin.left == 3);
    CHECK(at("#c")->style.margin.left == 3);
    CHECK(at("#b")->style.margin.left == 0);
  }

  SECTION("negating a tag") {
    CHECK(at("#d")->style.padding.left == 4);
    CHECK(at("#a")->style.padding.left == 0);
  }

  SECTION("an empty or malformed argument matches nothing and does not crash") {
    // The stylesheet above carries `span:not()` and `span:not(   )`. They must
    // match nothing rather than everything -- an empty negation that matched
    // would recolour every span here.
    CHECK(at("#a")->style.foreground_color == Color::RGB(1, 1, 1));
    CHECK_FALSE(at("#b")->style.foreground_color.has_value());
  }
}

namespace {

class NotWithPseudoApp : public rtxui::Component<NotWithPseudoApp> {
 public:
  std::string_view view = R"(
    <div>
      <span id="i1" class="x" data-k="a:b">1</span>
      <span id="i2" class="x">2</span>
      <span id="i3">3</span>
    </div>
    <style>
      span:not(:first-child) { padding-top: 2; }
      span:not(:nth-child(2)) { padding-bottom: 3; }
      /* The argument is one compound: both halves must match to negate. */
      span:not(.x:first-child) { padding-left: 4; }
      /* A ':' inside an attribute value is not a pseudo-class delimiter. */
      span[data-k="a:b"] { padding-right: 5; }
    </style>
  )";
};

}  // namespace

TEST_CASE("A pseudo-class inside :not()", "[component][css][selector]") {
  auto app = rtxui::Ref<NotWithPseudoApp>::New();
  app->Mount();

  auto at = [&](const char* sel) {
    auto* e = app->Root()->QuerySelector(sel);
    REQUIRE(e != nullptr);
    return e;
  };

  SECTION("negating a structural pseudo-class") {
    // The `:not(:first-child)` spacing idiom. Splitting the selector on a raw
    // ':' used to yield the two junk tokens "not(" and "first-child)", which
    // matched no branch of the matcher -- so the rule silently applied to
    // every span, #i1 included.
    CHECK(at("#i1")->style.padding.top == 0);
    CHECK(at("#i2")->style.padding.top == 2);
    CHECK(at("#i3")->style.padding.top == 2);
  }

  SECTION("negating a pseudo-class that takes an argument") {
    CHECK(at("#i1")->style.padding.bottom == 3);
    CHECK(at("#i2")->style.padding.bottom == 0);
    CHECK(at("#i3")->style.padding.bottom == 3);
  }

  SECTION("the argument's compound and pseudo-classes are ANDed") {
    // #i2 carries .x but is not the first child, and #i3 is neither, so only
    // #i1 -- which is both -- is negated away.
    CHECK(at("#i1")->style.padding.left == 0);
    CHECK(at("#i2")->style.padding.left == 4);
    CHECK(at("#i3")->style.padding.left == 4);
  }

  SECTION("a colon inside an attribute value does not start a pseudo-class") {
    CHECK(at("#i1")->style.padding.right == 5);
    CHECK(at("#i2")->style.padding.right == 0);
  }
}

namespace {

// The template is built at runtime, so the nesting depth can be a number
// rather than a wall of literal text.
class DeepNotApp : public rtxui::Component<DeepNotApp> {
 public:
  std::string source;
  std::string_view view;
};

// `span:not(:not(...(inner)...))`, nested `depth` levels deep.
std::string NestedNot(int depth, std::string_view inner) {
  std::string selector = "span:not(";
  for (int i = 1; i < depth; ++i) {
    selector += ":not(";
  }
  selector += inner;
  for (int i = 0; i < depth; ++i) {
    selector += ")";
  }
  return selector;
}

}  // namespace

TEST_CASE("Nested :not() is bounded", "[component][css][selector]") {
  auto app = rtxui::Ref<DeepNotApp>::New();
  app->source =
      R"(<div><span id="d1" class="x">1</span><span id="d2">2</span></div>)"
      "<style>" +
      NestedNot(2, ".x") + " { padding-left: 1; }" +    //
      NestedNot(3, ".x") + " { padding-bottom: 1; }" +  //
      NestedNot(32, ".x") + " { padding-top: 1; }" +    //
      NestedNot(33, ".x") + " { padding-right: 9; }" +  //
      NestedNot(500, ".x") + " { margin-top: 9; }" +    //
      "</style>";
  app->view = app->source;
  app->Mount();

  auto at = [&](const char* sel) {
    auto* e = app->Root()->QuerySelector(sel);
    REQUIRE(e != nullptr);
    return e;
  };

  SECTION("each level of nesting negates the one inside it") {
    // Two levels: `:not(:not(.x))` is `.x` again.
    CHECK(at("#d1")->style.padding.left == 1);
    CHECK(at("#d2")->style.padding.left == 0);
    // Three levels flips it back.
    CHECK(at("#d1")->style.padding.bottom == 0);
    CHECK(at("#d2")->style.padding.bottom == 1);
  }

  SECTION("nesting up to the cap is still evaluated") {
    CHECK(at("#d1")->style.padding.top == 1);
    CHECK(at("#d2")->style.padding.top == 0);
  }

  SECTION("nesting past the cap matches nothing") {
    // Not "matches everything", and not something that depends on whether the
    // depth is odd or even: refusing the token up front is what makes the
    // answer the same for 33 and for 500. Left unbounded this recursed until
    // it segfaulted, and the text copying made it quadratic long before that.
    CHECK(at("#d1")->style.padding.right == 0);
    CHECK(at("#d2")->style.padding.right == 0);
    CHECK(at("#d1")->style.margin.top == 0);
    CHECK(at("#d2")->style.margin.top == 0);
  }
}

namespace {

class NotListApp : public rtxui::Component<NotListApp> {
 public:
  std::string_view view = R"(
    <div>
      <span id="l1" class="a">1</span>
      <span id="l2" class="b">2</span>
      <span id="l3">3</span>
      <span id="l4" class="a">4</span>
    </div>
    <style>
      span:not(.a, .b) { padding-top: 1; }
      /* Entries are full compounds, pseudo-classes included. */
      span:not(#l1, .a:last-child) { padding-bottom: 2; }
      /* A one-entry list is still the plain form. */
      span:not(.a) { padding-left: 3; }
      /* A stray or trailing comma makes the whole argument malformed. */
      span:not(.a, ) { padding-right: 9; }
      span:not(, .a) { padding-right: 9; }
      /* A combinator inside the negation is still unsupported. */
      span:not(div span) { margin-top: 9; }
    </style>
  )";
};

}  // namespace

TEST_CASE("A selector list inside :not()", "[component][css][selector]") {
  auto app = rtxui::Ref<NotListApp>::New();
  app->Mount();

  auto at = [&](const char* sel) {
    auto* e = app->Root()->QuerySelector(sel);
    REQUIRE(e != nullptr);
    return e;
  };

  SECTION("an element must match no entry in the list") {
    // Only #l3 carries neither class. The commas also prove the stylesheet
    // parser no longer ends the selector at the first one.
    CHECK(at("#l3")->style.padding.top == 1);
    CHECK(at("#l1")->style.padding.top == 0);
    CHECK(at("#l2")->style.padding.top == 0);
    CHECK(at("#l4")->style.padding.top == 0);
  }

  SECTION("each entry is a compound that may carry pseudo-classes") {
    // #l1 is excluded by the id, #l4 by being an `.a` that is last.
    CHECK(at("#l1")->style.padding.bottom == 0);
    CHECK(at("#l4")->style.padding.bottom == 0);
    CHECK(at("#l2")->style.padding.bottom == 2);
    CHECK(at("#l3")->style.padding.bottom == 2);
  }

  SECTION("a single-entry list behaves like the plain form") {
    CHECK(at("#l2")->style.padding.left == 3);
    CHECK(at("#l3")->style.padding.left == 3);
    CHECK(at("#l1")->style.padding.left == 0);
  }

  SECTION("a stray or trailing comma matches nothing") {
    // Ignoring the gap would let these two rules style every span.
    for (const char* id : {"#l1", "#l2", "#l3", "#l4"}) {
      CHECK(at(id)->style.padding.right == 0);
    }
  }

  SECTION("a combinator inside the negation still matches nothing") {
    for (const char* id : {"#l1", "#l2", "#l3", "#l4"}) {
      CHECK(at(id)->style.margin.top == 0);
    }
  }
}

namespace {

class AnPlusBApp : public rtxui::Component<AnPlusBApp> {
 public:
  std::string_view view = R"(
    <div>
      <span id="n1">1</span>
      <span id="n2">2</span>
      <span id="n3">3</span>
      <span id="n4">4</span>
      <span id="n5">5</span>
      <span id="n6">6</span>
    </div>
    <style>
      span:nth-child(3n) { padding-top: 1; }
      span:nth-child(n+3) { padding-bottom: 2; }
      span:nth-child(-n+2) { padding-left: 3; }
      /* Whitespace anywhere, and the `n` is case-insensitive. */
      span:nth-child( 2N + 1 ) { padding-right: 4; }
      span:nth-last-child(3n+1) { margin-left: 5; }
      /* An offset must carry its own sign, and junk is still junk. */
      span:nth-child(2n3) { margin-top: 9; }
      span:nth-child(n+) { margin-top: 9; }
      span:nth-child(2junk) { margin-top: 9; }
    </style>
  )";
};

}  // namespace

TEST_CASE("An+B arguments to :nth-child()", "[component][css][selector]") {
  auto app = rtxui::Ref<AnPlusBApp>::New();
  app->Mount();

  auto at = [&](const char* sel) {
    auto* e = app->Root()->QuerySelector(sel);
    REQUIRE(e != nullptr);
    return e;
  };

  SECTION("a bare step selects every Nth sibling") {
    CHECK(at("#n3")->style.padding.top == 1);
    CHECK(at("#n6")->style.padding.top == 1);
    CHECK(at("#n1")->style.padding.top == 0);
    CHECK(at("#n2")->style.padding.top == 0);
    CHECK(at("#n4")->style.padding.top == 0);
    CHECK(at("#n5")->style.padding.top == 0);
  }

  SECTION("a step of 1 with an offset selects everything from there on") {
    CHECK(at("#n2")->style.padding.bottom == 0);
    CHECK(at("#n3")->style.padding.bottom == 2);
    CHECK(at("#n6")->style.padding.bottom == 2);
  }

  SECTION("a negative step counts back down to the first sibling") {
    // `-n+2` is the "first two" idiom: k=0 gives 2, k=1 gives 1, and k=2
    // gives 0, which is not a sibling position.
    CHECK(at("#n1")->style.padding.left == 3);
    CHECK(at("#n2")->style.padding.left == 3);
    CHECK(at("#n3")->style.padding.left == 0);
  }

  SECTION("whitespace and capitalisation are allowed") {
    // `2N + 1` is the odd siblings. The spaces around the '+' also prove the
    // selector splitter no longer reads it as a sibling combinator.
    CHECK(at("#n1")->style.padding.right == 4);
    CHECK(at("#n3")->style.padding.right == 4);
    CHECK(at("#n5")->style.padding.right == 4);
    CHECK(at("#n2")->style.padding.right == 0);
  }

  SECTION("nth-last-child counts An+B from the end") {
    // Positions from the end are 1 (#n6) and 4 (#n3).
    CHECK(at("#n6")->style.margin.left == 5);
    CHECK(at("#n3")->style.margin.left == 5);
    CHECK(at("#n1")->style.margin.left == 0);
  }

  SECTION("a malformed argument matches nothing") {
    for (const char* id : {"#n1", "#n2", "#n3", "#n4", "#n5", "#n6"}) {
      CHECK(at(id)->style.margin.top == 0);
    }
  }
}

namespace {

class UnknownPseudoApp : public rtxui::Component<UnknownPseudoApp> {
 public:
  std::string_view view = R"(
    <div>
      <span id="u1">1</span>
      <span id="u2">2</span>
    </div>
    <style>
      span:hovr { padding-top: 7; }
      span:first-child:hovr { padding-bottom: 7; }
      span:nth-child(1 { padding-right: 7; }
      span:first-child { padding-left: 6; }
    </style>
  )";
};

}  // namespace

TEST_CASE("An unrecognized pseudo-class matches nothing",
          "[component][css][selector]") {
  auto app = rtxui::Ref<UnknownPseudoApp>::New();
  app->Mount();

  auto at = [&](const char* sel) {
    auto* e = app->Root()->QuerySelector(sel);
    REQUIRE(e != nullptr);
    return e;
  };

  SECTION("a misspelled pseudo-class styles nothing, not everything") {
    // Skipping a token the matcher does not understand would drop the
    // constraint the author wrote and apply the rule to every span.
    CHECK(at("#u1")->style.padding.top == 0);
    CHECK(at("#u2")->style.padding.top == 0);
  }

  SECTION("one bad token disqualifies the whole compound") {
    // #u1 really is the first child, so only the unknown `:hovr` can keep
    // this rule from applying.
    CHECK(at("#u1")->style.padding.bottom == 0);
  }

  SECTION("an unterminated functional pseudo-class matches nothing") {
    CHECK(at("#u1")->style.padding.right == 0);
  }

  SECTION("recognized pseudo-classes still match") {
    CHECK(at("#u1")->style.padding.left == 6);
    CHECK(at("#u2")->style.padding.left == 0);
  }
}

namespace {

class OfTypeSelectorApp : public rtxui::Component<OfTypeSelectorApp> {
 public:
  // The interleaving matters: span #s1 is the parent's 1st child but so is
  // nothing else of its type, and #p1 is the 2nd child yet the 1st <p>.
  std::string_view view = R"(
    <div>
      <span id="s1">1</span>
      <p id="p1">2</p>
      <span id="s2">3</span>
      <p id="p2">4</p>
      <p id="p3">5</p>
    </div>
    <div id="lonely-parent"><span id="only-span">x</span></div>
    <style>
      p:first-of-type { color: rgb(1, 1, 1); }
      p:last-of-type { color: rgb(2, 2, 2); }
      p:nth-of-type(2) { margin-left: 5; }
      span:only-of-type { padding-left: 6; }
    </style>
  )";
};

}  // namespace

TEST_CASE("The -of-type structural pseudo-classes",
          "[component][css][selector]") {
  auto app = rtxui::Ref<OfTypeSelectorApp>::New();
  app->Mount();

  auto at = [&](const char* sel) {
    auto* e = app->Root()->QuerySelector(sel);
    REQUIRE(e != nullptr);
    return e;
  };

  SECTION("first-of-type counts only same-tag siblings") {
    // #p1 is the parent's second child, but its first <p>.
    CHECK(at("#p1")->style.foreground_color == Color::RGB(1, 1, 1));
    CHECK_FALSE(at("#p2")->style.foreground_color == Color::RGB(1, 1, 1));
  }

  SECTION("last-of-type") {
    CHECK(at("#p3")->style.foreground_color == Color::RGB(2, 2, 2));
  }

  SECTION("nth-of-type") {
    // Second <p>, even though it is the fourth child.
    CHECK(at("#p2")->style.margin.left == 5);
    CHECK(at("#p1")->style.margin.left == 0);
    CHECK(at("#p3")->style.margin.left == 0);
  }

  SECTION("only-of-type ignores siblings of other tags") {
    // #only-span is its parent's only child at all.
    CHECK(at("#only-span")->style.padding.left == 6);
    // #s1 has another <span> sibling, so it is not the only one of its type,
    // even though it has non-span siblings too.
    CHECK(at("#s1")->style.padding.left == 0);
  }
}

namespace {

class HasSelectorApp : public rtxui::Component<HasSelectorApp> {
 public:
  std::string_view view = R"(
    <div>
      <div id="with-error" class="card">
        <p><span class="error">!</span></p>
      </div>
      <div id="with-child" class="card">
        <span class="direct">x</span>
      </div>
      <div id="plain" class="card">
        <span>ok</span>
      </div>
      <span id="before-badge">a</span>
      <span class="badge">b</span>
      <span id="before-later">c</span>
      <span>d</span>
      <span class="late">e</span>
    </div>
    <style>
      .card:has(.error) { padding-left: 1; }
      .card:has(> .direct) { padding-right: 2; }
      .card:has(> .error) { margin-left: 5; }
      .card:has(.missing, .direct) { padding-top: 1; }
      span:has(+ .badge) { margin-left: 3; }
      span:has(~ .late) { margin-right: 4; }
      .card:not(:has(.error)) { padding-bottom: 1; }
      .card:has(span.direct:first-child) { margin-right: 6; }
      /* Malformed arguments must parse, match nothing, and not crash. */
      .card:has() { margin-top: 9; }
      .card:has(> ) { margin-top: 9; }
      .card:has(.a > > .b) { margin-top: 9; }
    </style>
  )";
};

}  // namespace

TEST_CASE("The :has() selector", "[component][css][selector]") {
  auto app = rtxui::Ref<HasSelectorApp>::New();
  app->Mount();

  auto at = [&](const char* sel) {
    auto* e = app->Root()->QuerySelector(sel);
    REQUIRE(e != nullptr);
    return e;
  };

  SECTION("a descendant") {
    CHECK(at("#with-error")->style.padding.left == 1);
    CHECK(at("#with-child")->style.padding.left == 0);
    CHECK(at("#plain")->style.padding.left == 0);
  }

  SECTION("a child") {
    CHECK(at("#with-child")->style.padding.right == 2);
    // The error is a grandchild, not a child.
    CHECK(at("#with-error")->style.margin.left == 0);
  }

  SECTION("a selector list") {
    CHECK(at("#with-child")->style.padding.top == 1);
    CHECK(at("#plain")->style.padding.top == 0);
  }

  SECTION("the next sibling, and any later one") {
    CHECK(at("#before-badge")->style.margin.left == 3);
    CHECK(at("#before-later")->style.margin.left == 0);
    CHECK(at("#before-badge")->style.margin.right == 4);
    CHECK(at("#before-later")->style.margin.right == 4);
    CHECK(at(".late")->style.margin.right == 0);
  }

  SECTION("inside :not(), and with pseudo-classes of its own") {
    CHECK(at("#plain")->style.padding.bottom == 1);
    CHECK(at("#with-error")->style.padding.bottom == 0);
    CHECK(at("#with-child")->style.margin.right == 6);
  }

  SECTION("a malformed argument matches nothing") {
    CHECK(at("#with-error")->style.margin.top == 0);
    CHECK(at("#with-child")->style.margin.top == 0);
    CHECK(at("#plain")->style.margin.top == 0);
  }
}

namespace {

// Shows what its user projects into it only while `shown` is set: the
// projected elements belong to its user, but come and go when this component
// re-renders on its own.
class HasReveal : public rtxui::Component<HasReveal> {
 public:
  bool shown = true;
  HasReveal() { Bind(shown); }
  std::string_view view = R"(
    <div><if condition="{shown}"><slot></slot></if></div>
  )";
};

// Marks its own internal element .invalid, which is no business of the
// stylesheets around it.
class HasField : public rtxui::Component<HasField> {
 public:
  std::string_view view = R"(
    <span class="invalid">field</span>
  )";
};

class HasForm : public rtxui::Component<HasForm> {
 public:
  HasForm() {
    Import<HasReveal>();
    Import<HasField>();
  }
  std::string_view view = R"(
    <div>
      <div class="form"><HasReveal><span class="invalid">!</span></HasReveal></div>
      <div class="other"><HasField/></div>
    </div>
    <style>
      .form:has(.invalid) { padding-left: 4; }
      .other:has(.invalid) { padding-left: 4; }
    </style>
  )";
};

}  // namespace

TEST_CASE("A :has() rule sees only its own component's elements",
          "[component][css][selector]") {
  auto app = rtxui::Ref<HasForm>::New();
  app->Mount();
  auto* other = app->Root()->QuerySelector(".other");
  REQUIRE(other != nullptr);
  // The .invalid inside <HasField> is that component's own.
  CHECK(other->style.padding.left == 0);
}

TEST_CASE("A :has() rule follows its elements projected into a component",
          "[component][css][selector]") {
  auto app = rtxui::Ref<HasForm>::New();
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  device->TriggerResize(20, 3);
  rtxui::Screen screen(app, device);
  screen.Draw();

  auto form_padding = [&] {
    auto* form = app->Root()->QuerySelector(".form");
    REQUIRE(form != nullptr);
    return form->style.padding.left;
  };
  auto invalid_count = [&] {
    int count = 0;
    app->Root()->Visit([&](rtxui::Element& e) {
      count += std::ranges::count(e.classes, std::string("invalid"));
    });
    return count;
  };
  CHECK(form_padding() == 4);
  auto* reveal =
      ComponentAs<HasReveal>(app->Root()->QuerySelector("HasReveal"));
  REQUIRE(reveal != nullptr);

  rtxui::PostTask([&] { reveal->shown = false; });
  screen.Step();
  INFO("invalid elements: " << invalid_count());
  CHECK(form_padding() == 0);

  rtxui::PostTask([&] { reveal->shown = true; });
  screen.Step();
  INFO("invalid elements: " << invalid_count());
  CHECK(form_padding() == 4);
}

namespace {

class AliasedCounter : public rtxui::Component<AliasedCounter> {
 public:
  int clicks = 0;
  AliasedCounter() { Bind(clicks); }
  std::string_view view = R"(<span>{clicks}</span>)";
};

class WrittenTags : public rtxui::Component<WrittenTags> {
 public:
  WrittenTags() { Import<AliasedCounter>("my-counter"); }
  std::string_view view = R"(
    <div>
      <my-counter/>
      <tabs><tab-pane label="a">a</tab-pane></tabs>
    </div>
    <style>
      my-counter { padding-left: 1; }
      tab-pane { padding-left: 2; }
    </style>
  )";
};

}  // namespace

TEST_CASE("A selector names a tag as the template wrote it",
          "[component][css][selector]") {
  auto app = rtxui::Ref<WrittenTags>::New();
  app->Mount();
  // Not by class name: AliasedCounter is <my-counter> here, tab_pane is
  // <tab-pane>.
  auto* counter = app->Root()->QuerySelector("my-counter");
  auto* pane = app->Root()->QuerySelector("tab-pane");
  REQUIRE(counter != nullptr);
  REQUIRE(pane != nullptr);
  CHECK(counter->style.padding.left == 1);
  CHECK(pane->style.padding.left == 2);
}

namespace {

class FocusWithinApp : public rtxui::Component<FocusWithinApp> {
 public:
  std::string_view view = R"(
    <div>
      <div class="panel" id="first"><div><button>one</button></div></div>
      <div class="panel" id="second"><input/></div>
    </div>
    <style>
      .panel:focus-within { padding-left: 2; }
      .panel:not(:focus-within) { padding-right: 1; }
      button:focus-within { margin-left: 3; }
    </style>
  )";
};

}  // namespace

TEST_CASE("The :focus-within selector", "[component][css][selector]") {
  auto app = rtxui::Ref<FocusWithinApp>::New();
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  device->TriggerResize(40, 10);
  rtxui::Screen screen(app, device);
  screen.Draw();

  auto at = [&](const char* sel) {
    auto* e = app->Root()->QuerySelector(sel);
    REQUIRE(e != nullptr);
    return e;
  };
  CHECK(at("#first")->style.padding.left == 0);
  CHECK(at("#first")->style.padding.right == 1);
  CHECK(at("#second")->style.padding.left == 0);

  // A focused grandchild.
  screen.Dispatch(rtxui::Event::Tab());
  screen.Draw();
  CHECK(at("#first")->style.padding.left == 2);
  CHECK(at("#first")->style.padding.right == 0);
  CHECK(at("#second")->style.padding.left == 0);
  // The focused element itself matches too.
  CHECK(at("button")->style.margin.left == 3);

  // Focus inside another component's internals counts as well.
  screen.Dispatch(rtxui::Event::Tab());
  screen.Draw();
  CHECK(at("#first")->style.padding.left == 0);
  CHECK(at("#second")->style.padding.left == 2);
  CHECK(at("#second")->style.padding.right == 0);
}

namespace {

class IsWhereApp : public rtxui::Component<IsWhereApp> {
 public:
  std::string_view view = R"(
    <div>
      <span id="a" class="a">a</span>
      <span id="b" class="b">b</span>
      <span id="c" class="c">c</span>
    </div>
    <style>
      span:is(.a, #b) { padding-left: 1; }
      span:is(.c:first-child, .c:last-child) { padding-right: 2; }
      :where(.a, .b) { margin-left: 4; }
      span { margin-left: 5; }
      .a { margin-bottom: 1; }
      span:where(.a) { margin-bottom: 2; }
      span:is(div span, .a) { margin-top: 9; }
      span:is() { margin-top: 9; }
      span:where(.a, ) { margin-top: 9; }
      span:not(:is(.a)) { padding-bottom: 1; }
    </style>
  )";
};

}  // namespace

TEST_CASE("The :is() and :where() selectors", "[component][css][selector]") {
  auto app = rtxui::Ref<IsWhereApp>::New();
  app->Mount();
  auto at = [&](const char* sel) {
    auto* e = app->Root()->QuerySelector(sel);
    REQUIRE(e != nullptr);
    return e;
  };

  SECTION("any entry of the list matches") {
    CHECK(at("#a")->style.padding.left == 1);
    CHECK(at("#b")->style.padding.left == 1);
    CHECK(at("#c")->style.padding.left == 0);
    CHECK(at("#c")->style.padding.right == 2);
    CHECK(at("#a")->style.padding.right == 0);
  }

  SECTION(":where() adds no specificity") {
    // `span` (0,0,1) outweighs `:where(.a, .b)` (0,0,0) wherever it appears.
    CHECK(at("#a")->style.margin.left == 5);
    // `.a` (0,1,0) outweighs the later `span:where(.a)` (0,0,1).
    CHECK(at("#a")->style.margin.bottom == 1);
  }

  SECTION("a malformed list matches nothing") {
    CHECK(at("#a")->style.margin.top == 0);
  }

  SECTION("nested in :not()") {
    CHECK(at("#a")->style.padding.bottom == 0);
    CHECK(at("#b")->style.padding.bottom == 1);
  }
}

namespace {

class IsTwiceApp : public rtxui::Component<IsTwiceApp> {
 public:
  std::string_view view = R"(
    <div>
      <span id="both" class="a b">x</span>
    </div>
    <style>
      span { background-color: #000000; }
      span:is(.a, .b) { background-color: lighten(50%); }
    </style>
  )";
};

}  // namespace

TEST_CASE("An element matching two entries of one :is() takes the rule once",
          "[component][css][selector]") {
  auto app = rtxui::Ref<IsTwiceApp>::New();
  app->Mount();
  auto* both = app->Root()->QuerySelector("#both");
  REQUIRE(both != nullptr);
  REQUIRE(both->style.background_color);
  CHECK(both->style.background_color->r == 127);
}

namespace {

class FocusVisibleApp : public rtxui::Component<FocusVisibleApp> {
 public:
  std::string_view view = R"(
    <div>
      <button id="button">press</button>
      <input id="field"/>
    </div>
    <style>
      button:focus { margin-left: 1; }
      button:focus-visible { margin-right: 2; }
      input:focus-visible { margin-left: 3; }
    </style>
  )";
};

}  // namespace

TEST_CASE("The :focus-visible selector", "[component][css][selector]") {
  auto app = rtxui::Ref<FocusVisibleApp>::New();
  rtxui::HeadlessScreen screen(app, 40, 10);
  auto at = [&](const char* sel) {
    auto* e = app->Root()->QuerySelector(sel);
    REQUIRE(e != nullptr);
    return e;
  };
  auto click = [&](const char* sel) {
    auto* e = at(sel);
    screen.Click(e->absolute_x() + 1, e->absolute_y());
  };

  // Focus from the keyboard shows.
  screen.Input("\t");
  CHECK(at("#button")->style.margin.left == 1);
  CHECK(at("#button")->style.margin.right == 2);

  // Focus from a click does not...
  screen.Input("\t");  // Away from the button, to click back onto it.
  click("#button");
  CHECK(at("#button")->style.margin.left == 1);
  CHECK(at("#button")->style.margin.right == 0);

  // ...until the keyboard is used again.
  screen.Input("x");
  CHECK(at("#button")->style.margin.right == 2);

  // A text field shows its focus however it got it: it reads the keyboard.
  click("#field");
  CHECK(at("#field")->style.margin.left == 3);
}
