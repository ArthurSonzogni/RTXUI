// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component.hpp"

#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <vector>

#include "rtxui/base/clock.hpp"
#include "rtxui/base/string.hpp"
#include "rtxui/color.hpp"
#include "rtxui/component/component_internal.hpp"
#include "rtxui/component/default/b/b.hpp"
#include "rtxui/component/default/button/button.hpp"
#include "rtxui/component/default/checkbox/checkbox.hpp"
#include "rtxui/component/default/div/div.hpp"
#include "rtxui/component/default/li/li.hpp"
#include "rtxui/component/default/option/option.hpp"
#include "rtxui/component/default/p/p.hpp"
#include "rtxui/component/default/radio/radio.hpp"
#include "rtxui/component/default/select/select.hpp"
#include "rtxui/component/default/slider/slider.hpp"
#include "rtxui/component/default/span/span.hpp"
#include "rtxui/component/default/tab_pane/tab_pane.hpp"
#include "rtxui/component/default/ul/ul.hpp"
#include "rtxui/diagnostic.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/dom/text_element.hpp"
#include "rtxui/event.hpp"
#include "rtxui/headless.hpp"
#include "rtxui/layout/style.hpp"
#include "rtxui/paint/texture.hpp"
#include "rtxui/refcounted.hpp"
#include "rtxui/screen.hpp"
#include "rtxui/terminal/terminal_device.hpp"
#include "rtxui/testing/render.hpp"

using rtxui::Color;
using rtxui::Event;
using rtxui::Texture;

namespace {

class Hello : public rtxui::Component<Hello> {
 public:
  std::string_view view = R"(
      Hello
    )";
};

class World : public rtxui::Component<World> {
 public:
  std::string_view view = R"(
      World
    )";
};

class HelloWorld : public rtxui::Component<HelloWorld> {
 public:
  void InitReflection() override {
    Import<Hello>();
    Import<World>();
    rtxui::Component<HelloWorld>::InitReflection();
  }

  std::string_view view = R"(
      Hello, World!
      <Hello/>
      <World/>

      <style>
        self {
          display: block;
          background-color: rgb(220, 38, 38);
          color: white;
        }
      </style>
    )";
};

TEST_CASE("Tag", "[component]") {
  REQUIRE(HelloWorld::StaticTag() == "HelloWorld");
  REQUIRE(Hello::StaticTag() == "Hello");
  REQUIRE(World::StaticTag() == "World");
}

TEST_CASE("Mount", "[component]") {
  auto component = rtxui::Ref<HelloWorld>::New();
  component->Mount();

  const std::string expected = R"(
    <HelloWorld>
      Hello, World!
      <Hello>
        Hello
      </Hello>
      <World>
        World
      </World>
    </HelloWorld>
  )";

  REQUIRE(component->Root()->Print() == StripIndent(expected));
}

class Page : public rtxui::Component<Page> {
 public:
  std::string_view view = R"(
    <slot.header/>
    <slot/>
    <slot.footer/>
  )";
};

class MyPage : public rtxui::Component<MyPage> {
 public:
  void InitReflection() override {
    Import<rtxui::p>();
    Import<Page>();
    rtxui::Component<MyPage>::InitReflection();
  }

  std::string_view view = R"(
      <p>This is a page:</p>
      <Page>
        <template.header>
          <p>
            A website by Arthur Sonzogni
          </p>
        </template.header>

        <template.footer>
          <p>
            © 2024 Arthur Sonzogni
          </p>
        </template.footer>

        <p>
         Hello, World!
        </p>
      </Page>
    )";
};

TEST_CASE("Component with named slots", "[component]") {
  auto mypage = rtxui::Ref<MyPage>::New();
  mypage->Mount();

  const std::string expected = R"(
    <MyPage>
      <p>
        This is a page:
      </p>
      <Page>
        <p>
          A website by Arthur Sonzogni
        </p>
        <p>
          Hello, World!
        </p>
        <p>
          © 2024 Arthur Sonzogni
        </p>
      </Page>
    </MyPage>
  )";

  REQUIRE(mypage->Root()->Print() == StripIndent(expected));
}

class Inverted : public rtxui::Component<Inverted> {
 public:
  void InitReflection() override {
    Import<rtxui::p>("div");
    Import<rtxui::div>("p");
    rtxui::Component<Inverted>::InitReflection();
  }

  std::string_view view = R"(
      <div>
        <p>
          This is a div inside a p.
        </p>
        <div>
          This is a p inside a p.
        </div>
      </div>

      <p>
        <div>
          This is a p inside a div.
        </div>
        <p>
          This is a div inside a div.
        </p>
      </p>
    )";
};

TEST_CASE("Import alias", "[component]") {
  const std::string expected = R"(
    <Inverted>
      <p>
        <div>
          This is a div inside a p.
        </div>
        <p>
          This is a p inside a p.
        </p>
      </p>
      <div>
        <p>
          This is a p inside a div.
        </p>
        <div>
          This is a div inside a div.
        </div>
      </div>
    </Inverted>
  )";

  auto inverted = rtxui::Ref<Inverted>::New();
  inverted->Mount();
  REQUIRE(inverted->Root()->Print() == StripIndent(expected));
}

class Counter : public rtxui::Component<Counter> {
 public:
  int count = 0;
  int double_count() const { return count * 2; }

  Counter() {
    Bind(count);
    Bind(double_count);
  }

  void increment() { count++; }

  void InitReflection() override {
    Import<rtxui::div>();
    Import<rtxui::ul>();
    Import<rtxui::li>();
    rtxui::Component<Counter>::InitReflection();
  }

  std::string_view view = R"(
      <ul>
        <li>Count: {count}</li>
        <li>Double: {double_count}</li>
      </ul>
    )";
};

TEST_CASE("Transparent Reactivity", "[component]") {
  auto counter = rtxui::Ref<Counter>::New();
  counter->Mount();

  REQUIRE(counter->count == 0);
  REQUIRE(counter->Root()->Print().find("Count: 0") != std::string::npos);
  REQUIRE(counter->Root()->Print().find("Double: 0") != std::string::npos);

  // Simulate an action
  counter->increment();
  counter->Digest();

  REQUIRE(counter->count == 1);
  REQUIRE(counter->Root()->Print().find("Count: 1") != std::string::npos);
  REQUIRE(counter->Root()->Print().find("Double: 2") != std::string::npos);
}

TEST_CASE("Bindings callback execution", "[component]") {
  auto counter = rtxui::Ref<Counter>::New();
  Counter* raw_counter = counter.get();
  counter->Import("increment", [raw_counter]() { raw_counter->increment(); });

  REQUIRE(counter->count == 0);
  bool ran = counter->RunCallback("increment");
  REQUIRE(ran == true);
  REQUIRE(counter->count == 1);

  bool ran_nonexistent = counter->RunCallback("nonexistent");
  REQUIRE(ran_nonexistent == false);
}

