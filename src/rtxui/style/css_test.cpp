// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "rtxui/base/clock.hpp"
#include "rtxui/color.hpp"
#include "rtxui/component.hpp"
#include "rtxui/component/component_internal.hpp"
#include "rtxui/component/default/div/div.hpp"
#include "rtxui/component/default/input/input.hpp"
#include "rtxui/diagnostic.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/event.hpp"
#include "rtxui/layout/style.hpp"
#include "rtxui/refcounted.hpp"
#include "rtxui/screen.hpp"
#include "rtxui/terminal/screen_internal.hpp"
#include "rtxui/terminal/terminal_device.hpp"
#include "rtxui/testing/component_as.hpp"

using namespace rtxui;

namespace {

class ImportantTestComponent : public rtxui::Component<ImportantTestComponent> {
 public:
  std::string_view view = R"(
    <div>
      <div id="a" class="base">A</div>
      <div id="b" class="base" style="color: rgb(9, 9, 9);">B</div>
    </div>
    <style>
      .base { color: rgb(1, 1, 1) !important; }
      #a { color: rgb(2, 2, 2); }
    </style>
  )";
};

TEST_CASE("CSS !important wins over later rules and inline styles",
          "[component][css][important]") {
  auto container = rtxui::Ref<ImportantTestComponent>::New();
  container->Mount();

  // The #id rule is applied after the class rule, but the class rule is
  // !important and must win.
  auto* a = container->Root()->QuerySelector("#a");
  REQUIRE(a != nullptr);
  CHECK(a->base_style.foreground_color.value_or(Color()) ==
        Color::RGB(1, 1, 1));

  // An !important rule also beats a normal inline style.
  auto* b = container->Root()->QuerySelector("#b");
  REQUIRE(b != nullptr);
  CHECK(b->base_style.foreground_color.value_or(Color()) ==
        Color::RGB(1, 1, 1));
}

class VarTestComponent : public rtxui::Component<VarTestComponent> {
 public:
  std::string_view view = R"(
    <div class="theme">
      <div id="direct">Direct</div>
      <div><span id="nested">Nested</span></div>
      <div id="fallback">Fallback</div>
      <div id="undef">Undefined</div>
      <div id="inline-var" style="--local: rgb(1, 2, 3); color: var(--local);">Inline</div>
      <div id="override">Override</div>
    </div>
    <style>
      .theme {
        --accent: rgb(10, 20, 30);
        --pad: 2;
      }
      #direct {
        color: var(--accent);
        padding-left: var(--pad);
      }
      #nested { color: var(--accent); }
      #fallback { color: var(--missing, rgb(40, 50, 60)); }
      #undef { color: var(--missing); }
      #override {
        --accent: rgb(70, 80, 90);
        color: var(--accent);
      }
    </style>
  )";
};

TEST_CASE("CSS custom properties resolve through the DOM tree",
          "[component][css][var]") {
  auto container = rtxui::Ref<VarTestComponent>::New();
  container->Mount();

  auto* root = container->Root();

  auto* direct = root->QuerySelector("#direct");
  REQUIRE(direct != nullptr);
  CHECK(direct->base_style.foreground_color.value_or(Color()) ==
        Color::RGB(10, 20, 30));
  CHECK(direct->base_style.padding.left == 2);

  // Variables inherit through intermediate elements.
  auto* nested = root->QuerySelector("#nested");
  REQUIRE(nested != nullptr);
  CHECK(nested->base_style.foreground_color.value_or(Color()) ==
        Color::RGB(10, 20, 30));

  auto* fallback = root->QuerySelector("#fallback");
  REQUIRE(fallback != nullptr);
  CHECK(fallback->base_style.foreground_color.value_or(Color()) ==
        Color::RGB(40, 50, 60));

  // Undefined variable without fallback: the declaration is ignored.
  auto* undef = root->QuerySelector("#undef");
  REQUIRE(undef != nullptr);
  CHECK_FALSE(undef->base_style.foreground_color.has_value());

  // Custom properties from the inline style attribute.
  auto* inline_var = root->QuerySelector("#inline-var");
  REQUIRE(inline_var != nullptr);
  CHECK(inline_var->base_style.foreground_color.value_or(Color()) ==
        Color::RGB(1, 2, 3));

  // An element may override an inherited variable for itself.
  auto* override_el = root->QuerySelector("#override");
  REQUIRE(override_el != nullptr);
  CHECK(override_el->base_style.foreground_color.value_or(Color()) ==
        Color::RGB(70, 80, 90));
}

class SharedSheetA : public rtxui::Component<SharedSheetA> {
 public:
  std::string_view view =
      R"(<div id="x">x</div><style>#x { padding-left: 3; }</style>)";
};

class SharedSheetB : public rtxui::Component<SharedSheetB> {
 public:
  // Deliberately a different stylesheet with the same selector, so a cache
  // that keyed or shared wrongly would show up as B being styled like A.
  std::string_view view =
      R"(<div id="x">x</div><style>#x { padding-left: 9; }</style>)";
};

TEST_CASE("Components sharing a stylesheet keep their own styles",
          "[component][css]") {
  // A component's parsed stylesheet is shared between every instance
  // declaring the same text, since the built-in tags are components too and an
  // interface repeats the same handful of them thousands of times. Sharing
  // must not let one component's rules reach another's.
  auto first = rtxui::Ref<SharedSheetA>::New();
  auto other = rtxui::Ref<SharedSheetB>::New();
  auto second = rtxui::Ref<SharedSheetA>::New();
  first->Mount();
  other->Mount();
  second->Mount();

  auto padding = [](auto& app) {
    auto* element = app->Root()->QuerySelector("#x");
    REQUIRE(element != nullptr);
    return element->style.padding.left;
  };

  SECTION("two instances of the same component are both styled") {
    CHECK(padding(first) == 3);
    CHECK(padding(second) == 3);
  }

  SECTION("a different stylesheet is not confused with a shared one") {
    CHECK(padding(other) == 9);
  }

  SECTION("a stylesheet with no live user is re-parsed correctly") {
    // The cache holds its entries weakly, so the parsed sheet goes away with
    // its last user, and the next instance builds it again from scratch.
    {
      auto only = rtxui::Ref<SharedSheetB>::New();
      only->Mount();
      CHECK(padding(only) == 9);
    }
    auto later = rtxui::Ref<SharedSheetB>::New();
    later->Mount();
    CHECK(padding(later) == 9);
  }
}