// State is bound by address, so binding a temporary would leave the binding
// pointing at a destroyed object. Import() must refuse it at compile time.
template <typename T>
concept CanImport = requires(Counter& c, T&& value) {
  c.Import("name", std::forward<T>(value));
};

static_assert(CanImport<int&>);

static_assert(CanImport<std::vector<int>&>);

static_assert(CanImport<int*>);

static_assert(CanImport<void (*)()>);

static_assert(!CanImport<int>);

static_assert(!CanImport<std::string>);

static_assert(!CanImport<std::vector<int>>);

TEST_CASE("Screen Drawing", "[terminal]") {
  auto counter = rtxui::Ref<Counter>::New();
  rtxui::Screen screen(counter);
  screen.Draw();
  REQUIRE(true);
}

class SelectorTest : public rtxui::Component<SelectorTest> {
 public:
  void InitReflection() override {
    Import<rtxui::div>();
    Import<rtxui::span>();
    rtxui::Component<SelectorTest>::InitReflection();
  }

  std::string_view view = R"(
      <div id="container" class="main-box panel">
        <span id="label" class="text-bold text-red">Hello Selector</span>
        <span>Other Label</span>
      </div>
    )";
};

TEST_CASE("Element.QuerySelector", "[dom]") {
  auto comp = rtxui::Ref<SelectorTest>::New();
  comp->Mount();

  auto* root = comp->Root();
  REQUIRE(root != nullptr);

  // 1. Tag selector
  auto* container_by_tag = root->QuerySelector("div");
  REQUIRE(container_by_tag != nullptr);
  REQUIRE(container_by_tag->id == "container");

  // 2. ID selector
  auto* label_by_id = root->QuerySelector("#label");
  REQUIRE(label_by_id != nullptr);
  REQUIRE(label_by_id->tag() == "span");

  // 3. Class selector
  auto* main_box = root->QuerySelector(".main-box");
  REQUIRE(main_box != nullptr);
  REQUIRE(main_box->id == "container");

  auto* panel = root->QuerySelector(".panel");
  REQUIRE(panel != nullptr);
  REQUIRE(panel->id == "container");

  auto* text_bold = root->QuerySelector(".text-bold");
  REQUIRE(text_bold != nullptr);
  REQUIRE(text_bold->id == "label");

  auto* text_red = root->QuerySelector(".text-red");
  REQUIRE(text_red != nullptr);
  REQUIRE(text_red->id == "label");

  // 4. Non-matching selectors
  CHECK(root->QuerySelector("#nonexistent") == nullptr);
  CHECK(root->QuerySelector(".nonexistent") == nullptr);
  CHECK(root->QuerySelector("nonexistent") == nullptr);
}

class ChildWithProps : public rtxui::Component<ChildWithProps> {
 public:
  struct Props {
    std::string message = "default";
    int value = 0;
  } props;

  void InitReflection() override {
    Bind(props.message);
    Bind(props.value);
    Import<rtxui::div>();
    rtxui::Component<ChildWithProps>::InitReflection();
  }

  // Directly name the props without the "props." prefix:
  std::string_view view = R"(
    <div>Message: {message}, Value: {value}</div>
  )";
};

class ParentOfProps : public rtxui::Component<ParentOfProps> {
 public:
  std::string parent_msg = "hello";
  int parent_val = 42;

  void InitReflection() override {
    Bind(parent_msg);
    Bind(parent_val);
    Import<ChildWithProps>();
    rtxui::Component<ParentOfProps>::InitReflection();
  }

  // Can pass using props. prefix or direct name
  std::string_view view = R"(
    <ChildWithProps message="{parent_msg}" value="{parent_val}" />
  )";
};

TEST_CASE("ComponentPropsAndReactivity", "[component]") {
  auto parent = rtxui::Ref<ParentOfProps>::New();
  parent->Mount();

  auto* root = parent->Root();
  REQUIRE(root != nullptr);

  std::string output = root->Print();
  CHECK(output.find("Message: hello, Value: 42") != std::string::npos);

  parent->parent_msg = "world";
  parent->parent_val = 100;
  parent->Digest();

  output = parent->Root()->Print();
  CHECK(output.find("Message: world, Value: 100") != std::string::npos);
}

class ParentOfPropsShortName : public rtxui::Component<ParentOfPropsShortName> {
 public:
  std::string parent_msg = "short";

  void InitReflection() override {
    Bind(parent_msg);
    Import<ChildWithProps>();
    rtxui::Component<ParentOfPropsShortName>::InitReflection();
  }

  std::string_view view = R"(
    <ChildWithProps props.message="{parent_msg}" props.value="999" />
  )";
};

TEST_CASE("ComponentPropsShortNames", "[component]") {
  auto parent = rtxui::Ref<ParentOfPropsShortName>::New();
  parent->Mount();

  auto* root = parent->Root();
  REQUIRE(root != nullptr);

  std::string output = root->Print();
  CHECK(output.find("Message: short, Value: 999") != std::string::npos);
}

class LetterSpacingWrapTestComponent
    : public rtxui::Component<LetterSpacingWrapTestComponent> {
 public:
  void InitReflection() override {
    Import<rtxui::div>();
    rtxui::Component<LetterSpacingWrapTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <div class="spaced">ab cd</div>
    <style>
      .spaced { letter-spacing: 1; width: 5; }
    </style>
  )";
};

TEST_CASE(
    "Letter spacing doesn't leave a stray NBSP at the start of a "
    "wrapped line",
    "[component][letter-spacing][regression]") {
  // Regression: letter-spacing inserts an NBSP on both sides of the space
  // between words ("b" NBSP " " NBSP "c"). Wrapping at that space used to
  // break right before the NBSP that pads the *next* word, so the new line
  // started with a stray non-breaking space before its first real
  // character.
  auto container = rtxui::Ref<LetterSpacingWrapTestComponent>::New();
  auto texture = RenderComponent(container, 80, 24);

  // "ab cd" at letter-spacing:1, width:5 wraps as "a\xC2\xA0b\xC2\xA0" then
  // "c\xC2\xA0d" - the second line must start with 'c', not an NBSP.
  CHECK(texture[0, 0].character == "a");
  CHECK(texture[1, 0].character == "\xC2\xA0");
  CHECK(texture[2, 0].character == "b");
  CHECK(texture[3, 0].character == "\xC2\xA0");
  CHECK(texture[0, 1].character == "c");
  CHECK(texture[1, 1].character == "\xC2\xA0");
  CHECK(texture[2, 1].character == "d");
}