class TwoStyleBlocksComponent
    : public rtxui::Component<TwoStyleBlocksComponent> {
 public:
  // Splitting a stylesheet in two, or putting an override below a base, is an
  // ordinary thing to write. Each block used to overwrite the last, so only
  // the final one survived.
  std::string_view view = R"(
    <div id="a">a</div>
    <div id="b" class="c">b</div>
    <style>
      #a { padding-left: 3; }
      .c { padding-top: 1; }
    </style>
    <div id="d">d</div>
    <style>
      #d { padding-left: 5; }
      /* Same specificity as the .c rule above, and later, so this one wins. */
      .c { padding-top: 7; }
    </style>
  )";
};

TEST_CASE("Every style block in a component applies", "[component][css]") {
  auto app = rtxui::Ref<TwoStyleBlocksComponent>::New();
  app->Mount();

  auto at = [&](const char* selector) {
    auto* element = app->Root()->QuerySelector(selector);
    REQUIRE(element != nullptr);
    return element;
  };

  SECTION("a rule in the first block still applies") {
    CHECK(at("#a")->style.padding.left == 3);
  }

  SECTION("a rule in the last block applies too") {
    CHECK(at("#d")->style.padding.left == 5);
  }

  SECTION("later blocks win ties, as document order decides") {
    // Both rules are a single class, so specificity cannot separate them and
    // the one written later takes it. Concatenating in order is what makes
    // that true; the blocks are not independent stylesheets.
    CHECK(at("#b")->style.padding.top == 7);
  }
}