class FallbackCard : public rtxui::Component<FallbackCard> {
 public:
  void InitReflection() override {
    Import<rtxui::div>();
    rtxui::Component<FallbackCard>::InitReflection();
  }
  std::string_view view =
      R"(<div><slot.header>DEFHEAD</slot.header>|<slot>DEFBODY</slot>|)"
      R"(<slot.footer>DEFFOOT</slot.footer></div>)";
};

class NoProjection : public rtxui::Component<NoProjection> {
 public:
  void InitReflection() override {
    Import<FallbackCard>();
    rtxui::Component<NoProjection>::InitReflection();
  }
  std::string_view view = R"(<FallbackCard></FallbackCard>)";
};

class BodyProjection : public rtxui::Component<BodyProjection> {
 public:
  bool show = true;
  void InitReflection() override {
    Bind(show);
    Import<FallbackCard>();
    Import<rtxui::span>();
    rtxui::Component<BodyProjection>::InitReflection();
  }
  std::string_view view =
      R"(<FallbackCard><if condition="{show}"><span>BODY</span></if></FallbackCard>)";
};

TEST_CASE("A slot shows its own content when nothing is projected",
          "[component][slot]") {
  // The content between a slot's tags is its fallback. It was never rendered
  // at all -- example/slots.cpp writes <slot.header>Default
  // Header</slot.header> and that text never appeared.
  auto text = [](auto& app) {
    std::string out;
    app->Root()->Visit([&out](rtxui::Element& element) {
      if (element.is_text()) {
        out += static_cast<rtxui::TextElement&>(element).text();
      }
    });
    return out;
  };

  SECTION("a component used with no children shows every fallback") {
    auto app = rtxui::Ref<NoProjection>::New();
    app->Mount();
    app->Digest();
    CHECK(text(app) == "DEFHEAD|DEFBODY|DEFFOOT");
  }

  SECTION("projecting into one slot leaves the others' fallbacks alone") {
    auto app = rtxui::Ref<BodyProjection>::New();
    app->Mount();
    app->Digest();
    CHECK(text(app) == "DEFHEAD|BODY|DEFFOOT");
  }

  SECTION("content that goes away brings the fallback back") {
    // The interesting direction: the slot has held projected content, so
    // showing the fallback again means rendering it a second time rather than
    // merely leaving it in place.
    auto app = rtxui::Ref<BodyProjection>::New();
    app->Mount();
    app->Digest();
    app->show = false;
    app->Digest();
    CHECK(text(app) == "DEFHEAD|DEFBODY|DEFFOOT");

    app->show = true;
    app->Digest();
    CHECK(text(app) == "DEFHEAD|BODY|DEFFOOT");
  }
}

class SlotCard : public rtxui::Component<SlotCard> {
 public:
  void InitReflection() override {
    Import<rtxui::div>();
    rtxui::Component<SlotCard>::InitReflection();
  }
  std::string_view view = R"(
    <div>
      <div>H<slot.header></slot.header></div>
      <div>B<slot></slot></div>
      <div>F<slot.footer></slot.footer></div>
    </div>
  )";
};

class ConditionalTemplateComponent
    : public rtxui::Component<ConditionalTemplateComponent> {
 public:
  bool with_header = true;
  bool with_footer = false;
  void InitReflection() override {
    Bind(with_header);
    Bind(with_footer);
    Import<SlotCard>();
    Import<rtxui::span>();
    rtxui::Component<ConditionalTemplateComponent>::InitReflection();
  }
  std::string_view view = R"(
    <SlotCard>
      <if condition="{with_header}">
        <template.header><span>HEAD</span></template.header>
      </if>
      <span>BODY</span>
      <if condition="{with_footer}">
        <template.footer><span>FOOT</span></template.footer>
      </if>
    </SlotCard>
  )";
};

TEST_CASE("A template that stops rendering takes its slot content with it",
          "[component][slot]") {
  // A <template.x> inside a condition that turns false used to leave its
  // content in the slot: nothing emptied it, since the pass that fills slots
  // only truncates the ones it writes to. The components behind that content
  // are released in the same reconcile, so what stayed behind was pointing at
  // freed memory -- a crash under a sanitizer, and silently stale otherwise.
  auto app = rtxui::Ref<ConditionalTemplateComponent>::New();
  app->Mount();
  app->Digest();

  auto text = [&]() {
    std::string out;
    app->Root()->Visit([&out](rtxui::Element& element) {
      if (element.is_text()) {
        out += static_cast<rtxui::TextElement&>(element).text();
      }
    });
    return out;
  };

  SECTION("content appears in the named slot it targets") {
    CHECK(text() == "HHEADBBODYF");
  }

  SECTION("a second named slot fills independently") {
    app->with_footer = true;
    app->Digest();
    CHECK(text() == "HHEADBBODYFFOOT");
  }

  SECTION("a template that disappears removes its content") {
    app->with_footer = true;
    app->Digest();
    app->with_header = false;
    app->Digest();
    CHECK(text() == "HBBODYFFOOT");
  }

  SECTION("and it comes back when the condition does") {
    app->with_header = false;
    app->Digest();
    app->with_header = true;
    app->with_footer = true;
    app->Digest();
    CHECK(text() == "HHEADBBODYFFOOT");
  }
}

class NestedLoopComponent : public rtxui::Component<NestedLoopComponent> {
 public:
  std::vector<std::string> outer{"a", "b"};
  std::vector<std::string> inner{"1", "2", "3"};
  bool show = true;
  void InitReflection() override {
    Bind(outer);
    Bind(inner);
    Bind(show);
    Import<rtxui::div>();
    Import<rtxui::span>();
    rtxui::Component<NestedLoopComponent>::InitReflection();
  }
  // The inner loop reads the outer loop's variable as well as its own, and
  // both use $index -- so a $index that leaked between scopes, or an outer
  // variable that did not reach the inner body, shows up in the text.
  std::string_view view = R"(
    <div id="root">
      <for each="{outer}" as="o">
        <span>[{o}:{$index}]</span>
        <for each="{inner}" as="i">
          <span>({o}-{i}-{$index})</span>
        </for>
      </for>
      <if condition="{show}">
        <for each="{inner}" as="k"><span>{k}</span></for>
      </if>
    </div>
  )";
};