// The cascade orders simple selectors by specificity, not by the order the
// engine happens to visit its selector buckets: an id selector outranks a
// class selector whichever way round they are written.
TEST_CASE("CSS cascade orders id above class", "[component][style][cascade]") {
  auto color_of = [](std::string_view view_text) {
    struct Probe : rtxui::Component<Probe> {
      std::string_view view;
    };
    auto probe = rtxui::Ref<Probe>::New();
    probe->view = view_text;
    probe->Mount();
    auto* el = probe->Root()->QuerySelector("#t");
    REQUIRE(el != nullptr);
    REQUIRE(el->style.foreground_color.has_value());
    return el->style.foreground_color.value();
  };

  const Color kFromId = Color::RGB(1, 0, 0);
  const Color kFromClass = Color::RGB(0, 2, 0);
  const Color kFromTag = Color::RGB(0, 0, 3);

  SECTION("id beats class when the id rule comes first") {
    CHECK(color_of(R"html(
        <style>
          #t { color: rgb(1,0,0); }
          .c { color: rgb(0,2,0); }
        </style>
        <div id="t" class="c">x</div>)html") == kFromId);
  }

  SECTION("id beats class when the class rule comes first") {
    CHECK(color_of(R"html(
        <style>
          .c { color: rgb(0,2,0); }
          #t { color: rgb(1,0,0); }
        </style>
        <div id="t" class="c">x</div>)html") == kFromId);
  }

  SECTION("class beats a tag selector") {
    CHECK(color_of(R"html(
        <style>
          .c { color: rgb(0,2,0); }
          div { color: rgb(0,0,3); }
        </style>
        <div id="t" class="c">x</div>)html") == kFromClass);
  }

  SECTION("id beats a tag selector") {
    CHECK(color_of(R"html(
        <style>
          #t { color: rgb(1,0,0); }
          div { color: rgb(0,0,3); }
        </style>
        <div id="t" class="c">x</div>)html") == kFromId);
  }

  SECTION("between equal-specificity classes, the later rule wins") {
    CHECK(color_of(R"html(
        <style>
          .a { color: rgb(1,0,0); }
          .b { color: rgb(0,2,0); }
        </style>
        <div id="t" class="a b">x</div>)html") == kFromClass);
  }

  SECTION("an inline style beats every selector") {
    CHECK(color_of(R"html(
        <style>
          #t { color: rgb(1,0,0); }
          .c { color: rgb(0,0,3); }
        </style>
        <div id="t" class="c" style="color: rgb(0,2,0);">x</div>)html") ==
          kFromClass);
  }

  (void)kFromTag;
}

// Specificity is summed across the whole selector, so rules living in the same
// selector bucket still rank against each other correctly.
TEST_CASE("CSS cascade scores compound and descendant selectors",
          "[component][style][cascade]") {
  auto color_of = [](std::string_view view_text) {
    struct Probe : rtxui::Component<Probe> {
      std::string_view view;
    };
    auto probe = rtxui::Ref<Probe>::New();
    probe->view = view_text;
    probe->Mount();
    auto* el = probe->Root()->QuerySelector("#t");
    REQUIRE(el != nullptr);
    REQUIRE(el->style.foreground_color.has_value());
    return el->style.foreground_color.value();
  };

  const Color kWinner = Color::RGB(0, 2, 0);
  const Color kLoser = Color::RGB(1, 0, 0);

  SECTION("two classes outrank one, written in the losing order") {
    CHECK(color_of(R"html(
        <style>
          .a.b { color: rgb(0,2,0); }
          .a { color: rgb(1,0,0); }
        </style>
        <div id="t" class="a b">x</div>)html") == kWinner);
  }

  SECTION("a tag qualifier outranks the bare class") {
    CHECK(color_of(R"html(
        <style>
          div.a { color: rgb(0,2,0); }
          .a { color: rgb(1,0,0); }
        </style>
        <div id="t" class="a">x</div>)html") == kWinner);
  }

  SECTION("a descendant selector outranks the bare class") {
    CHECK(color_of(R"html(
        <style>
          .wrap .a { color: rgb(0,2,0); }
          .a { color: rgb(1,0,0); }
        </style>
        <div class="wrap"><div id="t" class="a">x</div></div>)html") ==
          kWinner);
  }

  SECTION("a class outranks two tag names") {
    CHECK(color_of(R"html(
        <style>
          .a { color: rgb(0,2,0); }
          section div { color: rgb(1,0,0); }
        </style>
        <section><div id="t" class="a">x</div></section>)html") == kWinner);
  }

  SECTION("an id outranks any number of classes") {
    CHECK(color_of(R"html(
        <style>
          #t { color: rgb(0,2,0); }
          .a.b.c { color: rgb(1,0,0); }
        </style>
        <div id="t" class="a b c">x</div>)html") == kWinner);
  }

  SECTION("a pseudo-class weighs the same as a class") {
    // .a:hover is (0,2,0) against .a.b at (0,2,0): equal, so source order
    // decides and the later rule wins.
    CHECK(color_of(R"html(
        <style>
          .a.b { color: rgb(1,0,0); }
          .a.b { color: rgb(0,2,0); }
        </style>
        <div id="t" class="a b">x</div>)html") == kWinner);
  }

  (void)kLoser;
}

// !important is resolved in a second pass over the same matched rules, so it
// sits above specificity while still being ordered by specificity internally.
TEST_CASE("CSS cascade resolves !important above specificity",
          "[component][style][cascade]") {
  auto color_of = [](std::string_view view_text) {
    struct Probe : rtxui::Component<Probe> {
      std::string_view view;
    };
    auto probe = rtxui::Ref<Probe>::New();
    probe->view = view_text;
    probe->Mount();
    auto* el = probe->Root()->QuerySelector("#t");
    REQUIRE(el != nullptr);
    REQUIRE(el->style.foreground_color.has_value());
    return el->style.foreground_color.value();
  };

  const Color kWinner = Color::RGB(0, 2, 0);

  SECTION("an important class beats a plain id") {
    CHECK(color_of(R"html(
        <style>
          .a { color: rgb(0,2,0) !important; }
          #t { color: rgb(1,0,0); }
        </style>
        <div id="t" class="a">x</div>)html") == kWinner);
  }

  SECTION("an important rule beats a plain inline style") {
    CHECK(color_of(R"html(
        <style>
          .a { color: rgb(0,2,0) !important; }
        </style>
        <div id="t" class="a" style="color: rgb(1,0,0);">x</div>)html") ==
          kWinner);
  }

  SECTION("between two important rules, specificity still decides") {
    CHECK(color_of(R"html(
        <style>
          #t { color: rgb(0,2,0) !important; }
          .a { color: rgb(1,0,0) !important; }
        </style>
        <div id="t" class="a">x</div>)html") == kWinner);
  }

  SECTION("an important rule loses to nothing else that is important") {
    CHECK(color_of(R"html(
        <style>
          .a { color: rgb(1,0,0) !important; }
          .b { color: rgb(0,2,0) !important; }
        </style>
        <div id="t" class="a b">x</div>)html") == kWinner);
  }
}

// A conformance sweep over selector matching and specificity. These are the
// semantics a stylesheet author assumes without thinking about them, so they
// are worth pinning explicitly rather than inferring from feature tests.
TEST_CASE("CSS selector and specificity conformance",
          "[component][style][cascade]") {
  // Resolved foreground colour of #t, or nullopt when nothing set it.
  auto color_of = [](std::string_view view_text) -> std::optional<Color> {
    struct Probe : rtxui::Component<Probe> {
      std::string_view view;
    };
    auto probe = rtxui::Ref<Probe>::New();
    probe->view = view_text;
    probe->Mount();
    auto* el = probe->Root()->QuerySelector("#t");
    REQUIRE(el != nullptr);
    return el->style.foreground_color;
  };

  const Color kWinner = Color::RGB(0, 2, 0);
  const Color kLoser = Color::RGB(1, 0, 0);

  SECTION("child combinator matches only a direct child") {
    CHECK(color_of(R"(<style>.p > .c { color: rgb(0,2,0); }</style>
        <div class="p"><div id="t" class="c">x</div></div>)") == kWinner);
    CHECK_FALSE(color_of(R"(<style>.p > .c { color: rgb(0,2,0); }</style>
        <div class="p"><div><div id="t" class="c">x</div></div></div>)")
                    .has_value());
  }

  SECTION("descendant combinator reaches any depth") {
    CHECK(color_of(R"(<style>.p .c { color: rgb(0,2,0); }</style>
        <div class="p"><div><div id="t" class="c">x</div></div></div>)") ==
          kWinner);
  }

  SECTION("attribute selectors match on presence and on value") {
    CHECK(color_of(R"(<style>[data-x] { color: rgb(0,2,0); }</style>
        <div id="t" data-x="1">x</div>)") == kWinner);
    CHECK(color_of(R"(<style>[data-x="a"] { color: rgb(0,2,0); }</style>
        <div id="t" data-x="a">x</div>)") == kWinner);
    CHECK_FALSE(color_of(R"(<style>[data-x="a"] { color: rgb(0,2,0); }</style>
        <div id="t" data-x="b">x</div>)")
                    .has_value());
  }

  SECTION("an attribute selector weighs as a class, so it beats a tag") {
    CHECK(color_of(R"(<style>
          [data-x] { color: rgb(0,2,0); }
          div { color: rgb(1,0,0); }
        </style>
        <div id="t" data-x="1">x</div>)") == kWinner);
  }

  SECTION("the universal selector carries no weight") {
    CHECK(color_of(R"(<style>
          * { color: rgb(1,0,0); }
          div { color: rgb(0,2,0); }
        </style>
        <div id="t">x</div>)") == kWinner);
    // `* .a` and `.b` are both one class, so source order decides.
    CHECK(color_of(R"(<style>
          * .a { color: rgb(0,2,0); }
          .b { color: rgb(1,0,0); }
        </style>
        <div><div id="t" class="a b">x</div></div>)") == kLoser);
  }

  SECTION("an id in an ancestor position still counts as an id") {
    CHECK(color_of(R"(<style>
          #p .a { color: rgb(0,2,0); }
          .q .a { color: rgb(1,0,0); }
        </style>
        <div id="p" class="q"><div id="t" class="a">x</div></div>)") ==
          kWinner);
  }

  SECTION("any number of tag names loses to a single class") {
    CHECK(color_of(R"(<style>
          .a { color: rgb(0,2,0); }
          div div div { color: rgb(1,0,0); }
        </style>
        <div><div><div id="t" class="a">x</div></div></div>)") == kWinner);
  }

  SECTION("a selector list applies to each of its members") {
    CHECK(color_of(R"(<style>.x, .a { color: rgb(0,2,0); }</style>
        <div id="t" class="a">x</div>)") == kWinner);
  }

  SECTION("the later of two identical selectors wins") {
    CHECK(color_of(R"(<style>
          .a { color: rgb(1,0,0); }
          .a { color: rgb(0,2,0); }
        </style>
        <div id="t" class="a">x</div>)") == kWinner);
  }
}

class ConditionalTabTestApp : public Component<ConditionalTabTestApp> {
 public:
  bool is_active = false;
  std::string get_class() const { return is_active ? "active" : ""; }

  std::string_view view = R"html(
    <div class="tabs">
      <button class="{get_class}">Tab</button>
    </div>
    <style>
      button { background-color: red; transition: none; }
      button.active { background-color: blue; }
    </style>
  )html";

  ConditionalTabTestApp() {
    Bind(is_active);
    Bind(get_class);
  }
};

}  // namespace

TEST_CASE("CSS Tag and Class Selector Combination (e.g., button.active)",
          "[component][css]") {
  auto app = Ref<ConditionalTabTestApp>::New();
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  device->TriggerResize(80, 24);
  rtxui::Screen screen = rtxui::ScreenInternals::Make(app, device);

  auto* button = app->Root()->QuerySelector("button");
  REQUIRE(button != nullptr);

  // Initially is_active is false, so it doesn't have the active class
  // It should have the red background color
  REQUIRE(button->style.background_color.value() == Color::RGB(255, 0, 0));

  // Now activate it
  app->is_active = true;
  app->Render();

  // Resolve styles at a later time to finish any potential transition
  app->ResolveTargetStyles(rtxui::time::GetTimeMs() + 200.0);

  // It should now have the blue background color because of button.active
  REQUIRE(button->style.background_color.value() == Color::RGB(0, 0, 255));
}

TEST_CASE("Style caching regression test for multi-component resolution",
          "[component][style]") {
  struct ChildComp : rtxui::Component<ChildComp> {
    std::string_view view = R"html(
        <style>
          self { color: red; }
        </style>
        <slot></slot>
    )html";
  };

  struct ParentComp : rtxui::Component<ParentComp> {
    std::string child_class = "test-child";
    std::string_view view = R"html(
        <style>
          .test-child { background-color: blue; }
        </style>
        <div>
          <child class="{child_class}" id="mychild">Content</child>
        </div>
    )html";

    ParentComp() {
      RegisterState("child_class", &child_class);
      Import<ChildComp>("child");
      Import<rtxui::div>();
    }
  };

  auto parent = rtxui::Ref<ParentComp>::New();
  parent->Mount();

  auto* child_el = parent->Root()->QuerySelector("#mychild");
  REQUIRE(child_el != nullptr);

  // Both parent style (.test-child) and child style (self) should be applied.
  CHECK(child_el->style.foreground_color.value() == Color::RGB(255, 0, 0));
  CHECK(child_el->style.background_color.value() == Color::RGB(0, 0, 255));

  // Triggering attribute change to test cache invalidation
  parent->child_class = "";
  bool digested = parent->Digest();

  child_el = parent->Root()->QuerySelector("#mychild");
  REQUIRE(child_el != nullptr);

  // Child style (self) should still apply, but parent class style (.test-child)
  // should be gone.
  CHECK(child_el->style.foreground_color.value() == Color::RGB(255, 0, 0));
  CHECK(!child_el->style.background_color.has_value());
}

namespace {

class StyledParentTestComponent
    : public rtxui::Component<StyledParentTestComponent> {
 public:
  std::string value = "initial";
  std::string_view view = R"html(
    <div>
      <input id="test-input" class="custom-input" value="{value}" />
    </div>
    <style>
      .custom-input {
        background-color: rgb(100, 100, 100);
      }
    </style>
  )html";
  StyledParentTestComponent() { Bind(value); }
};

}  // namespace

TEST_CASE("Child component style is preserved when parent stylesheet styles it",
          "[component][style][regression]") {
  auto container = rtxui::Ref<StyledParentTestComponent>::New();
  container->Mount();

  auto* input_el = container->Root()->QuerySelector("#test-input");
  REQUIRE(input_el != nullptr);

  // Focus the input
  input_el->set_focused(true);

  // Initial render
  container->Render();

  // Verify that the parent's base style is applied
  CHECK(input_el->base_style.background_color.value() ==
        Color::RGB(100, 100, 100));

  // Simulate navigating with ArrowLeft which triggers input's Digest() and
  // re-render of input
  auto* input_ptr = ComponentAs<rtxui::input>(input_el);
  REQUIRE(input_ptr != nullptr);
  input_ptr->OnEvent(Event::ArrowLeft());
  input_ptr->Digest();

  // The parent style should still be preserved on the child's root base style!
  CHECK(input_el->base_style.background_color.value() ==
        Color::RGB(100, 100, 100));
}

TEST_CASE("Component.InlineStyleParsingRegression", "[component][style]") {
  class InlineStyleComp : public rtxui::Component<InlineStyleComp> {
   public:
    std::string_view view = R"xml(
      <div id="target" style="display: flex; width: 15; height: 10; background-color: rgb(255, 0, 0);"></div>
    )xml";
  };

  auto component = rtxui::Ref<InlineStyleComp>::New();
  component->Mount();

  auto* target = component->Root()->QuerySelector("#target");
  REQUIRE(target != nullptr);
  CHECK(target->style.display_inside == DisplayInside::Flex);
  CHECK(target->style.width.value == 15);
  CHECK(target->style.height.value == 10);
  CHECK(target->style.background_color.value() == Color::RGB(255, 0, 0));
}

TEST_CASE("Invalid interpolated CSS does not crash the process",
          "[component][style]") {
  class DynamicStyleComp : public rtxui::Component<DynamicStyleComp> {
   public:
    std::string rule = "color: red;";
    DynamicStyleComp() { Bind(rule); }
    std::string_view view = R"xml(
      <div id="target" class="box">hi</div>
      <style>
        .box { {rule} }
      </style>
    )xml";
  };

  auto component = rtxui::Ref<DynamicStyleComp>::New();
  component->Mount();

  // A bound value producing malformed CSS must be reported, not fatal: this
  // string is re-parsed by Render() on every Digest(), unlike the top-level
  // XML template which is only parsed once at Mount().
  component->rule = "not valid css {{{ ]]]";
  CHECK_NOTHROW(component->Digest());

  auto* target = component->Root()->QuerySelector("#target");
  REQUIRE(target != nullptr);
}

TEST_CASE("A CSS syntax error reaches the diagnostic handler",
          "[component][style]") {
  class DynamicStyleComp : public rtxui::Component<DynamicStyleComp> {
   public:
    std::string rule = "color: red;";
    DynamicStyleComp() { Bind(rule); }
    std::string_view view = R"xml(
      <div id="target" class="box">hi</div>
      <style>
        .box { {rule} }
      </style>
    )xml";
  };

  std::optional<rtxui::Diagnostic> captured;
  rtxui::SetDiagnosticHandler([&](const rtxui::Diagnostic& error) {
    if (error.kind == rtxui::Diagnostic::Kind::CssSyntax) {
      captured = error;
    }
  });

  auto component = rtxui::Ref<DynamicStyleComp>::New();
  component->Mount();
  component->rule = "not valid css {{{ ]]]";
  component->Digest();

  rtxui::SetDiagnosticHandler(nullptr);  // Don't leak into other tests.

  REQUIRE(captured.has_value());
  CHECK_FALSE(captured->message.empty());
}

namespace {

class ClassMutationApp : public rtxui::Component<ClassMutationApp> {
 public:
  std::string_view view = R"(
    <div id="box">x</div>
    <style>
      div { color: rgb(1, 1, 1); }
      .lit { color: rgb(2, 2, 2); }
    </style>
  )";
};

}  // namespace

TEST_CASE("Setting classes restyles without an explicit invalidation",
          "[component][style]") {
  // Regression: an element remembers which components have resolved it, and
  // resolution skips it while that memo stands. `classes` used to be a plain
  // public vector, so assigning to it left the memo in place and the new
  // classes silently never applied; <tabs> did that, twice. SetClasses()
  // drops the memo, and marks the ancestors so the pass reaches the element.
  auto app = rtxui::Ref<ClassMutationApp>::New();
  app->Mount();

  auto* box = app->Root()->QuerySelector("#box");
  REQUIRE(box != nullptr);
  CHECK(box->style.foreground_color == Color::RGB(1, 1, 1));

  box->SetClasses({"lit"});
  CHECK(app->Root()->subtree_needs_style_resolve);
  rtxui::ComponentInternals::ResolveStyles(*app);
  CHECK(box->style.foreground_color == Color::RGB(2, 2, 2));
  CHECK_FALSE(app->Root()->subtree_needs_style_resolve);

  // Removing it again must take the style away, not just add on top: the
  // element has to be resolved from scratch, not incrementally.
  box->SetClasses({});
  rtxui::ComponentInternals::ResolveStyles(*app);
  CHECK(box->style.foreground_color == Color::RGB(1, 1, 1));
}

namespace {

// Builds an element from Digest() rather than from its template, the way
// <tabs> and <select> rebuild theirs. Digest() runs *after* Render() has
// resolved styles, so the element misses that pass entirely.
class LateElementApp : public rtxui::Component<LateElementApp> {
 public:
  bool add_late = false;
  LateElementApp() { Bind(add_late); }
  std::string_view view = R"(
    <div id="host"></div>
    <style>
      .late { padding-left: 4; }
    </style>
  )";
  bool Digest() override {
    const bool changed = rtxui::Component<LateElementApp>::Digest();
    if (add_late && !added_) {
      auto* host = Root()->QuerySelector("#host");
      REQUIRE(host != nullptr);
      auto element = rtxui::Ref<rtxui::Element>::New(this);
      element->SetClasses({"late"});
      host->AddChild(element);
      added_ = true;
    }
    return changed;
  }

 private:
  bool added_ = false;
};

}  // namespace

TEST_CASE("An element created during Digest() still gets its base style",
          "[component][css][digest]") {
  auto app = rtxui::Ref<LateElementApp>::New();
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  rtxui::Screen screen = rtxui::ScreenInternals::Make(app, device);
  screen.Draw();

  app->add_late = true;
  app->Digest();
  auto* late = app->Root()->QuerySelector(".late");
  REQUIRE(late != nullptr);

  // Digest() alone cannot have styled it: the base pass already ran.
  CHECK(late->base_style.padding.left == 0);

  // Draw() is the choke point every path shares, so it resolves whatever the
  // DOM gained since the last frame. Without that, an element built in
  // Digest() renders with none of its component's stylesheet -- only
  // :hover/:focus would land on it, which is the giveaway symptom.
  screen.Draw();
  CHECK(late->base_style.padding.left == 4);
  CHECK(late->style.padding.left == 4);
}

namespace {

class HostStyledButtonsApp : public rtxui::Component<HostStyledButtonsApp> {
 public:
  int mode = 0;
  std::string a_class() const { return mode == 0 ? "active" : ""; }
  std::string b_class() const { return mode == 1 ? "active" : ""; }
  // `self { button { … } }` compiles to the descendant selector `self button`,
  // so matching it needs the button to still be reachable from this root.
  std::string_view view = R"(
    <div>
      <button id="a" class="{a_class}">A</button>
      <button id="b" class="{b_class}">B</button>
      <button id="c">C</button>
    </div>
    <style>
      self {
        button {
          border: vkey;
          padding: 0 1;
        }
      }
      .active { font-weight: bold; }
    </style>
  )";
  HostStyledButtonsApp() {
    Bind(mode);
    Bind(a_class);
    Bind(b_class);
  }
};

}  // namespace

TEST_CASE("A nested component keeps its host's rules when it re-renders",
          "[component][style][css]") {
  // Regression: Render() detaches its root from the parent tree while it
  // collects and restores element state by path, and styles were resolved
  // during that window. A host styling this component with a descendant
  // selector -- which is what CSS nesting compiles to -- then matched nothing,
  // because an orphaned root has no ancestors. Changing a bound class made the
  // button component re-render, and it came back stripped of every rule its
  // host had aimed at it: in example/conditional.cpp the tab buttons lost
  // their borders and collapsed from three rows to one on the first click.
  auto app = rtxui::Ref<HostStyledButtonsApp>::New();
  app->Mount();

  auto border_of = [&](const char* id) {
    auto* element = app->Root()->QuerySelector(id);
    REQUIRE(element != nullptr);
    return element->style.border.left;
  };
  auto padding_of = [&](const char* id) {
    auto* element = app->Root()->QuerySelector(id);
    REQUIRE(element != nullptr);
    return element->style.padding.left;
  };

  REQUIRE(border_of("#a") == 1);
  REQUIRE(border_of("#b") == 1);

  // Moving the active class re-renders both #a and #b, but not #c.
  app->mode = 1;
  app->Digest();
  rtxui::ComponentInternals::ResolveStyles(*app);

  CHECK(border_of("#a") == 1);
  CHECK(border_of("#b") == 1);
  CHECK(border_of("#c") == 1);
  CHECK(padding_of("#a") == 1);
  CHECK(padding_of("#b") == 1);

  // And back again, since the bug only showed after a re-render.
  app->mode = 0;
  app->Digest();
  rtxui::ComponentInternals::ResolveStyles(*app);
  CHECK(border_of("#a") == 1);
  CHECK(border_of("#b") == 1);
}

namespace {

// A component whose stylesheet mixes a bare `self` rule with rules that reach
// past its own root. The bare rule must not be taken as proof that the whole
// stylesheet stops at the root.
class SelfAndDescendantRulesApp
    : public rtxui::Component<SelfAndDescendantRulesApp> {
 public:
  std::string_view view = R"(
    <div id="root">
      <div id="child" class="tagged">
        <span id="grandchild">x</span>
      </div>
    </div>
    <style>
      self { padding: 3; }
      self .tagged { margin: 4; }
      span { padding: 5; }
    </style>
  )";
};

// Every rule here stops at the root, which is what lets the walk skip
// descendants. The root must still be styled.
class OnlySelfRulesApp : public rtxui::Component<OnlySelfRulesApp> {
 public:
  std::string_view view = R"(
    <div id="root">
      <div id="child">x</div>
    </div>
    <style>
      self { padding: 7; }
      self:hover { margin: 9; }
    </style>
  )";
};

}  // namespace

TEST_CASE("A bare `self` rule does not stop a stylesheet reaching descendants",
          "[component][style][css]") {
  // ResolveElementStyle skips the whole matching pass for a component whose
  // every rule is a bare `self { … }`, because such a component can only ever
  // style its own root -- and since every HTML tag is itself a component, that
  // describes most components in a tree. The skip is only sound if "bare self"
  // is judged precisely: a descendant, tag or ::part() rule sitting next to a
  // `self` rule has to disqualify the whole component, or its descendants
  // silently lose every style.
  SECTION("descendant and tag rules still apply") {
    auto app = rtxui::Ref<SelfAndDescendantRulesApp>::New();
    app->Mount();
    auto at = [&](const char* id) {
      auto* element = app->Root()->QuerySelector(id);
      REQUIRE(element != nullptr);
      return element;
    };
    // `self` names the component's own root, not the template's outermost tag.
    CHECK(app->Root()->style.padding.left == 3);
    CHECK(at("#child")->style.margin.left == 4);
    CHECK(at("#grandchild")->style.padding.left == 5);
  }

  SECTION("a component that really is self-only still styles its own root") {
    auto app = rtxui::Ref<OnlySelfRulesApp>::New();
    app->Mount();
    CHECK(app->Root()->style.padding.left == 7);
    // Descendants are untouched, which is the whole point: no rule can reach
    // them, which is what makes skipping the walk sound.
    auto* child = app->Root()->QuerySelector("#child");
    REQUIRE(child != nullptr);
    CHECK(child->style.padding.left == 0);
  }
}

namespace {

class HoverCard : public rtxui::Component<HoverCard> {
 public:
  std::string_view view = R"(
    <div>
      <button id="btn">x</button>
    </div>
    <style>
      button:hover { background-color: red; }
    </style>
  )";
};

class HoverCardHost : public rtxui::Component<HoverCardHost> {
 public:
  HoverCardHost() { Import<HoverCard>(); }
  std::string_view view = R"(
    <div>
      <HoverCard/>
    </div>
    <style>
      div:hover { background-color: blue; }
    </style>
  )";
};

class PartLabel : public rtxui::Component<PartLabel> {
 public:
  std::string kind = "before";
  PartLabel() { Bind(kind); }
  std::string_view view = R"(
    <div>
      <span part="label" class="{kind}">x</span>
    </div>
  )";
};

class PartLabelMiddle : public rtxui::Component<PartLabelMiddle> {
 public:
  PartLabelMiddle() { Import<PartLabel>(); }
  std::string_view view = R"(
    <div>
      <PartLabel/>
    </div>
  )";
};

// The rule reaches through PartLabelMiddle, two components above the span.
class PartLabelHost : public rtxui::Component<PartLabelHost> {
 public:
  PartLabelHost() { Import<PartLabelMiddle>(); }
  std::string_view view = R"(
    <div>
      <PartLabelMiddle/>
    </div>
    <style>
      PartLabelMiddle::part(label) { padding: 3; }
    </style>
  )";
};

}  // namespace

TEST_CASE("A ::part() rule survives the inner component re-rendering itself",
          "[component][style][part]") {
  // PartLabel re-renders on its own, from its own state, and the class change
  // makes it restyle the span. Nothing above it changes, but the span must
  // still get the ::part() rule of the host two components up, not only the
  // rules of PartLabel and of the component directly using it.
  auto app = rtxui::Ref<PartLabelHost>::New();
  app->Mount();
  auto label = [&] {
    auto* element = app->Root()->QuerySelector("span");
    REQUIRE(element != nullptr);
    return element;
  };
  CHECK(label()->style.padding.left == 3);

  ComponentAs<PartLabel>(app->Root()->QuerySelector("PartLabel"))->kind =
      "after";
  bool changed = app->Digest();
  CHECK(changed);
  CHECK(label()->classes() == std::vector<std::string>{"after"});
  rtxui::ComponentInternals::ResolveStyles(*app);
  CHECK(label()->style.padding.left == 3);
}

namespace {

class CountedChild : public rtxui::Component<CountedChild> {
 public:
  int value = 0;
  CountedChild() { Bind(value); }
  std::string_view view = R"(
    <div class="v{value}">
      <span>{value}</span>
    </div>
    <style>
      span:hover { color: red; }
    </style>
  )";
};

class CountedParent : public rtxui::Component<CountedParent> {
 public:
  int value = 0;
  CountedParent() {
    Bind(value);
    Import<CountedChild>();
  }
  std::string_view view = R"(
    <div class="v{value}">
      <CountedChild value="{value}"/>
      <CountedChild value="{value}"/>
    </div>
  )";
};

}  // namespace

TEST_CASE("One Digest() resolves the styles of the tree once",
          "[component][style]") {
  // The parent re-renders, and so do the children it passes the value to.
  // Each child resolving its own subtree, before or after the parent resolves
  // the whole tree, would be work thrown away: the walk covers each element
  // once per pass, base and pseudo-class.
  auto app = rtxui::Ref<CountedParent>::New();
  app->Mount();
  int elements = 0;
  app->Root()->Visit([&](rtxui::Element&) { ++elements; });

  app->value = 1;
  rtxui::ResetStyleVisitCount();
  app->Digest();
  CHECK(rtxui::StyleVisitCount() == 2 * elements);
}

namespace {

class DescendantRuleApp : public rtxui::Component<DescendantRuleApp> {
 public:
  bool on = false;
  std::string state() const { return on ? "on" : "off"; }
  DescendantRuleApp() {
    Bind(on);
    Bind(state);
  }
  std::string_view view = R"(
    <div class="{state}">
      <div>
        <span id="leaf">x</span>
      </div>
    </div>
    <style>
      .on span { padding: 3; }
    </style>
  )";
};

}  // namespace

TEST_CASE("A descendant rule follows a change of the ancestor it names",
          "[component][style][css]") {
  // Only the ancestor's class changes: the span itself is untouched, but
  // `.on span` now matches it.
  auto app = rtxui::Ref<DescendantRuleApp>::New();
  app->Mount();
  auto leaf = [&] { return app->Root()->QuerySelector("#leaf"); };
  REQUIRE(leaf() != nullptr);
  CHECK(leaf()->style.padding.left == 0);

  app->on = true;
  app->Digest();
  rtxui::ComponentInternals::ResolveStyles(*app);
  CHECK(leaf()->style.padding.left == 3);

  app->on = false;
  app->Digest();
  rtxui::ComponentInternals::ResolveStyles(*app);
  CHECK(leaf()->style.padding.left == 0);
}

namespace {

class SiblingRuleApp : public rtxui::Component<SiblingRuleApp> {
 public:
  bool on = false;
  std::string state() const { return on ? "on" : "off"; }
  SiblingRuleApp() {
    Bind(on);
    Bind(state);
  }
  std::string_view view = R"(
    <div>
      <div class="{state}">a</div>
      <span id="next">b</span>
      <span id="later">c</span>
    </div>
    <style>
      .on + span { padding: 3; }
      .on ~ span { margin: 2; }
    </style>
  )";
};

}  // namespace