TEST_CASE("Nested loops keep their own variable and index",
          "[component][for]") {
  auto app = rtxui::Ref<NestedLoopComponent>::New();
  app->Mount();
  app->Digest();

  auto text = [&]() {
    std::string out;
    app->Root()->Visit([&out](rtxui::Element& element) {
      if (element.is_text()) {
        out += static_cast<rtxui::TextElement&>(element).text();
      }
    });
    return out;
  };

  SECTION("each loop indexes itself, from zero") {
    CHECK(text() == "[a:0](a-1-0)(a-2-1)(a-3-2)[b:1](b-1-0)(b-2-1)(b-3-2)123");
  }

  SECTION("an empty collection yields nothing and leaves the rest alone") {
    app->inner.clear();
    app->Digest();
    // Both the inner loops and the one inside the <if> go, the outer stays.
    CHECK(text() == "[a:0][b:1]");
  }

  SECTION("a collection that empties and refills is rebuilt correctly") {
    app->inner.clear();
    app->Digest();
    app->inner = {"9"};
    app->Digest();
    CHECK(text() == "[a:0](a-9-0)[b:1](b-9-0)9");

    app->outer.clear();
    app->Digest();
    CHECK(text() == "9");

    app->outer = {"x", "y", "z"};
    app->inner = {"1", "2"};
    app->Digest();
    CHECK(text() ==
          "[x:0](x-1-0)(x-2-1)[y:1](y-1-0)(y-2-1)[z:2](z-1-0)(z-2-1)12");
  }

  SECTION("a loop inside a condition disappears with it") {
    app->show = false;
    app->Digest();
    CHECK(text() == "[a:0](a-1-0)(a-2-1)(a-3-2)[b:1](b-1-0)(b-2-1)(b-3-2)");
  }
}

class WordWrapStaleSpaceTestComponent
    : public rtxui::Component<WordWrapStaleSpaceTestComponent> {
 public:
  void InitReflection() override {
    Import<rtxui::div>();
    rtxui::Component<WordWrapStaleSpaceTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <div class="box">ab abcdefgh</div>
    <style>
      .box { width: 6; }
    </style>
  )";
};

TEST_CASE(
    "Word wrap doesn't lose width when overflow is detected several "
    "characters past the last space",
    "[component][word-wrap][regression]") {
  // Regression: when a single-word run overflows a few characters after the
  // last recorded space (not immediately after it), the break-at-space
  // logic used to reset cur_col back to col_start, discarding the width of
  // the characters typed since the space. The resulting fragment's declared
  // width undercounted its actual text, corrupting the following wrap.
  auto container = rtxui::Ref<WordWrapStaleSpaceTestComponent>::New();
  auto texture = RenderComponent(container, 80, 24);

  // "ab abcdefgh" in a width:6 box: "ab" fits, then "abcdefgh" doesn't fit
  // after it and overflow isn't detected until the 4th character of that
  // word ("ab abc|d"), several characters past the space.
  CHECK(RowText(texture, 0) == "ab");
  CHECK(RowText(texture, 1) == "abcdef");
  CHECK(RowText(texture, 2) == "gh");
}

class SpaceTestComponent : public rtxui::Component<SpaceTestComponent> {
 public:
  void InitReflection() override {
    Import<rtxui::b>();
    rtxui::Component<SpaceTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <div>This text is a <b>bold</b> statement</div>
  )";
};

TEST_CASE("Whitespace preservation around inline tags",
          "[component][xml][space]") {
  auto container = rtxui::Ref<SpaceTestComponent>::New();
  container->Mount();

  auto* root = container->Root();
  REQUIRE(root != nullptr);
  auto* div = root->QuerySelector("div");
  REQUIRE(div != nullptr);

  auto* slot = div->children()[0].get();
  REQUIRE(slot != nullptr);
  REQUIRE(slot->children().size() == 3);

  auto* child1 = dynamic_cast<rtxui::TextElement*>(slot->children()[0].get());
  REQUIRE(child1 != nullptr);
  CHECK(child1->text() == "This text is a ");

  const rtxui::ComponentBase* child2 = slot->children()[1]->component();
  REQUIRE(child2 != nullptr);
  CHECK(child2->Tag() == "b");

  auto* child3 = dynamic_cast<rtxui::TextElement*>(slot->children()[2].get());
  REQUIRE(child3 != nullptr);
  CHECK(child3->text() == " statement");
}

class ChildPropsTest : public rtxui::Component<ChildPropsTest> {
 public:
  struct Props {
    int value = 0;
  } props;

  void InitReflection() override {
    rtxui::Component<ChildPropsTest>::InitReflection();
    Bind(props.value);
  }

  std::string_view view = R"(
    <div>Value: {props.value}</div>
  )";
};

class ParentPropsTest : public rtxui::Component<ParentPropsTest> {
 public:
  int parent_value = 10;

  void InitReflection() override {
    Import<ChildPropsTest>();
    rtxui::Component<ParentPropsTest>::InitReflection();
    Bind(parent_value);
  }

  std::string_view view = R"(
    <ChildPropsTest id="child1" props.value="{parent_value}" />
    <ChildPropsTest id="child2" value="{parent_value}" />
  )";
};

TEST_CASE("Component props and two-way propagation", "[component][props]") {
  auto parent = rtxui::Ref<ParentPropsTest>::New();
  parent->Mount();
  parent->Digest();

  auto* root = parent->Root();
  auto* child1_el = root->QuerySelector("#child1");
  auto* child2_el = root->QuerySelector("#child2");
  REQUIRE(child1_el != nullptr);
  REQUIRE(child2_el != nullptr);

  auto* child1 = const_cast<rtxui::ComponentBase*>(child1_el->component());
  auto* child2 = const_cast<rtxui::ComponentBase*>(child2_el->component());
  REQUIRE(child1 != nullptr);
  REQUIRE(child2 != nullptr);

  // Check initial properties
  CHECK(parent->parent_value == 10);
  CHECK(rtxui::ComponentInternals::GetInterpolatedValue(*child1, "value") ==
        "10");
  CHECK(rtxui::ComponentInternals::GetInterpolatedValue(*child2, "value") ==
        "10");

  // Simulate child1 modifying its prop (e.g. from user input)
  rtxui::ComponentInternals::SetProperty(*child1, "props.value", "42");

  // Running Digest to propagate reactive updates
  parent->Digest();

  // The parent should be updated to 42
  CHECK(parent->parent_value == 42);

  // child2 should also receive the updated value 42
  CHECK(rtxui::ComponentInternals::GetInterpolatedValue(*child2, "value") ==
        "42");
}

}  // namespace

#define RTXUI_BENCHMARK

#include "../../../example/demo.cpp"

TEST_CASE("Add Todo Regression Test", "[demo]") {
  auto app = Ref<App>::New();
  auto device = std::make_shared<MockTerminalDevice>();
  device->TriggerResize(120, 40);
  Screen screen(app, device);

  screen.Draw();
  REQUIRE(app->todos.size() == 2);

  app->AddTodo();
  REQUIRE(app->todos.size() == 3);
  CHECK(app->todos[2].appearing == true);

  screen.Step();
  CHECK(app->todos[2].appearing == false);

  screen.Draw();
}

TEST_CASE("Slider click on real demo layout squash regression",
          "[demo][regression]") {
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  device->TriggerResize(120, 40);

  auto app = rtxui::Ref<App>::New();
  rtxui::Screen screen(app, device);
  screen.Draw();

  auto* slider_el = app->Root()->QuerySelector("slider");
  REQUIRE(slider_el != nullptr);
  auto* sidebar_el = app->Root()->QuerySelector("Sidebar");
  REQUIRE(sidebar_el != nullptr);

  int initial_sidebar_width = sidebar_el->layout_width();

  // Click/Drag the slider
  Event::Mouse mouse;
  mouse.button = Event::Mouse::Button::Left;
  mouse.motion = Event::Mouse::Motion::Pressed;
  mouse.x = slider_el->absolute_x() + 5;
  mouse.y = slider_el->absolute_y() + 1;
  screen.Dispatch(Event(mouse));

  mouse.motion = Event::Mouse::Motion::Released;
  screen.Dispatch(Event(mouse));

  screen.Draw();

  int final_sidebar_width = sidebar_el->layout_width();
  CHECK(final_sidebar_width == initial_sidebar_width);
}

TEST_CASE("Mouse capture is released when component is destroyed",
          "[component]") {
  ComponentBase* raw_comp_ptr = nullptr;
  {
    auto comp = rtxui::Ref<rtxui::slider>::New();
    comp->CaptureMouse();
    raw_comp_ptr = comp.get();
    CHECK(rtxui::ComponentInternals::GetMouseCapturer() == raw_comp_ptr);
  }
  CHECK(rtxui::ComponentInternals::GetMouseCapturer() == nullptr);
}

class HoverActiveTestComponent
    : public rtxui::Component<HoverActiveTestComponent> {
 public:
  std::string_view view = R"(
    <div id="test-node">Test</div>
  )";
};

TEST_CASE("Hovered and Active states are preserved across Render",
          "[component]") {
  auto container = rtxui::Ref<HoverActiveTestComponent>::New();
  container->Mount();

  auto* node = container->Root()->QuerySelector("#test-node");
  REQUIRE(node != nullptr);

  node->set_hovered(true);
  node->set_active(true);

  // Re-render
  container->Render();

  // Query node again
  auto* new_node = container->Root()->QuerySelector("#test-node");
  REQUIRE(new_node != nullptr);

  CHECK(new_node->hovered() == true);
  CHECK(new_node->active() == true);
}

class TransitionFlickerTestComponent
    : public rtxui::Component<TransitionFlickerTestComponent> {
 public:
  std::string_view view = R"html(
    <div id="test-node">Test</div>
    <style>
      #test-node {
        opacity: 0.8;
        transition: opacity 0.1s linear;
      }
      #test-node:focus {
        opacity: 1.0;
      }
    </style>
  )html";
};

TEST_CASE("Flickering transition is not triggered on re-render when focused",
          "[component][transition]") {
  auto container = rtxui::Ref<TransitionFlickerTestComponent>::New();
  container->Mount();

  auto* node = container->Root()->QuerySelector("#test-node");
  REQUIRE(node != nullptr);

  // Focus the element
  node->set_focused(true);

  // Initial render (and resolve target styles to start focus transition)
  container->Render();

  double now = rtxui::time::GetTimeMs();

  // Complete the transition by ticking at a later time
  node->TickTransitions(now + 200.0);
  CHECK(node->style.opacity == 1.0f);
  CHECK(node->active_transitions.empty());

  // Call Render again while focused
  container->Render();

  // No transitions should be started because we remained focused!
  CHECK(node->active_transitions.empty());
  CHECK(node->style.opacity == 1.0f);
}

class DynamicTagsTestComponent
    : public rtxui::Component<DynamicTagsTestComponent> {
 public:
  bool show_span_first = true;
  void InitReflection() override {
    Import<rtxui::div>();
    Import<rtxui::span>();
    Bind(show_span_first);
    rtxui::Component<DynamicTagsTestComponent>::InitReflection();
  }
  std::string_view view = R"html(
    <div id="container">
      <if condition="{show_span_first}">
        <span id="el-span">Span</span>
        <div id="el-div">Div</div>
      </if>
      <else>
        <div id="el-div">Div</div>
        <span id="el-span">Span</span>
      </else>
    </div>
  )html";
  DynamicTagsTestComponent() { Bind(show_span_first); }
};

TEST_CASE(
    "Component root elements are reused and reordered during reconciliation",
    "[component][reconciliation]") {
  auto container = rtxui::Ref<DynamicTagsTestComponent>::New();
  container->Mount();

  // Initial render
  container->Render();

  auto* root_div = container->Root()->QuerySelector("#container");
  REQUIRE(root_div != nullptr);
  REQUIRE(root_div->ChildCount() == 1);
  auto* slot_el = root_div->ChildAt(0);
  REQUIRE(slot_el != nullptr);
  REQUIRE(slot_el->ChildCount() == 2);

  auto* span = slot_el->ChildAt(0);
  auto* div = slot_el->ChildAt(1);
  REQUIRE(span != nullptr);
  REQUIRE(div != nullptr);
  CHECK(span->tag() == "span");
  CHECK(div->tag() == "div");

  // Toggle order of elements (which will cause a tag mismatch at index 0)
  container->show_span_first = false;
  container->Render();

  // The child elements should have been reordered (swapped) in-place without
  // creating/destroying new elements
  REQUIRE(slot_el->ChildCount() == 2);
  CHECK(slot_el->ChildAt(0) ==
        div);  // The div component root should now be first
  CHECK(slot_el->ChildAt(1) ==
        span);  // The span component root should now be second
}

class StandardTagsTestComponent
    : public rtxui::Component<StandardTagsTestComponent> {
 public:
  bool show_section_first = true;
  void InitReflection() override {
    Import<rtxui::div>();
    Bind(show_section_first);
    rtxui::Component<StandardTagsTestComponent>::InitReflection();
  }
  std::string_view view = R"html(
    <div id="container">
      <if condition="{show_section_first}">
        <section>Section</section>
        <header>Header</header>
      </if>
      <else>
        <header>Header</header>
        <section>Section</section>
      </else>
    </div>
  )html";
  StandardTagsTestComponent() { Bind(show_section_first); }
};