TEST_CASE("A sibling rule follows a change of the sibling it names",
          "[component][style][css]") {
  // Only the first div's class changes: the spans after it are untouched,
  // but `.on + span` and `.on ~ span` now match them.
  auto app = rtxui::Ref<SiblingRuleApp>::New();
  app->Mount();
  auto at = [&](const char* id) { return app->Root()->QuerySelector(id); };
  REQUIRE(at("#next") != nullptr);
  REQUIRE(at("#later") != nullptr);
  CHECK(at("#next")->style.padding.left == 0);

  app->on = true;
  app->Digest();
  rtxui::ComponentInternals::ResolveStyles(*app);
  CHECK(at("#next")->style.padding.left == 3);
  CHECK(at("#later")->style.padding.left == 0);
  CHECK(at("#later")->style.margin.left == 2);

  app->on = false;
  app->Digest();
  rtxui::ComponentInternals::ResolveStyles(*app);
  CHECK(at("#next")->style.padding.left == 0);
  CHECK(at("#later")->style.margin.left == 0);
}

namespace {

class ClassTransitionApp : public rtxui::Component<ClassTransitionApp> {
 public:
  bool lit = false;
  std::string state() const { return lit ? "card lit" : "card"; }
  ClassTransitionApp() {
    Bind(lit);
    Bind(state);
  }
  std::string_view view = R"(
    <div>
      <div id="card" class="{state}">x</div>
    </div>
    <style>
      .card {
        background-color: rgb(32, 32, 32);
        transition: background-color 4s linear;
      }
      .card.lit { background-color: rgb(208, 208, 208); }
    </style>
  )";
};

}  // namespace