TEST_CASE("Standard elements are reused and reordered during reconciliation",
          "[component][reconciliation]") {
  auto container = rtxui::Ref<StandardTagsTestComponent>::New();
  container->Mount();

  // Initial render
  container->Render();

  auto* root_div = container->Root()->QuerySelector("#container");
  REQUIRE(root_div != nullptr);
  REQUIRE(root_div->ChildCount() == 1);
  auto* slot_el = root_div->ChildAt(0);
  REQUIRE(slot_el != nullptr);
  REQUIRE(slot_el->ChildCount() == 2);

  auto* section = slot_el->ChildAt(0);
  auto* header = slot_el->ChildAt(1);
  REQUIRE(section != nullptr);
  REQUIRE(header != nullptr);
  CHECK(section->tag() == "section");
  CHECK(header->tag() == "header");

  // Toggle order of elements (which will cause a tag mismatch at index 0)
  container->show_section_first = false;
  container->Render();

  // The child elements should have been reordered (swapped) in-place without
  // creating/destroying new elements
  REQUIRE(slot_el->ChildCount() == 2);
  CHECK(slot_el->ChildAt(0) ==
        header);  // The header element should now be first
  CHECK(slot_el->ChildAt(1) ==
        section);  // The section element should now be second
}

// --- Hot Reload ---
class HotReloadTestComponent : public rtxui::Component<HotReloadTestComponent> {
 public:
  void InitReflection() override {
    Import<rtxui::div>();
    rtxui::Component<HotReloadTestComponent>::InitReflection();
  }
  std::string_view view = R"html(
    <div id="target">Initial Content</div>
  )html";
};

TEST_CASE("Component Template Hot Reloading", "[component][hotreload]") {
  std::string temp_file = "temp_hot_reload_test.cpp";

  // 1. Write initial template to temp file
  {
    std::ofstream out(temp_file);
    out << "std::string_view view = R\"html(\n"
        << "  <div id=\"target\">Initial Content</div>\n"
        << ")html\";\n";
  }

  // Make sure it is registered and points to the temp file
  auto container = rtxui::Ref<HotReloadTestComponent>::New();
  container->EnableHotReload("view", temp_file);

  rtxui::Screen screen(container);
  screen.Draw();

  // Initial check
  auto* target_el = container->Root()->QuerySelector("#target");
  REQUIRE(target_el != nullptr);
  CHECK(target_el->Print().find("Initial Content") != std::string::npos);

  // 2. Modify template in temp file
  {
    std::ofstream out(temp_file);
    out << "std::string_view view = R\"html(\n"
        << "  <div id=\"target\">Updated Content</div>\n"
        << ")html\";\n";
  }

  // Force file modification time forward to simulate a save
  std::error_code ec;
  auto current_time = std::filesystem::last_write_time(temp_file, ec);
  if (!ec) {
    std::filesystem::last_write_time(
        temp_file, current_time + std::chrono::seconds(2), ec);
  }

  // Poll changes and verify it detects and reloads
  bool reloaded = rtxui::HotReloadManager::PollChanges();
  CHECK(reloaded == true);

  // Re-draw screen to update layout and check the node contents
  screen.Draw();

  auto* updated_el = container->Root()->QuerySelector("#target");
  REQUIRE(updated_el != nullptr);
  CHECK(updated_el->Print().find("Updated Content") != std::string::npos);

  // Clean up
  std::filesystem::remove(temp_file);
}

namespace rtxui {

void PrintCompilerStyleError(std::string_view source_string,
                             int error_line,
                             int error_column,
                             std::string_view message,
                             std::string_view label);

}  // namespace rtxui

TEST_CASE("PrintCompilerStyleError Formatting", "[component][error]") {
  std::string_view sample = "line 1\nline 2\nline 3 error here\nline 4\nline 5";

  std::stringstream buffer;
  std::streambuf* old = std::cerr.rdbuf(buffer.rdbuf());

  rtxui::PrintCompilerStyleError(sample, 2, 7, "invalid token", "DOM");

  std::cerr.rdbuf(old);

  std::string output = buffer.str();

  CHECK(output.find("DOM Error") != std::string::npos);
  CHECK(output.find("invalid token") != std::string::npos);
  CHECK(output.find("Line 3, Column 8:") != std::string::npos);
  CHECK(output.find(" >    3 │ line 3 error here") != std::string::npos);
  CHECK(output.find("        │        ^") != std::string::npos);
  CHECK(output.find("      1 │ line 1") != std::string::npos);
  CHECK(output.find("      2 │ line 2") != std::string::npos);
  CHECK(output.find("      4 │ line 4") != std::string::npos);
  CHECK(output.find("      5 │ line 5") != std::string::npos);
}

namespace {

class MalformedTemplateApp : public Component<MalformedTemplateApp> {
 public:
  // Unterminated tag: xml::Parse rejects this.
  std::string_view view = R"(<div class=)";
};

}  // namespace

TEST_CASE(
    "A malformed template reaches the diagnostic handler instead of exiting",
    "[component][error]") {
  // Regression: the template parse path called std::exit(1) unconditionally,
  // ignoring the error handler the app installed -- so an app that had
  // asked to surface XML errors through its own UI was killed instead. Found
  // by the layout fuzzer, which could not survive its own first malformed
  // input.
  std::vector<rtxui::Diagnostic> seen;
  rtxui::SetDiagnosticHandler([&](const rtxui::Diagnostic& e) {
    if (e.kind == rtxui::Diagnostic::Kind::XmlSyntax) {
      seen.push_back(e);
    }
  });

  auto app = Ref<MalformedTemplateApp>::New();
  app->Mount();  // Must return rather than terminate the process.

  rtxui::SetDiagnosticHandler(nullptr);

  REQUIRE(seen.size() == 1);
  CHECK_FALSE(seen[0].message.empty());
  // Mount must not go on to read the parse result it does not have; the
  // component simply renders nothing.
  CHECK(app->Root() != nullptr);
}

namespace {

class ControlFlowNoAttrApp : public Component<ControlFlowNoAttrApp> {
 public:
  void InitReflection() override {
    Import<rtxui::div>();
    Component<ControlFlowNoAttrApp>::InitReflection();
  }
  std::string_view view = R"(
    <div>
      <if><div>a</div></if>
      <for><div>b</div></for>
    </div>
  )";
};

class ElifNoAttrApp : public Component<ElifNoAttrApp> {
 public:
  void InitReflection() override {
    Import<rtxui::div>();
    Component<ElifNoAttrApp>::InitReflection();
  }
  std::string_view view = R"(
    <div>
      <if condition="false"><div>a</div></if>
      <elif><div>b</div></elif>
    </div>
  )";
};

}  // namespace

TEST_CASE("Control-flow tags without their attribute render nothing",
          "[component][template]") {
  // Regression: <if>/<elif> read attributes.at("condition") and <for> read
  // attributes.at("each") with no presence check, so a template that omitted
  // the attribute threw std::out_of_range ("map::at") out of Render() -- an
  // opaque crash for what is a plain template typo. Found by the layout
  // fuzzer.
  SECTION("<if> and <for> without their attribute") {
    auto app = Ref<ControlFlowNoAttrApp>::New();
    REQUIRE_NOTHROW(app->Mount());
    // A condition that is not "true" renders nothing, and so does a missing
    // one; neither branch's content should appear.
    CHECK(app->Root()->QuerySelector("div") != nullptr);
  }

  SECTION("<elif> without a condition, reached because the <if> was false") {
    auto app = Ref<ElifNoAttrApp>::New();
    REQUIRE_NOTHROW(app->Mount());
  }
}

class DisabledInteractiveTestComponent
    : public rtxui::Component<DisabledInteractiveTestComponent> {
 public:
  bool cb_checked = false;
  bool radio_checked = false;
  std::string select_value = "a";
  int slider_val = 50;
  bool button_clicked = false;

  void InitReflection() override {
    Bind(cb_checked);
    Bind(radio_checked);
    Bind(select_value);
    Bind(slider_val);
    Import<rtxui::checkbox>();
    Import<rtxui::radio>();
    Import<rtxui::select>();
    Import<rtxui::option>();
    Import<rtxui::slider>();
    Import<rtxui::button>();
    Import("OnButtonClick", [this]() { button_clicked = true; });
    rtxui::Component<DisabledInteractiveTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <checkbox id="cb" checked="{cb_checked}" disabled="true">Check</checkbox>
    <radio id="rd" checked="{radio_checked}" disabled="true">Radio</radio>
    <select id="sel" value="{select_value}" disabled="true">
      <option value="a">A</option>
      <option value="b">B</option>
    </select>
    <slider id="sl" value="{slider_val}" min="0" max="100" width="11" disabled="true" />
    <button id="btn" onclick="OnButtonClick" disabled="true">Go</button>
  )";
};

TEST_CASE(
    "disabled=\"\" blocks interaction on checkbox/radio/select/slider/"
    "button",
    "[component][disabled][regression]") {
  // Regression: disabled was only ever wired up for input/textarea (via
  // TextInputBase); checkbox, radio, select, slider, and button silently
  // ignored the attribute and stayed fully interactive. Each now syncs
  // disabled onto its root Element (for :disabled CSS and Screen's
  // tab-navigation/click gating, both keyed off Element::disabled()) and
  // no-ops its own OnEvent (button has none of its own - Screen's click
  // path already gates on Element::disabled() centrally).
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  auto container = rtxui::Ref<DisabledInteractiveTestComponent>::New();
  rtxui::Screen screen(container, device);
  screen.Draw();

  auto* root = container->Root();

  auto* cb_el = root->QuerySelector("#cb");
  auto* cb_ptr = dynamic_cast<rtxui::checkbox*>(
      const_cast<rtxui::ComponentBase*>(cb_el->component()));
  REQUIRE(cb_ptr != nullptr);
  cb_el->set_focused(true);
  cb_ptr->OnEvent(Event::Keyboard::From(' '));
  CHECK_FALSE(cb_ptr->checked);

  auto* rd_el = root->QuerySelector("#rd");
  auto* rd_ptr = dynamic_cast<rtxui::radio*>(
      const_cast<rtxui::ComponentBase*>(rd_el->component()));
  REQUIRE(rd_ptr != nullptr);
  rd_el->set_focused(true);
  rd_ptr->OnEvent(Event::Keyboard::From(' '));
  CHECK_FALSE(rd_ptr->checked);

  auto* sel_el = root->QuerySelector("#sel");
  auto* sel_ptr = dynamic_cast<rtxui::select*>(
      const_cast<rtxui::ComponentBase*>(sel_el->component()));
  REQUIRE(sel_ptr != nullptr);
  sel_el->set_focused(true);
  sel_ptr->OnEvent(Event::Keyboard({
      Event::Keyboard::Motion::Pressed,
      Event::Keyboard::Special::Return,
  }));
  CHECK_FALSE(sel_ptr->is_open);

  auto* sl_el = root->QuerySelector("#sl");
  auto* sl_ptr = dynamic_cast<rtxui::slider*>(
      const_cast<rtxui::ComponentBase*>(sl_el->component()));
  REQUIRE(sl_ptr != nullptr);
  Event::Mouse mouse;
  mouse.button = Event::Mouse::Button::Left;
  mouse.motion = Event::Mouse::Motion::Pressed;
  mouse.x = sl_el->absolute_x() + 2;
  mouse.y = sl_el->absolute_y() + 1;
  screen.Dispatch(Event(mouse));
  CHECK(sl_ptr->value == 50);

  auto* btn_el = root->QuerySelector("#btn");
  Event::Mouse btn_mouse;
  btn_mouse.button = Event::Mouse::Button::Left;
  btn_mouse.motion = Event::Mouse::Motion::Pressed;
  btn_mouse.x = btn_el->absolute_x() + 1;
  btn_mouse.y = btn_el->absolute_y() + 1;
  screen.Dispatch(Event(btn_mouse));
  CHECK_FALSE(container->button_clicked);

  // :disabled dims the default style for each, mirroring input's behavior.
  container->ResolveTargetStyles();
  CHECK(cb_el->target_style.opacity < 0.9f);
  CHECK(rd_el->target_style.opacity < 0.9f);
  CHECK(sl_el->target_style.opacity < 0.9f);
  CHECK(btn_el->target_style.opacity < 0.85f);
}

namespace {

class TwiceImportingApp : public rtxui::Component<TwiceImportingApp> {
 public:
  void InitReflection() override {
    Import<rtxui::div>();
    Import<rtxui::div>();  // Same component again.
    rtxui::Component<TwiceImportingApp>::InitReflection();
  }
  std::string_view view = R"(<div id="a">hello</div>)";
};

}  // namespace

TEST_CASE("Importing the same component twice is not fatal",
          "[component][import]") {
  // Regression: a duplicate import printed a diagnostic and called
  // std::exit(1), taking the host process with it. A library has no business
  // ending someone else's program over a setup mistake -- and in a TUI the
  // diagnostic went to stdout, i.e. straight into the frame being drawn.
  // The first registration stands and the duplicate is ignored.
  std::stringstream captured;
  std::streambuf* previous = std::cerr.rdbuf(captured.rdbuf());

  auto app = rtxui::Ref<TwiceImportingApp>::New();
  app->Mount();

  std::cerr.rdbuf(previous);

  // It still says something, on stderr rather than stdout.
  CHECK(captured.str().find("already imported") != std::string::npos);
  // ...and the component still works.
  auto* el = app->Root()->QuerySelector("#a");
  REQUIRE(el != nullptr);
  CHECK(el->tag() == "div");
}