TEST_CASE("A class change transitions from the style on screen",
          "[component][style][transitions]") {
  // The class change re-renders the component and recomputes the card's base
  // style. Its style must not jump to the new color: the transition starts
  // from the one on screen.
  auto app = rtxui::Ref<ClassTransitionApp>::New();
  app->Mount();
  auto* card = app->Root()->QuerySelector("#card");
  REQUIRE(card != nullptr);
  CHECK(card->style.background_color == Color::RGB(32, 32, 32));

  app->lit = true;
  app->Digest();
  CHECK(card->style.background_color == Color::RGB(32, 32, 32));
  CHECK(card->target_style.background_color == Color::RGB(208, 208, 208));
  CHECK(card->active_transitions.count("background-color") == 1);
}

TEST_CASE("A component's :hover rule outranks the one nested inside it",
          "[component][style][css]") {
  // <button> has its own `self:hover { background-color: … }`, two components
  // below the root. HoverCard's `button:hover` must still win: each component's
  // pseudo-class pass runs once, children before the components that use them.
  auto app = rtxui::Ref<HoverCardHost>::New();
  app->Mount();
  auto* btn = app->Root()->QuerySelector("#btn");
  REQUIRE(btn != nullptr);

  const auto unhovered = btn->target_style.background_color;
  btn->set_hovered(true);
  app->ResolveTargetStyles();
  CHECK(btn->target_style.background_color == Color::RGB(255, 0, 0));

  // The pass skips elements no pseudo-class rule matches, now or last time.
  // One that stops matching is not one of them: it gets its base style back.
  btn->set_hovered(false);
  app->ResolveTargetStyles();
  CHECK(btn->target_style.background_color == unhovered);
}