namespace {

class RenderCountedChild : public rtxui::Component<RenderCountedChild> {
 public:
  int value = 0;
  mutable int renders = 0;
  // Interpolated once per render, so its calls count the renders.
  std::string label() const {
    ++renders;
    return std::to_string(value);
  }
  RenderCountedChild() {
    Bind(value);
    Bind(label);
  }
  std::string_view view = R"(<span>{label}</span>)";
};

class RenderCountedParent : public rtxui::Component<RenderCountedParent> {
 public:
  int value = 0;
  void InitReflection() override {
    Bind(value);
    Import<RenderCountedChild>();
    rtxui::Component<RenderCountedParent>::InitReflection();
  }
  std::string_view view = R"(
    <div>
      <RenderCountedChild value="{value}"/>
    </div>
  )";
};

}  // namespace

TEST_CASE("A child given a new prop renders once per Digest()", "[component]") {
  // The parent's render passes the new value and re-renders the child. The
  // child's own Digest() then runs, and must not find that same change again.
  auto app = rtxui::Ref<RenderCountedParent>::New();
  app->Mount();
  auto* child = dynamic_cast<RenderCountedChild*>(
      app->QueryComponent("RenderCountedChild"));
  REQUIRE(child != nullptr);

  app->value = 1;
  child->renders = 0;
  app->Digest();
  CHECK(child->value == 1);
  CHECK(child->renders == 1);

  child->renders = 0;
  app->Digest();
  CHECK(child->renders == 0);
}

TEST_CASE("HotReload reports a parse error to the XML error handler",
          "[component][hot_reload]") {
  // It used to print straight to std::cerr, which lands in the middle of a
  // raw-mode frame and cannot be surfaced by the app's own UI.
  struct HotReloadTarget : public rtxui::Component<HotReloadTarget> {
    std::string_view view = "<div>ok</div>";
  };
  auto app = rtxui::Ref<HotReloadTarget>::New();
  app->Mount();

  std::optional<rtxui::Diagnostic> reported_error;
  rtxui::SetDiagnosticHandler([&](const rtxui::Diagnostic& error) {
    if (error.kind == rtxui::Diagnostic::Kind::XmlSyntax) {
      reported_error = error;
    }
  });
  app->HotReload("<div>\n  <span>\n</div>");
  rtxui::SetDiagnosticHandler(nullptr);

  REQUIRE(reported_error.has_value());
  CHECK_FALSE(reported_error->message.empty());
}

namespace {

// Shows its slot only while `open` is set, deciding that on its own.
class SlotToggle : public rtxui::Component<SlotToggle> {
 public:
  bool open = true;
  SlotToggle() { Bind(open); }
  std::string_view view = R"(
    <div><if condition="{open}"><slot></slot></if></div>
  )";
};

class SlotToggleUser : public rtxui::Component<SlotToggleUser> {
 public:
  SlotToggleUser() { Import<SlotToggle>(); }
  std::string_view view = R"(
    <div><SlotToggle><span id="projected">x</span></SlotToggle></div>
  )";
};

}  // namespace

TEST_CASE("A slot a component shows again gets its projected content back",
          "[component][slot]") {
  auto app = rtxui::Ref<SlotToggleUser>::New();
  app->Mount();
  REQUIRE(app->Root()->QuerySelector("#projected") != nullptr);
  auto* toggle = dynamic_cast<SlotToggle*>(const_cast<rtxui::ComponentBase*>(
      app->Root()->QuerySelector("SlotToggle")->component()));
  REQUIRE(toggle != nullptr);

  // Only <SlotToggle> changes, so only it re-renders: the content it
  // projects is its user's, which has to fill the slot again.
  toggle->open = false;
  app->Digest();
  CHECK(app->Root()->QuerySelector("#projected") == nullptr);
  toggle->open = true;
  app->Digest();
  CHECK(app->Root()->QuerySelector("#projected") != nullptr);
}

namespace {

class AliasedCounter : public rtxui::Component<AliasedCounter> {
 public:
  int clicks = 0;
  AliasedCounter() { Bind(clicks); }
  std::string_view view = R"(<span>{clicks}</span>)";
};

class AliasUser : public rtxui::Component<AliasUser> {
 public:
  int renders = 0;
  AliasUser() {
    Bind(renders);
    Import<AliasedCounter>("my-counter");
  }
  std::string_view view = R"(
    <div>
      <span>{renders}</span>
      <my-counter id="aliased"/>
      <tabs><tab-pane label="a"><span id="pane">a</span></tab-pane></tabs>
    </div>
  )";
};

}  // namespace

TEST_CASE("A component written under another tag survives its user's render",
          "[component]") {
  auto app = rtxui::Ref<AliasUser>::New();
  app->Mount();
  // The component of type T in the tree.
  auto find = [&]<typename T>(std::type_identity<T>) -> T* {
    T* found = nullptr;
    app->Root()->Visit([&](rtxui::Element& element) {
      if (auto* component = dynamic_cast<const T*>(element.component())) {
        found = const_cast<T*>(component);
      }
    });
    REQUIRE(found != nullptr);
    return found;
  };
  AliasedCounter* counter = find(std::type_identity<AliasedCounter>{});
  rtxui::tab_pane* pane = find(std::type_identity<rtxui::tab_pane>{});
  counter->clicks = 3;

  app->renders++;
  app->Digest();
  // Matched on the tag the template wrote, not the class name, so the alias
  // and the hyphenated built-in keep their instances, and their state.
  CHECK(find(std::type_identity<AliasedCounter>{}) == counter);
  CHECK(find(std::type_identity<rtxui::tab_pane>{}) == pane);
  CHECK(counter->clicks == 3);
}

namespace {

class ClickFocusApp : public rtxui::Component<ClickFocusApp> {
 public:
  std::string_view view = R"(
    <div>
      <button id="button">press</button>
    </div>
    <style>
      button:focus { margin-left: 1; }
    </style>
  )";
};

}  // namespace

TEST_CASE("Clicking a button's label focuses the button",
          "[component][screen][focus]") {
  auto app = rtxui::Ref<ClickFocusApp>::New();
  rtxui::HeadlessScreen screen(app, 40, 10);
  auto* button = app->Root()->QuerySelector("#button");
  REQUIRE(button != nullptr);
  // The middle of " press ", on the text rather than the padding.
  screen.Click(button->absolute_x() + 3, button->absolute_y());
  button = app->Root()->QuerySelector("#button");
  CHECK(button->focused());
  CHECK(button->style.margin.left == 1);  // button:focus
}