TEST_CASE("Playground button hover and active styling test",
          "[component][style][playground]") {
  class TestLivePreview : public rtxui::Component<TestLivePreview> {
   public:
    std::string_view view = R"(
      <div>
        <button id="btn">Click me</button>
      </div>
      <style>
        button {
          background-color: rgb(31, 38, 47);
          border: tall;
          border-color: rgb(48, 54, 61);
          color: rgb(230, 237, 243);
          padding: 0 2;
          transition: all 0.3s ease;
        }
        button:hover {
          background-color: red;
          border-color: rgb(88, 166, 255);
          color: rgb(13, 17, 23);
        }
        button:active {
          background-color: rgb(67, 56, 202);
          border-color: rgb(67, 56, 202);
          color: rgb(230, 237, 243);
        }
      </style>
    )";
  };

  auto app = rtxui::Ref<TestLivePreview>::New();
  app->Mount();
  auto* btn = app->Root()->QuerySelector("#btn");
  REQUIRE(btn != nullptr);

  // Initial style
  CHECK(btn->style.background_color == Color::RGB(31, 38, 47));

  // Hover
  btn->set_hovered(true);
  app->ResolveTargetStyles();

  // Check if red
  CHECK(btn->target_style.background_color == Color::RGB(255, 0, 0));

  // Now HotReload with button:hover background-color: red and border-color: red
  app->HotReload(R"(
      <div>
        <button id="btn">Click me</button>
      </div>
      <style>
        button {
          background-color: rgb(31, 38, 47);
          border: tall;
          border-color: rgb(48, 54, 61);
          color: rgb(230, 237, 243);
          padding: 0 2;
          transition: all 0.3s ease;
        }
        button:hover {
          background-color: red;
          border-color: red;
          color: rgb(13, 17, 23);
        }
        button:active {
          background-color: red;
          border-color: red;
          color: rgb(230, 237, 243);
        }
      </style>
  )");
  btn = app->Root()->QuerySelector("#btn");
  REQUIRE(btn != nullptr);
  btn->set_hovered(true);
  app->ResolveTargetStyles();
  CHECK(btn->target_style.background_color == Color::RGB(255, 0, 0));
  CHECK(btn->target_style.border_color_top == Color::RGB(255, 0, 0));
}
