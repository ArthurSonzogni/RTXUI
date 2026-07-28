// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/internal/component.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <fstream>

#include "rtxui/component/default_components_internal.hpp"
#include "rtxui/base/string.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/internal/screen.hpp"
#include "rtxui/layout/layout.hpp"
#include "rtxui/layout/layout_tree_builder.hpp"
#include "rtxui/paint/color.hpp"
#include "rtxui/paint/paint.hpp"
#include "rtxui/paint/texture.hpp"
#include "rtxui/terminal/terminal_device.hpp"

namespace {

std::string RemoveWhitespace(std::string_view str) {
  std::string result;
  for (char c : str) {
    if (!std::isspace(static_cast<unsigned char>(c))) {
      result += c;
    }
  }
  return result;
}

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

TEST_CASE("Slider squashed layout regression", "[component][slider][layout]") {
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  device->TriggerResize(40, 5);

  class SliderTest : public rtxui::Component<SliderTest> {
   public:
    void InitReflection() override {
      Import<rtxui::slider>();
      rtxui::Component<SliderTest>::InitReflection();
    }
    std::string_view view = R"(
      <div style="display: flex; width: 5;">
        <slider id="slider" width="20" value="50" />
      </div>
    )";
  };

  auto component = rtxui::Ref<SliderTest>::New();
  rtxui::Screen screen(component, device);
  screen.Draw();

  std::string output = device->GetOutput();

  // Count track characters '─'. We expect 19 (10 + 9).
  int track_count = 0;
  size_t pos = 0;
  while ((pos = output.find("─", pos)) != std::string::npos) {
    track_count++;
    pos += 3;  // '─' is 3 bytes in UTF-8
  }

  if (track_count <
      15) {  // Allow some slack but ensure it's not squashed to ~3
    FAIL("Slider squashed. Track count=" << track_count << "\nOutput:\n"
                                         << output);
  }

  CHECK(track_count >= 15);
  CHECK(output.find("●") != std::string::npos);
}

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

class InputTestComponent : public rtxui::Component<InputTestComponent> {
 public:
  std::string my_text = "hello world";
  void InitReflection() override {
    Bind(my_text);
    Import<rtxui::input>();
    rtxui::Component<InputTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <input value="{my_text}" />
  )";
};

TEST_CASE("Input Component Basic Interactions", "[component]") {
  auto container = rtxui::Ref<InputTestComponent>::New();
  container->Mount();

  auto* input_el = container->Root()->QuerySelector("input");
  REQUIRE(input_el != nullptr);

  auto* input_comp = const_cast<rtxui::ComponentBase*>(input_el->component());
  REQUIRE(input_comp != nullptr);

  auto* input_ptr = dynamic_cast<rtxui::input*>(input_comp);
  REQUIRE(input_ptr != nullptr);

  // Verify initial state
  CHECK(input_ptr->value == "hello world");
  CHECK(input_ptr->cursor_pos == 0);

  // Focus to accept events
  input_el->set_focused(true);

  // ArrowRight
  input_ptr->OnEvent(Event::ArrowRight());
  input_ptr->Digest();
  CHECK(input_ptr->cursor_pos == 1);

  // Type a char '!'
  input_ptr->OnEvent(Event::Keyboard::From('!'));
  input_ptr->Digest();
  CHECK(input_ptr->value == "h!ello world");
  CHECK(input_ptr->cursor_pos == 2);

  // Backspace
  input_ptr->OnEvent(Event::Backspace());
  input_ptr->Digest();
  CHECK(input_ptr->value == "hello world");
  CHECK(input_ptr->cursor_pos == 1);

  // Ctrl + ArrowRight to skip word boundary
  input_ptr->OnEvent(Event::ArrowRightCtrl());
  input_ptr->Digest();
  CHECK(input_ptr->cursor_pos == 5);

  // Ctrl + Backspace to delete the word "hello"
  input_ptr->OnEvent(Event::BackspaceCtrl());
  input_ptr->Digest();
  CHECK(input_ptr->value == " world");
  CHECK(input_ptr->cursor_pos == 0);

  // Ctrl + Delete to delete the word " world"
  input_ptr->OnEvent(Event::DeleteCtrl());
  input_ptr->Digest();
  CHECK(input_ptr->value == "");
  CHECK(input_ptr->cursor_pos == 0);

  // Test Alt modifiers (for word boundaries and deletion)
  input_ptr->value = "hello world";
  input_ptr->cursor_pos = 1;
  input_ptr->Digest();

  // Alt + ArrowRight to skip word boundary
  input_ptr->OnEvent(Event::ArrowRightAlt());
  input_ptr->Digest();
  CHECK(input_ptr->cursor_pos == 5);

  // Alt + Backspace to delete the word "hello"
  input_ptr->OnEvent(Event::BackspaceAlt());
  input_ptr->Digest();
  CHECK(input_ptr->value == " world");
  CHECK(input_ptr->cursor_pos == 0);

  // Alt + Delete to delete the word " world"
  input_ptr->OnEvent(Event::DeleteAlt());
  input_ptr->Digest();
  CHECK(input_ptr->value == "");
  CHECK(input_ptr->cursor_pos == 0);

  // Test CJK navigation
  input_ptr->value = "你好world";
  input_ptr->cursor_pos = 0;
  input_ptr->Digest();

  input_ptr->OnEvent(Event::ArrowRight());
  input_ptr->Digest();
  CHECK(input_ptr->cursor_pos == 1);

  input_ptr->OnEvent(Event::Keyboard::From('x'));
  input_ptr->Digest();
  CHECK(input_ptr->value == "你x好world");
  CHECK(input_ptr->cursor_pos == 2);

  // Test Scrolling keeping cursor visible
  input_el->set_layout_width(10);  // total layout width of 10 cells
  // With no border and 1 cell padding on left and right, inner visible width
  // is 8.
  input_ptr->value = "1234567890";
  input_ptr->cursor_pos = 9;
  input_ptr->Digest();
  // cursor_col = 9. scroll_x should adjust to 9 - 8 + 1 = 2.
  CHECK(input_el->scroll_x() == 2);
}

TEST_CASE("Input Component Layout Height", "[component]") {
  auto container = rtxui::Ref<InputTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* input_el = container->Root()->QuerySelector("input");
  REQUIRE(input_el != nullptr);
  // The layout height should be 1 cell (no borders).
  CHECK(input_el->layout_height() == 1);
}

TEST_CASE("Input Component Advanced Selection and Editing",
          "[component][input]") {
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  auto container = rtxui::Ref<InputTestComponent>::New();
  rtxui::Screen screen(container, device);
  screen.Draw();

  auto* input_el = container->Root()->QuerySelector("input");
  REQUIRE(input_el != nullptr);

  auto* input_comp = const_cast<rtxui::ComponentBase*>(input_el->component());
  REQUIRE(input_comp != nullptr);

  auto* input_ptr = dynamic_cast<rtxui::input*>(input_comp);
  REQUIRE(input_ptr != nullptr);

  input_el->set_focused(true);

  SECTION("Shift Selection and Arrow Movement") {
    input_ptr->value = "hello world";
    input_ptr->cursor_pos = 0;
    input_ptr->selection_start = -1;
    input_ptr->Digest();

    // Shift + ArrowRight
    Event::Keyboard kb;
    kb.special = Event::Keyboard::Special::ArrowRight;
    kb.modifier.shift = true;
    input_ptr->OnEvent(Event(kb));
    input_ptr->Digest();
    CHECK(input_ptr->selection_start == 0);
    CHECK(input_ptr->cursor_pos == 1);

    // Shift + ArrowRight again
    input_ptr->OnEvent(Event(kb));
    input_ptr->Digest();
    CHECK(input_ptr->selection_start == 0);
    CHECK(input_ptr->cursor_pos == 2);

    // ArrowLeft without shift clears selection and places cursor at min
    input_ptr->OnEvent(Event::ArrowLeft());
    input_ptr->Digest();
    CHECK(input_ptr->selection_start == -1);
    CHECK(input_ptr->cursor_pos == 0);
  }

  SECTION("CTRL+A Selection") {
    input_ptr->value = "hello world";
    input_ptr->cursor_pos = 2;
    input_ptr->selection_start = -1;
    input_ptr->Digest();

    input_ptr->OnEvent(Event::CtrlA());
    input_ptr->Digest();
    CHECK(input_ptr->selection_start == 0);
    CHECK(input_ptr->cursor_pos == 11);

    // Typing a character replaces selection
    input_ptr->OnEvent(Event::Keyboard::From('x'));
    input_ptr->Digest();
    CHECK(input_ptr->value == "x");
    CHECK(input_ptr->cursor_pos == 1);
    CHECK(input_ptr->selection_start == -1);
  }

  SECTION("Ctrl+Delete word deletion") {
    input_ptr->value = "hello world";
    input_ptr->cursor_pos = 0;
    input_ptr->selection_start = -1;
    input_ptr->Digest();

    input_ptr->OnEvent(Event::DeleteCtrl());
    input_ptr->Digest();
    CHECK(input_ptr->value == " world");
    CHECK(input_ptr->cursor_pos == 0);
  }

  SECTION("Double Click Word Selection") {
    input_ptr->value = "hello world";
    input_ptr->cursor_pos = 0;
    input_ptr->selection_start = -1;
    input_ptr->Digest();

    // Click on index 2 ('l' in "hello") twice
    int x_pos = input_el->absolute_x() + 1 + 2 +
                1;  // 1 (border/padding) + 2 (index) + 1 (1-based)
    Event::Mouse mouse;
    mouse.button = Event::Mouse::Button::Left;
    mouse.motion = Event::Mouse::Motion::Pressed;
    mouse.x = x_pos;
    mouse.y = input_el->absolute_y() + 1;

    // First Click
    screen.Dispatch(Event(mouse));
    // Release
    mouse.motion = Event::Mouse::Motion::Released;
    screen.Dispatch(Event(mouse));

    // Second Click (Double Click)
    mouse.motion = Event::Mouse::Motion::Pressed;
    screen.Dispatch(Event(mouse));

    input_ptr->Digest();
    // Selection should span "hello" -> [0, 5]
    CHECK(input_ptr->selection_start == 0);
    CHECK(input_ptr->cursor_pos == 5);

    // Release mouse
    mouse.motion = Event::Mouse::Motion::Released;
    screen.Dispatch(Event(mouse));
    input_ptr->Digest();
    // Selection should still be active
    CHECK(input_ptr->selection_start == 0);
    CHECK(input_ptr->cursor_pos == 5);
  }

  SECTION("Double Click + Move Selection") {
    input_ptr->value = "hello world test";
    input_ptr->cursor_pos = 0;
    input_ptr->selection_start = -1;
    input_ptr->Digest();

    // Double click at index 2 ('l' in "hello")
    int click_x = input_el->absolute_x() + 1 + 2 + 1;
    Event::Mouse mouse;
    mouse.button = Event::Mouse::Button::Left;
    mouse.motion = Event::Mouse::Motion::Pressed;
    mouse.x = click_x;
    mouse.y = input_el->absolute_y() + 1;

    screen.Dispatch(Event(mouse));
    mouse.motion = Event::Mouse::Motion::Released;
    screen.Dispatch(Event(mouse));

    mouse.motion = Event::Mouse::Motion::Pressed;
    screen.Dispatch(Event(mouse));
    input_ptr->Digest();
    REQUIRE(input_ptr->selection_start == 0);
    REQUIRE(input_ptr->cursor_pos == 5);

    // Now move the mouse to index 8 ('o' in "world")
    mouse.motion = Event::Mouse::Motion::Moved;
    mouse.x = input_el->absolute_x() + 1 + 8 + 1;
    screen.Dispatch(Event(mouse));
    input_ptr->Digest();

    // Selection should span "hello world" -> [0, 11]
    CHECK(input_ptr->selection_start == 0);
    CHECK(input_ptr->cursor_pos == 11);

    // Release mouse
    mouse.motion = Event::Mouse::Motion::Released;
    screen.Dispatch(Event(mouse));
  }
}

TEST_CASE("Input Component State Preservation", "[component]") {
  auto container = rtxui::Ref<InputTestComponent>::New();
  container->Mount();

  auto* input_el = container->Root()->QuerySelector("input");
  REQUIRE(input_el != nullptr);
  auto* input_comp = const_cast<rtxui::ComponentBase*>(input_el->component());
  REQUIRE(input_comp != nullptr);
  auto* input_ptr = dynamic_cast<rtxui::input*>(input_comp);
  REQUIRE(input_ptr != nullptr);

  // Focus and type 'a'
  input_el->set_focused(true);
  input_ptr->OnEvent(Event::Keyboard::From('a'));
  input_ptr->Digest();
  CHECK(input_ptr->value == "ahello world");
  CHECK(input_ptr->cursor_pos == 1);

  // Type 'b'
  input_ptr->OnEvent(Event::Keyboard::From('b'));
  input_ptr->Digest();
  // cursor_pos should be preserved as 1, so 'b' is typed after 'a'.
  CHECK(input_ptr->value == "abhello world");
  CHECK(input_ptr->cursor_pos == 2);
}

TEST_CASE("Input Component Disabled Blocks All Interaction",
          "[component][input][disabled]") {
  auto container = rtxui::Ref<InputTestComponent>::New();
  container->Mount();

  auto* input_el = container->Root()->QuerySelector("input");
  REQUIRE(input_el != nullptr);
  auto* input_ptr = dynamic_cast<rtxui::input*>(
      const_cast<rtxui::ComponentBase*>(input_el->component()));
  REQUIRE(input_ptr != nullptr);

  input_el->set_focused(true);
  input_ptr->disabled = true;

  bool handled = input_ptr->OnEvent(Event::Keyboard::From('a'));
  input_ptr->Digest();
  CHECK_FALSE(handled);
  CHECK(input_ptr->value == "hello world");

  // A disabled field can't stay focused either.
  CHECK_FALSE(input_el->focused());
}

TEST_CASE("Input Component Disabled Ignores Mouse Click Focus",
          "[component][input][disabled]") {
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  auto container = rtxui::Ref<InputTestComponent>::New();
  rtxui::Screen screen(container, device);
  screen.Draw();

  auto* input_el = container->Root()->QuerySelector("input");
  REQUIRE(input_el != nullptr);
  auto* input_ptr = dynamic_cast<rtxui::input*>(
      const_cast<rtxui::ComponentBase*>(input_el->component()));
  REQUIRE(input_ptr != nullptr);
  input_ptr->disabled = true;
  input_ptr->Digest();

  Event::Mouse mouse;
  mouse.button = Event::Mouse::Button::Left;
  mouse.motion = Event::Mouse::Motion::Pressed;
  mouse.x = input_el->absolute_x() + 1;
  mouse.y = input_el->absolute_y() + 1;
  screen.Dispatch(Event(mouse));

  CHECK_FALSE(input_el->focused());
}

TEST_CASE("Input Component Readonly Blocks Mutation But Allows Selection "
          "And Copy",
          "[component][input][readonly]") {
  auto container = rtxui::Ref<InputTestComponent>::New();
  container->Mount();

  auto* input_el = container->Root()->QuerySelector("input");
  REQUIRE(input_el != nullptr);
  auto* input_ptr = dynamic_cast<rtxui::input*>(
      const_cast<rtxui::ComponentBase*>(input_el->component()));
  REQUIRE(input_ptr != nullptr);

  input_el->set_focused(true);
  input_ptr->readonly = true;

  // Typing is swallowed: the key is consumed, but the value is untouched.
  CHECK(input_ptr->OnEvent(Event::Keyboard::From('a')));
  input_ptr->Digest();
  CHECK(input_ptr->value == "hello world");

  // Backspace/Delete are no-ops too, still consumed.
  input_ptr->cursor_pos = 3;
  input_ptr->Digest();
  CHECK(input_ptr->OnEvent(Event::Backspace()));
  input_ptr->Digest();
  CHECK(input_ptr->value == "hello world");
  CHECK(input_ptr->OnEvent(Event::Delete()));
  input_ptr->Digest();
  CHECK(input_ptr->value == "hello world");

  // Navigation and shift-selection still work.
  input_ptr->cursor_pos = 0;
  input_ptr->selection_start = -1;
  input_ptr->Digest();
  Event::Keyboard kb;
  kb.special = Event::Keyboard::Special::ArrowRight;
  kb.modifier.shift = true;
  input_ptr->OnEvent(Event(kb));
  input_ptr->Digest();
  CHECK(input_ptr->cursor_pos == 1);
  CHECK(input_ptr->selection_start == 0);

  // Ctrl+X must not delete the selection under readonly.
  input_ptr->OnEvent(Event::CtrlX());
  input_ptr->Digest();
  CHECK(input_ptr->value == "hello world");
}

class DisabledReadonlyInputTestComponent
    : public rtxui::Component<DisabledReadonlyInputTestComponent> {
 public:
  void InitReflection() override {
    Import<rtxui::input>();
    rtxui::Component<DisabledReadonlyInputTestComponent>::InitReflection();
  }
  std::string_view view = R"(<input disabled="true" readonly="true" />)";
};

TEST_CASE("Input Component disabled/readonly Attributes Parsed From XML",
          "[component][input][disabled][readonly]") {
  auto container = rtxui::Ref<DisabledReadonlyInputTestComponent>::New();
  container->Mount();
  container->Digest();

  auto* input_el = container->Root()->QuerySelector("input");
  REQUIRE(input_el != nullptr);
  auto* input_ptr = dynamic_cast<rtxui::input*>(
      const_cast<rtxui::ComponentBase*>(input_el->component()));
  REQUIRE(input_ptr != nullptr);

  CHECK(input_ptr->disabled);
  CHECK(input_ptr->readonly);
}

TEST_CASE("Input Component :disabled pseudo-class dims the default style",
          "[component][input][disabled][style]") {
  auto container = rtxui::Ref<InputTestComponent>::New();
  container->Mount();

  auto* input_el = container->Root()->QuerySelector("input");
  REQUIRE(input_el != nullptr);

  container->ResolveTargetStyles();
  float enabled_opacity = input_el->target_style.opacity;

  input_el->set_disabled(true);
  container->ResolveTargetStyles();
  float disabled_opacity = input_el->target_style.opacity;

  CHECK(disabled_opacity < enabled_opacity);
}

TEST_CASE("Input Click Does Not Resize", "[component][input]") {
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  auto container = rtxui::Ref<InputTestComponent>::New();
  rtxui::Screen screen(container, device);
  screen.Draw();

  auto* input_el = container->Root()->QuerySelector("input");
  REQUIRE(input_el != nullptr);

  int initial_width = input_el->layout_width();
  int initial_height = input_el->layout_height();
  // The input should have a proper fixed size (width: 20 default).
  REQUIRE(initial_width > 0);
  REQUIRE(initial_height > 0);

  // Click at the input's position.
  Event::Mouse mouse;
  mouse.button = Event::Mouse::Button::Left;
  mouse.motion = Event::Mouse::Motion::Pressed;
  mouse.x = input_el->absolute_x() + 1;  // 1-based
  mouse.y = input_el->absolute_y() + 1;
  screen.Dispatch(Event(mouse));

  // Dimensions must not change after click (no resize-on-focus regression).
  CHECK(input_el->layout_width() == initial_width);
  CHECK(input_el->layout_height() == initial_height);
}

class TextareaTestComponent : public rtxui::Component<TextareaTestComponent> {
 public:
  std::string my_text = "line one\nline two\nline three";
  void InitReflection() override {
    Bind(my_text);
    Import<rtxui::textarea>();
    rtxui::Component<TextareaTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <textarea value="{my_text}" />
  )";
};

TEST_CASE("Textarea Click Does Not Resize", "[component][textarea]") {
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  auto container = rtxui::Ref<TextareaTestComponent>::New();
  rtxui::Screen screen(container, device);
  screen.Draw();

  auto* ta_el = container->Root()->QuerySelector("textarea");
  REQUIRE(ta_el != nullptr);

  int initial_width = ta_el->layout_width();
  int initial_height = ta_el->layout_height();
  // The textarea should have a proper fixed size (width: 40, height: 5
  // default).
  REQUIRE(initial_width > 0);
  REQUIRE(initial_height > 0);

  // Click at the textarea's position.
  Event::Mouse mouse;
  mouse.button = Event::Mouse::Button::Left;
  mouse.motion = Event::Mouse::Motion::Pressed;
  mouse.x = ta_el->absolute_x() + 1;  // 1-based
  mouse.y = ta_el->absolute_y() + 1;
  screen.Dispatch(Event(mouse));
  screen.Draw();

  // Dimensions must not change after click (no resize-on-focus regression).
  CHECK(ta_el->layout_width() == initial_width);
  CHECK(ta_el->layout_height() == initial_height);
}

TEST_CASE("Textarea Component Readonly Blocks Enter And Tab Indent",
          "[component][textarea][readonly]") {
  auto container = rtxui::Ref<TextareaTestComponent>::New();
  container->Mount();

  auto* textarea_el = container->Root()->QuerySelector("textarea");
  REQUIRE(textarea_el != nullptr);
  auto* textarea_ptr = dynamic_cast<rtxui::textarea*>(
      const_cast<rtxui::ComponentBase*>(textarea_el->component()));
  REQUIRE(textarea_ptr != nullptr);

  textarea_el->set_focused(true);
  textarea_ptr->readonly = true;
  textarea_ptr->cursor_pos = 0;
  textarea_ptr->Digest();
  std::string original = textarea_ptr->value;

  Event::Keyboard tab_kb;
  tab_kb.special = Event::Keyboard::Special::Tab;
  CHECK(textarea_ptr->OnEvent(Event(tab_kb)));
  textarea_ptr->Digest();
  CHECK(textarea_ptr->value == original);

  CHECK(textarea_ptr->OnEvent(Event::Return()));
  textarea_ptr->Digest();
  CHECK(textarea_ptr->value == original);
}

TEST_CASE("Textarea Component Disabled Blocks All Interaction",
          "[component][textarea][disabled]") {
  auto container = rtxui::Ref<TextareaTestComponent>::New();
  container->Mount();

  auto* textarea_el = container->Root()->QuerySelector("textarea");
  REQUIRE(textarea_el != nullptr);
  auto* textarea_ptr = dynamic_cast<rtxui::textarea*>(
      const_cast<rtxui::ComponentBase*>(textarea_el->component()));
  REQUIRE(textarea_ptr != nullptr);

  textarea_el->set_focused(true);
  textarea_ptr->disabled = true;
  std::string original = textarea_ptr->value;

  bool handled = textarea_ptr->OnEvent(Event::Keyboard::From('x'));
  textarea_ptr->Digest();
  CHECK_FALSE(handled);
  CHECK(textarea_ptr->value == original);
  CHECK_FALSE(textarea_el->focused());
}

TEST_CASE("Textarea Component Basic Typing", "[component][textarea]") {
  auto container = rtxui::Ref<TextareaTestComponent>::New();
  container->Mount();

  auto* ta_el = container->Root()->QuerySelector("textarea");
  REQUIRE(ta_el != nullptr);
  auto* ta_comp = const_cast<rtxui::ComponentBase*>(ta_el->component());
  REQUIRE(ta_comp != nullptr);
  auto* ta_ptr = dynamic_cast<rtxui::textarea*>(ta_comp);
  REQUIRE(ta_ptr != nullptr);

  CHECK(ta_ptr->value == "line one\nline two\nline three");
  CHECK(ta_ptr->cursor_pos == 0);

  ta_el->set_focused(true);

  // Type a character at position 0
  ta_ptr->OnEvent(Event::Keyboard::From('A'));
  ta_ptr->Digest();
  CHECK(ta_ptr->value == "Aline one\nline two\nline three");
  CHECK(ta_ptr->cursor_pos == 1);
}

TEST_CASE("Textarea Component Enter Key", "[component][textarea]") {
  auto container = rtxui::Ref<TextareaTestComponent>::New();
  container->Mount();

  auto* ta_el = container->Root()->QuerySelector("textarea");
  auto* ta_ptr = dynamic_cast<rtxui::textarea*>(
      const_cast<rtxui::ComponentBase*>(ta_el->component()));
  REQUIRE(ta_ptr != nullptr);

  ta_el->set_focused(true);
  ta_ptr->value = "hello world";
  ta_ptr->cursor_pos = 5;
  ta_ptr->Digest();

  // Press Enter in the middle — should split into two lines
  ta_ptr->OnEvent(Event::Return());
  ta_ptr->Digest();
  CHECK(ta_ptr->value == "hello\n world");
  CHECK(ta_ptr->cursor_pos == 6);
}

TEST_CASE("Textarea Component Enter Key Preserves Indentation",
          "[component][textarea]") {
  auto container = rtxui::Ref<TextareaTestComponent>::New();
  container->Mount();

  auto* ta_el = container->Root()->QuerySelector("textarea");
  auto* ta_ptr = dynamic_cast<rtxui::textarea*>(
      const_cast<rtxui::ComponentBase*>(ta_el->component()));
  REQUIRE(ta_ptr != nullptr);

  ta_el->set_focused(true);

  SECTION("spaces") {
    ta_ptr->value = "    int main() {";
    ta_ptr->cursor_pos = static_cast<int>(ta_ptr->value.size());
    ta_ptr->Digest();

    ta_ptr->OnEvent(Event::Return());
    ta_ptr->Digest();
    CHECK(ta_ptr->value == "    int main() {\n    ");
    CHECK(ta_ptr->cursor_pos == static_cast<int>(ta_ptr->value.size()));
  }

  SECTION("tabs") {
    ta_ptr->value = "\t\tif (true) {";
    ta_ptr->cursor_pos = static_cast<int>(ta_ptr->value.size());
    ta_ptr->Digest();

    ta_ptr->OnEvent(Event::Return());
    ta_ptr->Digest();
    CHECK(ta_ptr->value == "\t\tif (true) {\n\t\t");
    CHECK(ta_ptr->cursor_pos == static_cast<int>(ta_ptr->value.size()));
  }

  SECTION("no leading whitespace") {
    ta_ptr->value = "hello";
    ta_ptr->cursor_pos = static_cast<int>(ta_ptr->value.size());
    ta_ptr->Digest();

    ta_ptr->OnEvent(Event::Return());
    ta_ptr->Digest();
    CHECK(ta_ptr->value == "hello\n");
  }

  SECTION("only indentation carried, not trailing content") {
    ta_ptr->value = "  first line\n  second";
    ta_ptr->cursor_pos = static_cast<int>(ta_ptr->value.size());
    ta_ptr->Digest();

    ta_ptr->OnEvent(Event::Return());
    ta_ptr->Digest();
    CHECK(ta_ptr->value == "  first line\n  second\n  ");
  }
}

TEST_CASE("Textarea Component Arrow Up/Down Navigation",
          "[component][textarea]") {
  auto container = rtxui::Ref<TextareaTestComponent>::New();
  container->Mount();

  auto* ta_el = container->Root()->QuerySelector("textarea");
  auto* ta_ptr = dynamic_cast<rtxui::textarea*>(
      const_cast<rtxui::ComponentBase*>(ta_el->component()));
  REQUIRE(ta_ptr != nullptr);

  ta_el->set_focused(true);
  ta_ptr->value = "abc\ndefgh\nij";
  ta_ptr->cursor_pos = 0;
  ta_ptr->Digest();

  // Move right 2 chars on line 0 -> cursor at 'c', col=2
  ta_ptr->OnEvent(Event::ArrowRight());
  ta_ptr->OnEvent(Event::ArrowRight());
  ta_ptr->Digest();
  CHECK(ta_ptr->cursor_pos == 2);

  // ArrowDown should land on line 1, col 2 -> grapheme index 6 (4 for "abc\n" +
  // 2)
  ta_ptr->OnEvent(Event::ArrowDown());
  ta_ptr->Digest();
  CHECK(ta_ptr->cursor_pos == 6);  // "abc\n" = 4, then 2 more = 6

  // ArrowDown again -> line 2, col 2 -> grapheme index 11 (4 + 6 + 1 = 11?
  // "abc\n"=4, "defgh\n"=6, "ij"=2) "abc\n" = indices 0..3 (4 graphemes),
  // "defgh\n" = 4..9 (6 graphemes), "ij" = 10..11 (2) On line 2, col 2 -> index
  // 12 (past last character), clamped to 12 which equals size
  ta_ptr->OnEvent(Event::ArrowDown());
  ta_ptr->Digest();
  CHECK(ta_ptr->cursor_pos == 12);  // End of "ij" = 10 + 2 = 12

  // ArrowUp should go back to line 1, col 2 -> index 6
  ta_ptr->OnEvent(Event::ArrowUp());
  ta_ptr->Digest();
  CHECK(ta_ptr->cursor_pos == 6);
}

TEST_CASE("Textarea Component Home/End Keys", "[component][textarea]") {
  auto container = rtxui::Ref<TextareaTestComponent>::New();
  container->Mount();

  auto* ta_el = container->Root()->QuerySelector("textarea");
  auto* ta_ptr = dynamic_cast<rtxui::textarea*>(
      const_cast<rtxui::ComponentBase*>(ta_el->component()));
  REQUIRE(ta_ptr != nullptr);

  ta_el->set_focused(true);
  ta_ptr->value = "hello\nworld";
  ta_ptr->cursor_pos = 3;  // middle of "hello"
  ta_ptr->Digest();

  // Home -> goes to start of line 0 (index 0)
  ta_ptr->OnEvent(Event::Home());
  ta_ptr->Digest();
  CHECK(ta_ptr->cursor_pos == 0);

  // End -> goes to end of line 0 (index 5, before '\n')
  ta_ptr->OnEvent(Event::End());
  ta_ptr->Digest();
  CHECK(ta_ptr->cursor_pos == 5);

  // Move to line 1
  ta_ptr->cursor_pos = 8;  // middle of "world"
  ta_ptr->Digest();

  // Home on line 1 -> index 6 (after '\n')
  ta_ptr->OnEvent(Event::Home());
  ta_ptr->Digest();
  CHECK(ta_ptr->cursor_pos == 6);

  // End on line 1 -> index 11 (end of "world")
  ta_ptr->OnEvent(Event::End());
  ta_ptr->Digest();
  CHECK(ta_ptr->cursor_pos == 11);
}

TEST_CASE("Textarea Component Backspace/Delete", "[component][textarea]") {
  auto container = rtxui::Ref<TextareaTestComponent>::New();
  container->Mount();

  auto* ta_el = container->Root()->QuerySelector("textarea");
  auto* ta_ptr = dynamic_cast<rtxui::textarea*>(
      const_cast<rtxui::ComponentBase*>(ta_el->component()));
  REQUIRE(ta_ptr != nullptr);

  ta_el->set_focused(true);
  ta_ptr->value = "hello\nworld";
  ta_ptr->cursor_pos = 6;  // start of "world" (after '\n')
  ta_ptr->Digest();

  // Backspace at the beginning of line 1 joins the two lines
  ta_ptr->OnEvent(Event::Backspace());
  ta_ptr->Digest();
  CHECK(ta_ptr->value == "helloworld");
  CHECK(ta_ptr->cursor_pos == 5);
}

TEST_CASE("Textarea selection highlight actually renders (not just resolves "
          "in base_style)",
          "[component][textarea][regression]") {
  // Regression: the .selection span's background-color correctly resolved
  // in base_style, but was silently dropped at paint time by a
  // LayoutInlineFlow bug (see "Inline span background-color is applied
  // when painted"). Verify the real painted output, not just the resolved
  // style, for both a single-line selection and one spanning multiple
  // lines within the same <span>.
  auto container = rtxui::Ref<TextareaTestComponent>::New();
  container->Mount();

  auto* ta_el = container->Root()->QuerySelector("textarea");
  auto* ta_ptr = dynamic_cast<rtxui::textarea*>(
      const_cast<rtxui::ComponentBase*>(ta_el->component()));
  REQUIRE(ta_ptr != nullptr);
  ta_el->set_focused(true);

  // Checks the .selection span's painted background is distinctly the blue
  // highlight rather than the neutral gray textarea background -- not an
  // exact color match, since self's opacity is itself blended in at paint
  // time (its own :hover/:focus transition), which isn't what this
  // regression test is about. absolute_x()/absolute_y() are only populated
  // by a Paint() pass, so this paints once, then reads the position back
  // (rather than guessing an offset from padding, which both is easy to
  // get wrong and would still read stale pre-paint coordinates).
  auto SelectionBackgroundLooksHighlighted = [&]() {
    auto root_box = rtxui::LayoutTreeBuilder::Build(container->Root());
    rtxui::LayoutConstraints viewport = {
        {40, rtxui::MeasureMode::Exactly},
        {10, rtxui::MeasureMode::Exactly},
    };
    auto root_fragment = rtxui::RunLayout({root_box.get()}, viewport);
    Texture texture(40, 10);
    rtxui::Paint(root_fragment.get(), texture);
    auto* selection_el = container->Root()->QuerySelector(".selection");
    if (!selection_el) {
      return false;
    }
    Color bg =
        texture[selection_el->absolute_x(), selection_el->absolute_y()]
            .background_color;
    // The unselected background is a neutral gray (r == g == b); the blue
    // selection highlight is not.
    return bg.b > bg.r && bg.b > bg.g;
  };

  SECTION("single-line selection") {
    ta_ptr->value = "hello world";
    ta_ptr->selection_start = 2;
    ta_ptr->cursor_pos = 5;  // selects "llo"
    ta_ptr->Digest();

    CHECK(SelectionBackgroundLooksHighlighted());
  }

  SECTION("selection spanning multiple lines within one span") {
    ta_ptr->value = "line one\nline two\nline three";
    ta_ptr->selection_start = 2;
    ta_ptr->cursor_pos = 21;  // spans into "line three"
    ta_ptr->Digest();

    CHECK(SelectionBackgroundLooksHighlighted());
  }
}

class CheckboxTestComponent : public rtxui::Component<CheckboxTestComponent> {
 public:
  bool my_checked = false;
  bool onchange_called = false;

  void InitReflection() override {
    Bind(my_checked);
    Import<rtxui::checkbox>();
    Import("ToggleEnabled", [this]() { onchange_called = true; });
    rtxui::Component<CheckboxTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <checkbox checked="{my_checked}" onchange="ToggleEnabled">Click Me</checkbox>
  )";
};

TEST_CASE("Checkbox Component Basic Interactions", "[component][checkbox]") {
  auto container = rtxui::Ref<CheckboxTestComponent>::New();
  container->Mount();

  auto* cb_el = container->Root()->QuerySelector("checkbox");
  REQUIRE(cb_el != nullptr);
  auto* cb_comp = const_cast<rtxui::ComponentBase*>(cb_el->component());
  REQUIRE(cb_comp != nullptr);
  auto* cb_ptr = dynamic_cast<rtxui::checkbox*>(cb_comp);
  REQUIRE(cb_ptr != nullptr);

  // Initial state
  CHECK(cb_ptr->checked == false);
  CHECK(container->my_checked == false);
  CHECK(container->onchange_called == false);

  // Focus and trigger space
  cb_el->set_focused(true);
  cb_ptr->OnEvent(Event::Keyboard::From(' '));  // Space
  container->Digest();

  CHECK(cb_ptr->checked == true);
  CHECK(container->my_checked == true);
  CHECK(container->onchange_called == true);

  // Press space again
  container->onchange_called = false;
  cb_ptr->OnEvent(Event::Keyboard::From(' '));
  container->Digest();

  CHECK(cb_ptr->checked == false);
  CHECK(container->my_checked == false);
  CHECK(container->onchange_called == true);
}

class SliderTestComponent : public rtxui::Component<SliderTestComponent> {
 public:
  int my_val = 50;
  bool onchange_called = false;

  void InitReflection() override {
    Bind(my_val);
    Import<rtxui::slider>();
    Import("OnSliderChange", [this]() { onchange_called = true; });
    rtxui::Component<SliderTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <slider value="{my_val}" min="0" max="100" step="10" width="11" onchange="OnSliderChange" />
  )";
};

TEST_CASE("Slider Component Basic Interactions", "[component][slider]") {
  auto container = rtxui::Ref<SliderTestComponent>::New();
  container->Mount();

  auto* slider_el = container->Root()->QuerySelector("slider");
  REQUIRE(slider_el != nullptr);
  auto* slider_comp = const_cast<rtxui::ComponentBase*>(slider_el->component());
  REQUIRE(slider_comp != nullptr);
  auto* slider_ptr = dynamic_cast<rtxui::slider*>(slider_comp);
  REQUIRE(slider_ptr != nullptr);

  // Initial state (value = 50)
  CHECK(slider_ptr->value == 50);
  CHECK(container->my_val == 50);
  CHECK(container->onchange_called == false);

  // Focus and trigger arrow keys
  slider_el->set_focused(true);
  slider_ptr->OnEvent(Event::ArrowRight());
  container->Digest();

  // Value should increase by step (10)
  CHECK(slider_ptr->value == 60);
  CHECK(container->my_val == 60);
  CHECK(container->onchange_called == true);

  // Press ArrowLeft
  container->onchange_called = false;
  slider_ptr->OnEvent(Event::ArrowLeft());
  container->Digest();

  CHECK(slider_ptr->value == 50);
  CHECK(container->my_val == 50);
  CHECK(container->onchange_called == true);
}

TEST_CASE("Slider Component Mouse Drag and Capture", "[component][slider]") {
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  auto container = rtxui::Ref<SliderTestComponent>::New();
  rtxui::Screen screen(container, device);
  screen.Draw();

  auto* slider_el = container->Root()->QuerySelector("slider");
  REQUIRE(slider_el != nullptr);
  auto* slider_comp = const_cast<rtxui::ComponentBase*>(slider_el->component());
  REQUIRE(slider_comp != nullptr);
  auto* slider_ptr = dynamic_cast<rtxui::slider*>(slider_comp);
  REQUIRE(slider_ptr != nullptr);

  // width=11, min=0, max=100, step=10. Root() is an inline <span> whose
  // layout_width_ is 0; use the width attribute directly for coordinates.
  int abs_x = slider_ptr->Root()->absolute_x();  // 0 in mock terminal
  int abs_y = slider_ptr->Root()->absolute_y();  // 0 in mock terminal
  int track_w = std::max(2, slider_ptr->width);  // 11

  // Press at the leftmost column of the track (1-based mouse coords)
  {
    Event::Mouse mouse;
    mouse.button = Event::Mouse::Button::Left;
    mouse.motion = Event::Mouse::Motion::Pressed;
    mouse.x = abs_x + 2;  // 1-based: column 1 in 0-indexed (start of track)
    mouse.y = abs_y + 1;
    screen.Dispatch(Event(mouse));
  }

  // Value should be 0 (leftmost) and capture active
  CHECK(slider_ptr->value == 0);
  CHECK(rtxui::ComponentBase::GetMouseCapturer() == slider_ptr);

  // Move to the centre of the track — y deliberately far off (capture ignores
  // it)
  {
    Event::Mouse mouse;
    mouse.button = Event::Mouse::Button::Left;
    mouse.motion = Event::Mouse::Motion::Moved;
    mouse.x = abs_x + (track_w / 2) + 2;  // middle column, 1-based
    mouse.y = abs_y + 50;
    screen.Dispatch(Event(mouse));
  }

  // Value should be 50 and capture still held
  CHECK(slider_ptr->value == 50);
  CHECK(rtxui::ComponentBase::GetMouseCapturer() == slider_ptr);

  // Release at the rightmost column
  {
    Event::Mouse mouse;
    mouse.button = Event::Mouse::Button::Left;
    mouse.motion = Event::Mouse::Motion::Released;
    mouse.x = abs_x + track_w + 1;  // last column, 1-based
    mouse.y = abs_y + 1;
    screen.Dispatch(Event(mouse));
  }

  // Value should be 100 and capture released
  CHECK(slider_ptr->value == 100);
  CHECK(rtxui::ComponentBase::GetMouseCapturer() == nullptr);
}

class ProgressTestComponent : public rtxui::Component<ProgressTestComponent> {
 public:
  double my_progress = 25.0;

  void InitReflection() override {
    Bind(my_progress);
    Import<rtxui::progress>();
    rtxui::Component<ProgressTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <progress value="{my_progress}" max="100" width="10" />
  )";
};

TEST_CASE("Progress Component Basic Rendering", "[component][progress]") {
  auto container = rtxui::Ref<ProgressTestComponent>::New();
  container->Mount();
  container->Digest();

  auto* progress_el = container->Root()->QuerySelector("progress");
  REQUIRE(progress_el != nullptr);
  auto* progress_comp =
      const_cast<rtxui::ComponentBase*>(progress_el->component());
  REQUIRE(progress_comp != nullptr);
  auto* progress_ptr = dynamic_cast<rtxui::progress*>(progress_comp);
  REQUIRE(progress_ptr != nullptr);

  // Initial state (value = 25.0) -> width is 10, so 2.5 rounded to 3 characters
  // filled.
  CHECK(progress_ptr->value == 25.0);
  CHECK(progress_ptr->filled_track == "███");
  CHECK(progress_ptr->empty_track == "       ");  // 7 spaces

  // Update value
  container->my_progress = 70.0;
  container->Digest();

  // Value = 70.0 -> 7 characters filled.
  CHECK(progress_ptr->value == 70.0);
  CHECK(progress_ptr->filled_track == "███████");
  CHECK(progress_ptr->empty_track == "   ");  // 3 spaces
}

class SelectTestComponent : public rtxui::Component<SelectTestComponent> {
 public:
  std::string my_theme = "light";

  void InitReflection() override {
    Bind(my_theme);
    Import<rtxui::select>();
    Import<rtxui::option>();
    rtxui::Component<SelectTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <select value="{my_theme}">
      <option value="dark">Dark Theme</option>
      <option value="light">Light Theme</option>
      <option value="solarized">Solarized</option>
    </select>
  )";
};

TEST_CASE("Select and Option Components", "[component][select]") {
  auto container = rtxui::Ref<SelectTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* select_el = container->Root()->QuerySelector("select");
  REQUIRE(select_el != nullptr);
  auto* select_comp = const_cast<rtxui::ComponentBase*>(select_el->component());
  REQUIRE(select_comp != nullptr);
  auto* select_ptr = dynamic_cast<rtxui::select*>(select_comp);
  REQUIRE(select_ptr != nullptr);

  // Check initial state
  std::cout << "DOM TREE:\n" << container->Root()->Print() << std::endl;
  std::cout << "select_el absolute_x=" << select_el->absolute_x()
            << " absolute_y=" << select_el->absolute_y()
            << " width=" << select_el->layout_width()
            << " height=" << select_el->layout_height() << std::endl;
  std::cout << "select_ptr->Root() absolute_x="
            << select_ptr->Root()->absolute_x()
            << " absolute_y=" << select_ptr->Root()->absolute_y()
            << " width=" << select_ptr->Root()->layout_width()
            << " height=" << select_ptr->Root()->layout_height() << std::endl;
  CHECK(select_ptr->value == "light");
  CHECK(select_ptr->selected_label == "Light Theme");
  CHECK(select_ptr->is_open == false);

  // Click on the select element to open it
  select_el->set_focused(true);
  Event::Mouse mouse_click;
  mouse_click.button = Event::Mouse::Button::Left;
  mouse_click.motion = Event::Mouse::Motion::Pressed;
  mouse_click.x = select_el->absolute_x() + 2;
  mouse_click.y = select_el->absolute_y() + 1;
  Event click_event(mouse_click);
  CHECK(select_ptr->OnEvent(click_event) == true);
  container->Digest();
  screen.Draw();
  CHECK(select_ptr->is_open == true);
  CHECK(select_ptr->hovered_index == 1);  // "light" is at index 1

  // Use keyboard: ArrowDown to "solarized" (index 2)
  Event down_event = Event::Keyboard({
      Event::Keyboard::Motion::Pressed,
      Event::Keyboard::Special::ArrowDown,
  });
  CHECK(select_ptr->OnEvent(down_event) == true);
  container->Digest();
  screen.Draw();
  CHECK(select_ptr->hovered_index == 2);

  // Use keyboard: Enter to select "solarized"
  Event enter_event = Event::Keyboard({
      Event::Keyboard::Motion::Pressed,
      Event::Keyboard::Special::Return,
  });
  CHECK(select_ptr->OnEvent(enter_event) == true);
  container->Digest();
  screen.Draw();

  // Verify states after selection
  CHECK(select_ptr->is_open == false);
  CHECK(select_ptr->value == "solarized");
  CHECK(container->my_theme == "solarized");
  CHECK(select_ptr->selected_label == "Solarized");

  // Re-open the select to make the option elements visible and laid out
  Event::Mouse reopen_click;
  reopen_click.button = Event::Mouse::Button::Left;
  reopen_click.motion = Event::Mouse::Motion::Pressed;
  reopen_click.x = select_el->absolute_x() + 2;
  reopen_click.y = select_el->absolute_y() + 1;
  Event reopen_event(reopen_click);
  CHECK(select_ptr->OnEvent(reopen_event) == true);
  container->Digest();
  screen.Draw();
  CHECK(select_ptr->is_open == true);

  // Verify option click selection: click on "dark" option (index 0)
  // Let's find the "dark" option element
  auto* dark_option_el = container->Root()->QuerySelector("option");
  REQUIRE(dark_option_el != nullptr);
  auto* dark_option_comp =
      const_cast<rtxui::ComponentBase*>(dark_option_el->component());
  REQUIRE(dark_option_comp != nullptr);
  auto* dark_option_ptr = dynamic_cast<rtxui::option*>(dark_option_comp);
  REQUIRE(dark_option_ptr != nullptr);
  CHECK(dark_option_ptr->value == "dark");

  Event::Mouse option_mouse_click;
  option_mouse_click.button = Event::Mouse::Button::Left;
  option_mouse_click.motion = Event::Mouse::Motion::Pressed;
  option_mouse_click.x = dark_option_el->absolute_x() + 1;
  option_mouse_click.y = dark_option_el->absolute_y() + 1;
  Event option_click_event(option_mouse_click);
  // Send click to option via container propagation
  CHECK(container->OnEvent(option_click_event) == true);
  container->Digest();
  screen.Draw();

  // Check state has been updated to "dark"
  CHECK(select_ptr->value == "dark");
  CHECK(container->my_theme == "dark");
  CHECK(select_ptr->selected_label == "Dark Theme");
}

TEST_CASE("Select Component Positioning and Mouse Hover Alignment", "[component][select][position][hover]") {
  auto container = rtxui::Ref<SelectTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* select_el = container->Root()->QuerySelector("select");
  REQUIRE(select_el != nullptr);
  auto* select_comp = const_cast<rtxui::ComponentBase*>(select_el->component());
  REQUIRE(select_comp != nullptr);
  auto* select_ptr = dynamic_cast<rtxui::select*>(select_comp);
  REQUIRE(select_ptr != nullptr);

  // Click on the select element to open it
  select_el->set_focused(true);
  Event::Mouse mouse_click;
  mouse_click.button = Event::Mouse::Button::Left;
  mouse_click.motion = Event::Mouse::Motion::Pressed;
  mouse_click.x = select_el->absolute_x() + 2;
  mouse_click.y = select_el->absolute_y() + 1;
  Event click_event(mouse_click);
  CHECK(select_ptr->OnEvent(click_event) == true);
  container->Digest();
  screen.Draw();
  std::cout << "TEST INFO: select_el pointer=" << select_el
            << " select_ptr->Root()=" << select_ptr->Root()
            << " select_el->classes=";
  for (const auto& c : select_el->classes) std::cout << c << " ";
  std::cout << "\nTEST INFO: z_index value=" << select_el->base_style.z_index.value_or(-99)
            << " has_val=" << select_el->base_style.z_index.has_value() << std::endl;
  CHECK(select_el->base_style.z_index.value_or(0) == 1000);

  // Verify the absolute coordinates of the dropdown-list
  auto* dropdown_el = container->Root()->QuerySelector(".dropdown-list");
  REQUIRE(dropdown_el != nullptr);
  CHECK(dropdown_el->absolute_x() == select_el->absolute_x());
  CHECK(dropdown_el->absolute_y() == select_el->absolute_y() + select_el->layout_height());

  auto options = select_ptr->GetOptions();
  REQUIRE(options.size() == 3);
  auto* opt0 = options[0].element;
  auto* opt1 = options[1].element;

  // Initially "light" (opt1) is selected and hovered, and "dark" (opt0) is not.
  CHECK(opt1->base_style.background_color.value_or(Color()) == Color::RGB(59, 130, 246));
  CHECK(!opt0->base_style.background_color.has_value());

  // Verify options are hoverable by moving mouse
  auto* dark_option_el = container->Root()->QuerySelector("option"); // first option is "dark" at index 0
  REQUIRE(dark_option_el != nullptr);
  int dark_x = dark_option_el->absolute_x();
  int dark_y = dark_option_el->absolute_y();

  Event::Mouse mouse_move;
  mouse_move.button = Event::Mouse::Button::None;
  mouse_move.motion = Event::Mouse::Motion::Moved;
  mouse_move.x = dark_x + 1; // 1-indexed for screen
  mouse_move.y = dark_y + 1; // 1-indexed for screen
  Event move_event(mouse_move);

  CHECK(select_ptr->OnEvent(move_event) == true);
  container->Digest();
  screen.Draw();

  // hovered_index should now be 0 ("dark")
  CHECK(select_ptr->hovered_index == 0);

  // Now "dark" (opt0) is hovered (blue) and "light" (opt1) is only selected (slate)
  CHECK(opt0->base_style.background_color.value_or(Color()) == Color::RGB(59, 130, 246));
  CHECK(opt1->base_style.background_color.value_or(Color()) == Color::RGB(51, 65, 85));
}

class SelectZIndexTestComponent : public rtxui::Component<SelectZIndexTestComponent> {
 public:
  std::string my_theme = "light";

  void InitReflection() override {
    Bind(my_theme);
    Import<rtxui::select>();
    Import<rtxui::option>();
    rtxui::Component<SelectZIndexTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <div style="display: block; width: 40; height: 10;">
      <select value="{my_theme}">
        <option value="dark">Dark Theme</option>
        <option value="light">Light Theme</option>
        <option value="solarized">Solarized</option>
      </select>
      <div id="sibling" style="display: block; width: 40; height: 5;">
        SIBLING_CONTENT_LINE_1
        SIBLING_CONTENT_LINE_2
      </div>
    </div>
  )";
};

TEST_CASE("Select Component Z-index sibling overlap", "[component][select][z-index]") {
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  device->TriggerResize(40, 10);

  auto container = rtxui::Ref<SelectZIndexTestComponent>::New();
  rtxui::Screen screen(container, device);
  screen.Draw();

  auto* select_el = container->Root()->QuerySelector("select");
  REQUIRE(select_el != nullptr);
  auto* select_comp = const_cast<rtxui::ComponentBase*>(select_el->component());
  REQUIRE(select_comp != nullptr);
  auto* select_ptr = dynamic_cast<rtxui::select*>(select_comp);
  REQUIRE(select_ptr != nullptr);

  // Click on the select element to open it
  select_el->set_focused(true);
  Event::Mouse mouse_click;
  mouse_click.button = Event::Mouse::Button::Left;
  mouse_click.motion = Event::Mouse::Motion::Pressed;
  mouse_click.x = select_el->absolute_x() + 2;
  mouse_click.y = select_el->absolute_y() + 1;
  Event click_event(mouse_click);
  CHECK(select_ptr->OnEvent(click_event) == true);
  container->Digest();
  device->ClearOutput();
  screen.Draw();

  std::string output = device->GetOutput();

  // If z-index is correct, the dropdown should overlay the sibling.
  // The first option in the dropdown is "Dark Theme", which should render at absolute_y = 1
  // (covering "SIBLING_CONTENT_LINE_1" which also starts at absolute_y = 1).
  // Therefore, "Dark Theme" must be present in the output, and "SIBLING_CONTENT_LINE_1" must NOT.
  CHECK(output.find("Dark Theme") != std::string::npos);
  CHECK(output.find("SIBLING_CONTENT_LINE_1") == std::string::npos);
}

class HrTestComponent : public rtxui::Component<HrTestComponent> {
 public:
  void InitReflection() override {
    Import<rtxui::hr>();
    rtxui::Component<HrTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <hr />
  )";
};

TEST_CASE("Horizontal Rule Component", "[component][hr]") {
  auto container = rtxui::Ref<HrTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* hr_el = container->Root()->QuerySelector("hr");
  REQUIRE(hr_el != nullptr);
  auto* hr_comp = const_cast<rtxui::ComponentBase*>(hr_el->component());
  REQUIRE(hr_comp != nullptr);
  auto* hr_ptr = dynamic_cast<rtxui::hr*>(hr_comp);
  REQUIRE(hr_ptr != nullptr);

  container->Digest();
  screen.Draw();

  CHECK(hr_ptr->line_chars.size() > 0);
  CHECK(hr_ptr->line_chars.substr(0, 3) == "─");
}

class BoldTestComponent : public rtxui::Component<BoldTestComponent> {
 public:
  void InitReflection() override {
    Import<rtxui::b>();
    Import<rtxui::strong>();
    rtxui::Component<BoldTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <div>
      <b>Bold text</b>
      <strong>Strong text</strong>
    </div>
  )";
};

TEST_CASE("Bold and Strong Components", "[component][b][strong]") {
  auto container = rtxui::Ref<BoldTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* b_el = container->Root()->QuerySelector("b");
  REQUIRE(b_el != nullptr);
  auto* b_comp = const_cast<rtxui::ComponentBase*>(b_el->component());
  REQUIRE(b_comp != nullptr);
  auto* b_ptr = dynamic_cast<rtxui::b*>(b_comp);
  REQUIRE(b_ptr != nullptr);

  auto* strong_el = container->Root()->QuerySelector("strong");
  REQUIRE(strong_el != nullptr);
  auto* strong_comp = const_cast<rtxui::ComponentBase*>(strong_el->component());
  REQUIRE(strong_comp != nullptr);
  auto* strong_ptr = dynamic_cast<rtxui::strong*>(strong_comp);
  REQUIRE(strong_ptr != nullptr);
}

TEST_CASE("Bold and Strong Components cell.bold rendering",
          "[component][b][strong][paint]") {
  auto container = rtxui::Ref<BoldTestComponent>::New();
  container->Mount();

  auto root_box = rtxui::LayoutTreeBuilder::Build(container->Root());
  REQUIRE(root_box != nullptr);

  rtxui::LayoutConstraints viewport = {
      {80, rtxui::MeasureMode::Exactly},
      {24, rtxui::MeasureMode::Exactly},
  };
  auto root_fragment = rtxui::RunLayout({root_box.get()}, viewport);
  REQUIRE(root_fragment != nullptr);

  Texture texture(80, 24);
  rtxui::Paint(root_fragment.get(), texture);

  bool found_bold = false;
  bool found_strong = false;
  for (int y = 0; y < texture.height(); ++y) {
    for (int x = 0; x < texture.width(); ++x) {
      const auto& cell = texture[x, y];
      if (cell.character == "B") {
        if (x + 8 < texture.width()) {
          std::string text = "";
          bool all_bold = true;
          for (int i = 0; i < 9; ++i) {
            text += texture[x + i, y].character;
            if (!texture[x + i, y].bold) {
              all_bold = false;
            }
          }
          if (text == "Bold text") {
            CHECK(all_bold);
            found_bold = true;
          }
        }
      }
      if (cell.character == "S") {
        if (x + 10 < texture.width()) {
          std::string text = "";
          bool all_bold = true;
          for (int i = 0; i < 11; ++i) {
            text += texture[x + i, y].character;
            if (!texture[x + i, y].bold) {
              all_bold = false;
            }
          }
          if (text == "Strong text") {
            CHECK(all_bold);
            found_strong = true;
          }
        }
      }
    }
  }
  CHECK(found_bold);
  CHECK(found_strong);

  std::string rendered = texture.Render();
  CHECK(rendered.find("\x1B[1m") != std::string::npos);
  CHECK(rendered.find("\x1B[22m") != std::string::npos);
}

class ItalicTestComponent : public rtxui::Component<ItalicTestComponent> {
 public:
  void InitReflection() override {
    Import<rtxui::i>();
    Import<rtxui::em>();
    rtxui::Component<ItalicTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <div>
      <i>Italic text</i>
      <em>Em text</em>
    </div>
  )";
};

TEST_CASE("Italic and Em Components cell.italic rendering",
          "[component][i][em][paint]") {
  auto container = rtxui::Ref<ItalicTestComponent>::New();
  container->Mount();

  auto root_box = rtxui::LayoutTreeBuilder::Build(container->Root());
  REQUIRE(root_box != nullptr);

  rtxui::LayoutConstraints viewport = {
      {80, rtxui::MeasureMode::Exactly},
      {24, rtxui::MeasureMode::Exactly},
  };
  auto root_fragment = rtxui::RunLayout({root_box.get()}, viewport);
  REQUIRE(root_fragment != nullptr);

  Texture texture(80, 24);
  rtxui::Paint(root_fragment.get(), texture);

  auto find_italic_run = [&](const std::string& expected) {
    for (int y = 0; y < texture.height(); ++y) {
      for (int x = 0; x < texture.width(); ++x) {
        if (texture[x, y].character != expected.substr(0, 1)) {
          continue;
        }
        std::string text = "";
        bool all_italic = true;
        for (size_t c = 0; c < expected.size(); ++c) {
          text += texture[x + static_cast<int>(c), y].character;
          if (!texture[x + static_cast<int>(c), y].italic) {
            all_italic = false;
          }
        }
        if (text == expected) {
          CHECK(all_italic);
          return true;
        }
      }
    }
    return false;
  };

  CHECK(find_italic_run("Italic text"));
  CHECK(find_italic_run("Em text"));

  std::string rendered = texture.Render();
  CHECK(rendered.find("\x1B[3m") != std::string::npos);
  CHECK(rendered.find("\x1B[23m") != std::string::npos);
}

class PseudoVarTestComponent
    : public rtxui::Component<PseudoVarTestComponent> {
 public:
  void InitReflection() override {
    Import<rtxui::div>();
    rtxui::Component<PseudoVarTestComponent>::InitReflection();
  }
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

class ImportantTestComponent
    : public rtxui::Component<ImportantTestComponent> {
 public:
  void InitReflection() override {
    Import<rtxui::div>();
    rtxui::Component<ImportantTestComponent>::InitReflection();
  }
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
  void InitReflection() override {
    Import<rtxui::div>();
    Import<rtxui::span>();
    rtxui::Component<VarTestComponent>::InitReflection();
  }
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

class DimTestComponent : public rtxui::Component<DimTestComponent> {
 public:
  void InitReflection() override {
    Import<rtxui::div>();
    rtxui::Component<DimTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <div>
      <div class="light">Dim text</div>
    </div>
    <style>
      .light { font-weight: lighter; }
    </style>
  )";
};

TEST_CASE("Dim text rendering via font-weight lighter",
          "[component][font-weight][paint]") {
  auto container = rtxui::Ref<DimTestComponent>::New();
  container->Mount();

  auto root_box = rtxui::LayoutTreeBuilder::Build(container->Root());
  REQUIRE(root_box != nullptr);

  rtxui::LayoutConstraints viewport = {
      {80, rtxui::MeasureMode::Exactly},
      {24, rtxui::MeasureMode::Exactly},
  };
  auto root_fragment = rtxui::RunLayout({root_box.get()}, viewport);
  REQUIRE(root_fragment != nullptr);

  Texture texture(80, 24);
  rtxui::Paint(root_fragment.get(), texture);

  bool found = false;
  const std::string expected = "Dim text";
  for (int y = 0; y < texture.height() && !found; ++y) {
    for (int x = 0; x < texture.width() && !found; ++x) {
      std::string text = "";
      bool all_dim = true;
      for (size_t c = 0; c < expected.size(); ++c) {
        text += texture[x + static_cast<int>(c), y].character;
        if (!texture[x + static_cast<int>(c), y].dim) {
          all_dim = false;
        }
      }
      if (text == expected) {
        CHECK(all_dim);
        found = true;
      }
    }
  }
  CHECK(found);

  std::string rendered = texture.Render();
  CHECK(rendered.find("\x1B[2m") != std::string::npos);
}

class TextTransformTestComponent
    : public rtxui::Component<TextTransformTestComponent> {
 public:
  void InitReflection() override {
    Import<rtxui::div>();
    Import<rtxui::span>();
    rtxui::Component<TextTransformTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <div>
      <div class="up">hello wörld</div>
      <div class="down">QUIET Text</div>
      <div class="cap">two words</div>
      <div class="up"><span>inherited text</span></div>
    </div>
    <style>
      .up { text-transform: uppercase; }
      .down { text-transform: lowercase; }
      .cap { text-transform: capitalize; }
    </style>
  )";
};

TEST_CASE("Text transform rendering", "[component][text-transform][paint]") {
  auto container = rtxui::Ref<TextTransformTestComponent>::New();
  container->Mount();

  auto root_box = rtxui::LayoutTreeBuilder::Build(container->Root());
  REQUIRE(root_box != nullptr);

  rtxui::LayoutConstraints viewport = {
      {80, rtxui::MeasureMode::Exactly},
      {24, rtxui::MeasureMode::Exactly},
  };
  auto root_fragment = rtxui::RunLayout({root_box.get()}, viewport);
  REQUIRE(root_fragment != nullptr);

  Texture texture(80, 24);
  rtxui::Paint(root_fragment.get(), texture);

  std::string all_text;
  for (int y = 0; y < texture.height(); ++y) {
    for (int x = 0; x < texture.width(); ++x) {
      all_text += texture[x, y].character;
    }
    all_text += "\n";
  }

  // Uppercase transforms ASCII letters; multi-byte UTF-8 passes through.
  CHECK(all_text.find("HELLO WöRLD") != std::string::npos);
  CHECK(all_text.find("quiet text") != std::string::npos);
  CHECK(all_text.find("Two Words") != std::string::npos);
  // The transform inherits into nested inline elements.
  CHECK(all_text.find("INHERITED TEXT") != std::string::npos);
}

class LetterSpacingTestComponent
    : public rtxui::Component<LetterSpacingTestComponent> {
 public:
  void InitReflection() override {
    Import<rtxui::div>();
    Import<rtxui::span>();
    rtxui::Component<LetterSpacingTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <div>
      <div class="spaced">ab cd</div>
      <div class="wide">xy</div>
      <div class="spaced"><span>ef</span></div>
    </div>
    <style>
      .spaced { letter-spacing: 1; }
      .wide { letter-spacing: 2; }
    </style>
  )";
};

TEST_CASE("Letter spacing rendering", "[component][letter-spacing][paint]") {
  auto container = rtxui::Ref<LetterSpacingTestComponent>::New();
  container->Mount();

  auto root_box = rtxui::LayoutTreeBuilder::Build(container->Root());
  REQUIRE(root_box != nullptr);

  rtxui::LayoutConstraints viewport = {
      {80, rtxui::MeasureMode::Exactly},
      {24, rtxui::MeasureMode::Exactly},
  };
  auto root_fragment = rtxui::RunLayout({root_box.get()}, viewport);
  REQUIRE(root_fragment != nullptr);

  Texture texture(80, 24);
  rtxui::Paint(root_fragment.get(), texture);

  std::string all_text;
  for (int y = 0; y < texture.height(); ++y) {
    for (int x = 0; x < texture.width(); ++x) {
      all_text += texture[x, y].character;
    }
    all_text += "\n";
  }

  // One non-breaking space (U+00A0) between every pair of grapheme
  // clusters, including around the (still breakable) ASCII word gap.
  CHECK(all_text.find("a\xC2\xA0"
                      "b\xC2\xA0 \xC2\xA0"
                      "c\xC2\xA0"
                      "d") != std::string::npos);
  // letter-spacing: 2 inserts two cells.
  CHECK(all_text.find("x\xC2\xA0\xC2\xA0y") != std::string::npos);
  // The spacing inherits into nested inline elements.
  CHECK(all_text.find("e\xC2\xA0"
                      "f") != std::string::npos);
}

class LineHeightTestComponent
    : public rtxui::Component<LineHeightTestComponent> {
 public:
  void InitReflection() override {
    Import<rtxui::div>();
    rtxui::Component<LineHeightTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <div class="lh">first
second</div>
    <style>
      .lh { line-height: 2; white-space: pre; }
    </style>
  )";
};

TEST_CASE("Line height rendering", "[component][line-height][paint]") {
  auto container = rtxui::Ref<LineHeightTestComponent>::New();
  container->Mount();

  auto root_box = rtxui::LayoutTreeBuilder::Build(container->Root());
  REQUIRE(root_box != nullptr);

  rtxui::LayoutConstraints viewport = {
      {80, rtxui::MeasureMode::Exactly},
      {24, rtxui::MeasureMode::Exactly},
  };
  auto root_fragment = rtxui::RunLayout({root_box.get()}, viewport);
  REQUIRE(root_fragment != nullptr);

  Texture texture(80, 24);
  rtxui::Paint(root_fragment.get(), texture);

  auto row_text = [&](int y) {
    std::string row;
    for (int x = 0; x < texture.width(); ++x) {
      row += texture[x, y].character;
    }
    return row;
  };

  int first_row = -1;
  int second_row = -1;
  for (int y = 0; y < texture.height(); ++y) {
    if (row_text(y).find("first") != std::string::npos) {
      first_row = y;
    }
    if (row_text(y).find("second") != std::string::npos) {
      second_row = y;
    }
  }
  REQUIRE(first_row != -1);
  REQUIRE(second_row != -1);
  // line-height: 2 makes each line box two rows tall: one blank row
  // separates consecutive lines.
  CHECK(second_row - first_row == 2);
}

class OverflowWrapTestComponent
    : public rtxui::Component<OverflowWrapTestComponent> {
 public:
  void InitReflection() override {
    Import<rtxui::div>();
    rtxui::Component<OverflowWrapTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <div>
      <div class="break">abcdefghijklmno</div>
      <div class="keep">abcdefghijklmno</div>
    </div>
    <style>
      .break, .keep { width: 6; }
      .keep { overflow-wrap: normal; }
    </style>
  )";
};

TEST_CASE("Overflow wrap rendering", "[component][overflow-wrap][paint]") {
  auto container = rtxui::Ref<OverflowWrapTestComponent>::New();
  container->Mount();

  auto root_box = rtxui::LayoutTreeBuilder::Build(container->Root());
  REQUIRE(root_box != nullptr);

  rtxui::LayoutConstraints viewport = {
      {80, rtxui::MeasureMode::Exactly},
      {24, rtxui::MeasureMode::Exactly},
  };
  auto root_fragment = rtxui::RunLayout({root_box.get()}, viewport);
  REQUIRE(root_fragment != nullptr);

  Texture texture(80, 24);
  rtxui::Paint(root_fragment.get(), texture);

  auto row_text = [&](int y) {
    std::string row;
    for (int x = 0; x < texture.width(); ++x) {
      row += texture[x, y].character;
    }
    while (!row.empty() && row.back() == ' ') {
      row.pop_back();
    }
    return row;
  };

  // Default: the word emergency-breaks at the container edge, and each
  // line stays within the specified width (regression: the break used to
  // land one column too late).
  CHECK(row_text(0) == "abcdef");
  CHECK(row_text(1) == "ghijkl");
  CHECK(row_text(2) == "mno");
  // overflow-wrap: normal keeps the word intact and lets it overflow.
  CHECK(row_text(3) == "abcdefghijklmno");
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

class StyleDecorationTestComponent
    : public rtxui::Component<StyleDecorationTestComponent> {
 public:
  void InitReflection() override {
    rtxui::Component<StyleDecorationTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <box>
      <item id="u">u</item>
      <item id="ud">ud</item>
      <item id="s">s</item>
      <item id="bl">bl</item>
    </box>
    <style>
      box { display: block; }
      item { display: inline; }
      #u { text-decoration: underline; }
      #ud { text-decoration: double-underline; }
      #s { text-decoration: strikethrough; }
      #bl { text-decoration: blink; }
    </style>
  )";
};

TEST_CASE("Text Decoration rendering onto cells",
          "[component][style][paint][decoration]") {
  auto container = rtxui::Ref<StyleDecorationTestComponent>::New();
  container->Mount();

  auto root_box = rtxui::LayoutTreeBuilder::Build(container->Root());
  REQUIRE(root_box != nullptr);

  rtxui::LayoutConstraints viewport = {
      {80, rtxui::MeasureMode::Exactly},
      {24, rtxui::MeasureMode::Exactly},
  };
  auto root_fragment = rtxui::RunLayout({root_box.get()}, viewport);
  REQUIRE(root_fragment != nullptr);

  Texture texture(80, 24);
  rtxui::Paint(root_fragment.get(), texture);

  bool found_u = false;
  bool found_ud = false;
  bool found_s = false;
  bool found_bl = false;

  for (int y = 0; y < texture.height(); ++y) {
    for (int x = 0; x < texture.width(); ++x) {
      const auto& cell = texture[x, y];
      if (cell.character == "u" && x + 1 < texture.width() &&
          texture[x + 1, y].character != "d") {
        CHECK(cell.underlined);
        CHECK_FALSE(cell.underlined_double);
        found_u = true;
      }
      if (cell.character == "u" && x + 1 < texture.width() &&
          texture[x + 1, y].character == "d") {
        CHECK_FALSE(cell.underlined);
        CHECK(cell.underlined_double);
        CHECK(texture[x + 1, y].underlined_double);
        found_ud = true;
      }
      if (cell.character == "s") {
        CHECK(cell.strikethrough);
        found_s = true;
      }
      if (cell.character == "b" && x + 1 < texture.width() &&
          texture[x + 1, y].character == "l") {
        CHECK(cell.blink);
        CHECK(texture[x + 1, y].blink);
        found_bl = true;
      }
    }
  }
  CHECK(found_u);
  CHECK(found_ud);
  CHECK(found_s);
  CHECK(found_bl);
}

class MarginAutoTestComponent
    : public rtxui::Component<MarginAutoTestComponent> {
 public:
  void InitReflection() override {
    rtxui::Component<MarginAutoTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <box id="parent">
      <box id="child">centered</box>
    </box>
    <style>
      #parent {
        display: block;
        width: 80;
      }
      #child {
        display: block;
        max-width: 40;
        margin: 0 auto;
      }
    </style>
  )";
};

TEST_CASE("Block layout max-width and margin auto centering",
          "[component][layout][margin][max-width]") {
  auto container = rtxui::Ref<MarginAutoTestComponent>::New();
  container->Mount();

  auto root_box = rtxui::LayoutTreeBuilder::Build(container->Root());
  REQUIRE(root_box != nullptr);

  rtxui::LayoutConstraints viewport = {
      {80, rtxui::MeasureMode::Exactly},
      {24, rtxui::MeasureMode::Exactly},
  };
  auto root_fragment = rtxui::RunLayout({root_box.get()}, viewport);
  REQUIRE(root_fragment != nullptr);

  REQUIRE(root_fragment->children.size() == 1);
  auto parent_frag = root_fragment->children[0].fragment;
  REQUIRE(parent_frag->children.size() == 1);
  auto child_link = parent_frag->children[0];
  CHECK(child_link.fragment->width == 40);
  CHECK(child_link.x == 20);
}

class MaxHeightTestComponent : public rtxui::Component<MaxHeightTestComponent> {
 public:
  void InitReflection() override {
    rtxui::Component<MaxHeightTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <box id="parent">
      <box id="child">
        <box>1</box>
        <box>2</box>
        <box>3</box>
        <box>4</box>
        <box>5</box>
      </box>
    </box>
    <style>
      box {
        display: block;
      }
      #parent {
        height: 20;
      }
      #child {
        max-height: 3;
      }
    </style>
  )";
};

TEST_CASE("Block layout max-height constraint",
          "[component][layout][max-height]") {
  auto container = rtxui::Ref<MaxHeightTestComponent>::New();
  container->Mount();

  auto root_box = rtxui::LayoutTreeBuilder::Build(container->Root());
  REQUIRE(root_box != nullptr);

  rtxui::LayoutConstraints viewport = {
      {80, rtxui::MeasureMode::Exactly},
      {24, rtxui::MeasureMode::Exactly},
  };
  auto root_fragment = rtxui::RunLayout({root_box.get()}, viewport);
  REQUIRE(root_fragment != nullptr);

  REQUIRE(root_fragment->children.size() == 1);
  auto parent_frag = root_fragment->children[0].fragment;
  REQUIRE(parent_frag->children.size() == 1);
  auto child_link = parent_frag->children[0];
  CHECK(child_link.fragment->height == 3);
}

class MarkdownTestContainer : public rtxui::Component<MarkdownTestContainer> {
 public:
  void InitReflection() override {
    Import<rtxui::markdown>();
    rtxui::Component<MarkdownTestContainer>::InitReflection();
  }

  std::string_view view = R"(
    <markdown id="md" content="# Title" stylesheet="h1 { color: red; }"></markdown>
  )";
};

TEST_CASE("Markdown Component rendering", "[component][markdown]") {
  auto container = rtxui::Ref<MarkdownTestContainer>::New();
  container->Mount();
  container->Digest();

  auto* root = container->Root();
  auto* md_el = root->QuerySelector("#md");
  REQUIRE(md_el != nullptr);

  auto* h1_el = root->QuerySelector("h1");
  REQUIRE(h1_el != nullptr);

  // The stylesheet inside the markdown component should apply to the h1
  // since the markdown component's GetView prepends the <style> block.
  CHECK(h1_el->style.foreground_color.has_value());
}

class MarkdownLinkInjectionTestContainer
    : public rtxui::Component<MarkdownLinkInjectionTestContainer> {
 public:
  std::string content = "[click](\" onclick=\"Evil)";
  void InitReflection() override {
    Bind(content);
    Import<rtxui::markdown>();
    rtxui::Component<MarkdownLinkInjectionTestContainer>::InitReflection();
  }

  std::string_view view = R"(
    <markdown id="md" content="{content}"></markdown>
  )";
};

TEST_CASE("Markdown link URL with a quote does not crash Mount or inject "
          "attributes",
          "[component][markdown]") {
  // Regression: a link URL containing '"' used to produce HTML that failed
  // to parse as XML (crashing the whole process on Mount, since that path
  // is not recoverable like Digest()'s), or, when it happened to still
  // parse, injected an extra "onclick" attribute into the <a> element.
  auto container = rtxui::Ref<MarkdownLinkInjectionTestContainer>::New();
  CHECK_NOTHROW(container->Mount());
  CHECK_NOTHROW(container->Digest());

  auto* link_el = container->Root()->QuerySelector("a");
  REQUIRE(link_el != nullptr);
  CHECK(link_el->GetAttribute("onclick") == nullptr);
}

class MarkdownListTestContainer
    : public rtxui::Component<MarkdownListTestContainer> {
 public:
  void InitReflection() override {
    Import<rtxui::markdown>();
    rtxui::Component<MarkdownListTestContainer>::InitReflection();
  }

  std::string_view view = R"(
    <markdown id="md" content="- list item 1\n- list item 2"></markdown>
  )";
};

TEST_CASE("Markdown List rendering", "[component][markdown][list]") {
  auto container = rtxui::Ref<MarkdownListTestContainer>::New();
  container->Mount();
  container->Digest();

  auto* root = container->Root();
  auto* md_el = root->QuerySelector("#md");
  REQUIRE(md_el != nullptr);

  auto* ul_el = root->QuerySelector("ul");
  REQUIRE(ul_el != nullptr);

  auto* li_el = root->QuerySelector("li");
  REQUIRE(li_el != nullptr);

  auto print = ul_el->Print();
  CHECK(RemoveWhitespace(print).find("•</span>listitem1") != std::string::npos);
}

class MarkdownCodeBlockTestContainer
    : public rtxui::Component<MarkdownCodeBlockTestContainer> {
 public:
  std::string content = "```\nint main() {\n  return 0;\n}\n```";
  void InitReflection() override {
    Bind(content);
    Import<rtxui::markdown>();
    rtxui::Component<MarkdownCodeBlockTestContainer>::InitReflection();
  }

  std::string_view view = R"(
    <markdown id="md" content="{content}"></markdown>
  )";
};

TEST_CASE("Markdown code block newlines preserved",
          "[component][markdown][pre]") {
  auto container = rtxui::Ref<MarkdownCodeBlockTestContainer>::New();
  container->Mount();
  container->Digest();

  auto root_box = rtxui::LayoutTreeBuilder::Build(container->Root());
  REQUIRE(root_box != nullptr);

  rtxui::LayoutConstraints viewport = {
      {80, rtxui::MeasureMode::Exactly},
      {24, rtxui::MeasureMode::Exactly},
  };
  auto root_fragment = rtxui::RunLayout({root_box.get()}, viewport);
  REQUIRE(root_fragment != nullptr);

  Texture texture(80, 24);
  rtxui::Paint(root_fragment.get(), texture);

  std::string rendered = texture.Render();
  auto pos_main = rendered.find("int main()");
  auto pos_return = rendered.find("return 0;");
  REQUIRE(pos_main != std::string::npos);
  REQUIRE(pos_return != std::string::npos);
  CHECK(rendered.find('\n', pos_main) < pos_return);
}

TEST_CASE("Default Components Registration", "[component]") {
  CHECK(rtxui::GetGlobalComponentFactory("ul") != nullptr);
  CHECK(rtxui::GetGlobalComponentFactory("ol") != nullptr);
  CHECK(rtxui::GetGlobalComponentFactory("li") != nullptr);
  CHECK(rtxui::GetGlobalComponentFactory("div") != nullptr);
  CHECK(rtxui::GetGlobalComponentFactory("span") != nullptr);
  CHECK(rtxui::GetGlobalComponentFactory("p") != nullptr);
  CHECK(rtxui::GetGlobalComponentFactory("h1") != nullptr);
  CHECK(rtxui::GetGlobalComponentFactory("h2") != nullptr);
  CHECK(rtxui::GetGlobalComponentFactory("h3") != nullptr);
  CHECK(rtxui::GetGlobalComponentFactory("h4") != nullptr);
  CHECK(rtxui::GetGlobalComponentFactory("h5") != nullptr);
  CHECK(rtxui::GetGlobalComponentFactory("h6") != nullptr);
  CHECK(rtxui::GetGlobalComponentFactory("button") != nullptr);
  CHECK(rtxui::GetGlobalComponentFactory("input") != nullptr);
  CHECK(rtxui::GetGlobalComponentFactory("textarea") != nullptr);
  CHECK(rtxui::GetGlobalComponentFactory("checkbox") != nullptr);
  CHECK(rtxui::GetGlobalComponentFactory("slider") != nullptr);
  CHECK(rtxui::GetGlobalComponentFactory("progress") != nullptr);
  CHECK(rtxui::GetGlobalComponentFactory("select") != nullptr);
  CHECK(rtxui::GetGlobalComponentFactory("option") != nullptr);
  CHECK(rtxui::GetGlobalComponentFactory("hr") != nullptr);
  CHECK(rtxui::GetGlobalComponentFactory("markdown") != nullptr);
  CHECK(rtxui::GetGlobalComponentFactory("b") != nullptr);
  CHECK(rtxui::GetGlobalComponentFactory("strong") != nullptr);
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
  CHECK(child1->GetInterpolatedValue("value") == "10");
  CHECK(child2->GetInterpolatedValue("value") == "10");

  // Simulate child1 modifying its prop (e.g. from user input)
  child1->SetProperty("props.value", "42");

  // Running Digest to propagate reactive updates
  parent->Digest();

  // The parent should be updated to 42
  CHECK(parent->parent_value == 42);

  // child2 should also receive the updated value 42
  CHECK(child2->GetInterpolatedValue("value") == "42");
}

class ListTestComponent : public rtxui::Component<ListTestComponent> {
 public:
  void InitReflection() override {
    Import<rtxui::div>();
    Import<rtxui::ul>();
    Import<rtxui::ol>();
    Import<rtxui::li>();
    rtxui::Component<ListTestComponent>::InitReflection();
  }

  std::string_view view = R"(
      <div>
        <ul id="ul_disc">
          <li>item 1</li>
          <li>item 2</li>
        </ul>
        <ul id="ul_nested">
          <li>outer 1
            <ul>
              <li>inner 1</li>
              <li>inner 2</li>
            </ul>
          </li>
        </ul>
        <ol id="ol_decimal">
          <li>first</li>
          <li>second</li>
        </ol>
        <ul id="ul_custom_square">
          <li>square item</li>
        </ul>
        <ul id="ul_custom_none">
          <li>none item</li>
        </ul>
      </div>

      <style>
        #ul_custom_square {
          list-style-type: square;
        }
        #ul_custom_none {
          list-style-type: none;
        }
      </style>
    )";
};

TEST_CASE("List Rendering - ul, ol, li, and CSS", "[component][list]") {
  auto comp = rtxui::Ref<ListTestComponent>::New();
  comp->Mount();
  comp->Digest();

  auto* root = comp->Root();
  REQUIRE(root != nullptr);

  // 1. Unordered list with default bullet (disc -> "• ")
  auto* ul_disc = root->QuerySelector("#ul_disc");
  REQUIRE(ul_disc != nullptr);
  auto print_disc = ul_disc->Print();
  CHECK(RemoveWhitespace(print_disc) ==
        "<ulid=\"ul_disc\"><li><span>•</span>item1</li><li><span>•</"
        "span>item2</li></ul>");

  // 2. Nested unordered list (outer disc -> "• ", inner circle -> "○ ")
  auto* ul_nested = root->QuerySelector("#ul_nested");
  REQUIRE(ul_nested != nullptr);
  auto print_nested = ul_nested->Print();
  CHECK(RemoveWhitespace(print_nested) ==
        "<ulid=\"ul_nested\"><li><span>•</span>outer1<ul><li><span>○</"
        "span>inner1</li><li><span>○</span>inner2</li></ul></li></ul>");

  // 3. Ordered list with decimal numbering ("1. ", "2. ")
  auto* ol_decimal = root->QuerySelector("#ol_decimal");
  REQUIRE(ol_decimal != nullptr);
  auto print_decimal = ol_decimal->Print();
  CHECK(RemoveWhitespace(print_decimal) ==
        "<olid=\"ol_decimal\"><li><span>1.</span>first</li><li><span>2.</"
        "span>second</li></ol>");

  // 4. Custom list-style-type: square ("■ ")
  auto* ul_custom_square = root->QuerySelector("#ul_custom_square");
  REQUIRE(ul_custom_square != nullptr);
  auto print_square = ul_custom_square->Print();
  CHECK(RemoveWhitespace(print_square) ==
        "<ulid=\"ul_custom_square\"><li><span>■</span>squareitem</li></ul>");

  // 5. Custom list-style-type: none ("")
  auto* ul_custom_none = root->QuerySelector("#ul_custom_none");
  REQUIRE(ul_custom_none != nullptr);
  auto print_none = ul_custom_none->Print();
  CHECK(RemoveWhitespace(print_none) ==
        "<ulid=\"ul_custom_none\"><li><span></span>noneitem</li></ul>");
}

TEST_CASE("Heading Tags Rendering h1-h6", "[component]") {
  class HeadingsTestComponent : public rtxui::Component<HeadingsTestComponent> {
   public:
    std::string_view view = R"html(
      <div>
        <h1 id="heading1">H1</h1>
        <h2 id="heading2">H2</h2>
        <h3 id="heading3">H3</h3>
        <h4 id="heading4">H4</h4>
        <h5 id="heading5">H5</h5>
        <h6 id="heading6">H6</h6>
      </div>
    )html";

    HeadingsTestComponent() {
      Import<rtxui::div>();
      Import<rtxui::h1>();
      Import<rtxui::h2>();
      Import<rtxui::h3>();
      Import<rtxui::h4>();
      Import<rtxui::h5>();
      Import<rtxui::h6>();
    }
  };

  auto app = rtxui::Ref<HeadingsTestComponent>::New();
  app->Mount();
  app->Digest();

  auto* root = app->Root();
  REQUIRE(root != nullptr);

  auto* h1_el = root->QuerySelector("#heading1");
  auto* h2_el = root->QuerySelector("#heading2");
  auto* h3_el = root->QuerySelector("#heading3");
  auto* h4_el = root->QuerySelector("#heading4");
  auto* h5_el = root->QuerySelector("#heading5");
  auto* h6_el = root->QuerySelector("#heading6");

  REQUIRE(h1_el != nullptr);
  REQUIRE(h2_el != nullptr);
  REQUIRE(h3_el != nullptr);
  REQUIRE(h4_el != nullptr);
  REQUIRE(h5_el != nullptr);
  REQUIRE(h6_el != nullptr);

  // Check that default style block display is active
  CHECK(h1_el->style.display_outside == rtxui::DisplayOutside::Block);
  CHECK(h2_el->style.display_outside == rtxui::DisplayOutside::Block);
  CHECK(h3_el->style.display_outside == rtxui::DisplayOutside::Block);
  CHECK(h4_el->style.display_outside == rtxui::DisplayOutside::Block);
  CHECK(h5_el->style.display_outside == rtxui::DisplayOutside::Block);
  CHECK(h6_el->style.display_outside == rtxui::DisplayOutside::Block);
}

}  // namespace

#define RTXUI_BENCHMARK
#include "../../../example/demo.cpp"
#include "rtxui/terminal/terminal_device.hpp"

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

TEST_CASE("CSS Tag and Class Selector Combination (e.g., button.active)",
          "[component][css]") {
  auto app = Ref<ConditionalTabTestApp>::New();
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  device->TriggerResize(80, 24);
  rtxui::Screen screen(app, device);

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

TEST_CASE("Vertical slider layout regression", "[component][slider][layout]") {
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  device->TriggerResize(10, 10);

  class VerticalSliderTest : public rtxui::Component<VerticalSliderTest> {
   public:
    void InitReflection() override {
      Import<rtxui::slider>();
      rtxui::Component<VerticalSliderTest>::InitReflection();
    }
    std::string_view view = R"(
      <slider direction="vertical" width="5" value="50" min="0" max="100" />
    )";
  };

  auto component = rtxui::Ref<VerticalSliderTest>::New();
  rtxui::Screen screen(component, device);
  screen.Draw();

  std::string output = device->GetOutput();

  int rows_with_slider = 0;
  std::stringstream ss(output);
  std::string line;
  while (std::getline(ss, line)) {
    if (line.find("│") != std::string::npos ||
        line.find("●") != std::string::npos) {
      rows_with_slider++;
    }
  }

  if (rows_with_slider < 5) {
    FAIL("Vertical slider should span 5 rows. Output:\n" << output);
  }
  CHECK(rows_with_slider >= 5);
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

TEST_CASE("Slider click on real demo layout squash regression", "[demo][regression]") {
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

TEST_CASE("Input Click Layout Regression Test",
          "[component][input][regression]") {
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  auto container = rtxui::Ref<InputTestComponent>::New();
  rtxui::Screen screen(container, device);
  screen.Draw();

  auto* input_el = container->Root()->QuerySelector("input");
  REQUIRE(input_el != nullptr);

  int initial_width = input_el->layout_width();
  int initial_height = input_el->layout_height();

  // Click at the input's position
  Event::Mouse mouse;
  mouse.button = Event::Mouse::Button::Left;
  mouse.motion = Event::Mouse::Motion::Pressed;
  mouse.x = input_el->absolute_x() + 1;
  mouse.y = input_el->absolute_y() + 1;
  screen.Dispatch(Event(mouse));
  screen.Draw();

  CHECK(input_el->layout_width() == initial_width);
  CHECK(input_el->layout_height() == initial_height);
}

TEST_CASE("Textarea Click Layout Regression Test",
          "[component][textarea][regression]") {
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  auto container = rtxui::Ref<TextareaTestComponent>::New();
  rtxui::Screen screen(container, device);
  screen.Draw();

  auto* ta_el = container->Root()->QuerySelector("textarea");
  REQUIRE(ta_el != nullptr);

  int initial_width = ta_el->layout_width();
  int initial_height = ta_el->layout_height();

  // Click at the textarea's position
  Event::Mouse mouse;
  mouse.button = Event::Mouse::Button::Left;
  mouse.motion = Event::Mouse::Motion::Pressed;
  mouse.x = ta_el->absolute_x() + 1;
  mouse.y = ta_el->absolute_y() + 1;
  screen.Dispatch(Event(mouse));
  screen.Draw();

  CHECK(ta_el->layout_width() == initial_width);
  CHECK(ta_el->layout_height() == initial_height);
}

TEST_CASE("Input Cursor Vertical Line Regression Test",
          "[component][input][cursor][regression]") {
  auto container = rtxui::Ref<InputTestComponent>::New();
  container->Mount();

  auto* input_el = container->Root()->QuerySelector("input");
  REQUIRE(input_el != nullptr);

  auto* input_comp = const_cast<rtxui::ComponentBase*>(input_el->component());
  REQUIRE(input_comp != nullptr);

  auto* input_ptr = dynamic_cast<rtxui::input*>(input_comp);
  REQUIRE(input_ptr != nullptr);

  // Set cursor position to the end of the text
  input_ptr->value = "hello";
  input_ptr->cursor_pos = 5;

  // Unfocused initially
  input_el->set_focused(false);
  input_ptr->Digest();
  CHECK(input_ptr->cursor_char == " ");
  CHECK(input_ptr->cursor_class == "cursor");

  // Focus the element
  input_el->set_focused(true);
  input_ptr->Digest();
  CHECK(input_ptr->cursor_char == " ");
  CHECK(input_ptr->cursor_class == "cursor cursor-focused");

  // Move cursor to a character inside the text
  input_ptr->cursor_pos = 1;
  input_ptr->Digest();
  CHECK(input_ptr->cursor_char == "e");
  CHECK(input_ptr->cursor_class == "cursor cursor-focused");
}

class SmallInput : public rtxui::Component<SmallInput>,
                   public rtxui::TextInputBase {
 public:
  void InitReflection() override {
    rtxui::Component<SmallInput>::InitReflection();
  }
  std::string_view Setup() override {
    return R"html(
      <span>{left_text}</span><span class="{cursor_class}">{cursor_char}</span><span>{right_text}</span>
      <style>
        self {
          display: inline-flex;
          flex-direction: row;
          width: 5;
          padding-left: 1;
          padding-right: 1;
          overflow-x: scroll;
          scrollbar-width: none;
          white-space: nowrap;
          background-color: #1e293b;
          border: solid;
          border-color: #334155;
        }
        .cursor {
          background-color: transparent;
        }
        .cursor-focused {
          background-color: transparent;
        }
      </style>
    )html";
  }
  bool OnEvent(Event event) override {
    return OnEventShared(this, event, false);
  }
  bool Digest() override {
    DigestShared(this);
    return Component<SmallInput>::Digest();
  }
};

class SmallInputTestComponent
    : public rtxui::Component<SmallInputTestComponent> {
 public:
  std::string my_text = "";
  void InitReflection() override {
    Bind(my_text);
    Import<SmallInput>();
    rtxui::Component<SmallInputTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <SmallInput value="{my_text}" />
  )";
};

class SmallTextarea : public rtxui::Component<SmallTextarea>,
                      public rtxui::TextInputBase {
 public:
  void InitReflection() override {
    rtxui::Component<SmallTextarea>::InitReflection();
  }
  std::string_view Setup() override {
    return R"html(
      <span>{left_text}</span><span class="{cursor_class}">{cursor_char}</span><span>{right_text}</span>
      <style>
        self {
          display: block;
          width: 5;
          height: 3;
          padding-left: 1;
          padding-right: 1;
          overflow-y: scroll;
          scrollbar-width: none;
          white-space: pre-wrap;
          background-color: #1e293b;
          border: solid;
          border-color: #334155;
        }
        .cursor {
          background-color: transparent;
        }
        .cursor-focused {
          background-color: transparent;
        }
      </style>
    )html";
  }
  bool OnEvent(Event event) override {
    return OnEventShared(this, event, true);
  }
  bool Digest() override {
    DigestShared(this);
    return Component<SmallTextarea>::Digest();
  }
};

class SmallTextareaTestComponent
    : public rtxui::Component<SmallTextareaTestComponent> {
 public:
  std::string my_text = "";
  void InitReflection() override {
    Bind(my_text);
    Import<SmallTextarea>();
    rtxui::Component<SmallTextareaTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <SmallTextarea value="{my_text}" />
  )";
};

TEST_CASE("Input Scrolling Regression Test",
          "[component][input][scroll][regression]") {
  auto container = rtxui::Ref<SmallInputTestComponent>::New();
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  rtxui::Screen screen(container, device);

  auto* input_el = container->Root()->QuerySelector("SmallInput");
  REQUIRE(input_el != nullptr);

  auto* input_comp = const_cast<rtxui::ComponentBase*>(input_el->component());
  REQUIRE(input_comp != nullptr);

  auto* input_ptr = dynamic_cast<SmallInput*>(input_comp);
  REQUIRE(input_ptr != nullptr);

  screen.Draw();
  CHECK(input_el->layout_width() == 5);

  // Clear value to start empty
  input_ptr->value = "";
  input_ptr->cursor_pos = 0;

  // Focus to accept keyboard events
  input_el->set_focused(true);

  // Type characters to overflow
  screen.Dispatch(Event::Keyboard::From('a'));
  screen.Dispatch(Event::Keyboard::From('b'));
  screen.Dispatch(Event::Keyboard::From('c'));
  screen.Dispatch(Event::Keyboard::From('d'));
  screen.Draw();

  // Scroll offset should have increased to keep the cursor visible
  CHECK(input_el->scroll_x() > 0);

  // Press ArrowLeft multiple times to move cursor back to beginning
  screen.Dispatch(Event::ArrowLeft());
  screen.Dispatch(Event::ArrowLeft());
  screen.Dispatch(Event::ArrowLeft());
  screen.Dispatch(Event::ArrowLeft());
  screen.Draw();

  // Scroll offset should have reverted to 0
  CHECK(input_el->scroll_x() == 0);
}

TEST_CASE("Textarea Scrolling Regression Test",
          "[component][textarea][scroll][regression]") {
  auto container = rtxui::Ref<SmallTextareaTestComponent>::New();
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  rtxui::Screen screen(container, device);

  auto* ta_el = container->Root()->QuerySelector("SmallTextarea");
  REQUIRE(ta_el != nullptr);

  auto* ta_comp = const_cast<rtxui::ComponentBase*>(ta_el->component());
  REQUIRE(ta_comp != nullptr);

  auto* ta_ptr = dynamic_cast<SmallTextarea*>(ta_comp);
  REQUIRE(ta_ptr != nullptr);

  screen.Draw();
  CHECK(ta_el->layout_height() == 3);

  // Clear value to start empty
  ta_ptr->value = "";
  ta_ptr->cursor_pos = 0;

  // Focus to accept keyboard events
  ta_el->set_focused(true);

  // Press Return multiple times to create new lines and overflow vertically
  screen.Dispatch(Event::Return());
  screen.Dispatch(Event::Return());
  screen.Dispatch(Event::Return());
  screen.Draw();

  // Scroll offset should have increased vertically
  CHECK(ta_el->scroll_y() > 0);

  // Press ArrowUp multiple times to move cursor back to beginning
  screen.Dispatch(Event::ArrowUp());
  screen.Dispatch(Event::ArrowUp());
  screen.Dispatch(Event::ArrowUp());
  screen.Draw();

  CHECK(ta_el->scroll_y() == 0);
}

TEST_CASE("Mouse capture is released when component is destroyed",
          "[component]") {
  ComponentBase* raw_comp_ptr = nullptr;
  {
    auto comp = rtxui::Ref<rtxui::slider>::New();
    comp->CaptureMouse();
    raw_comp_ptr = comp.get();
    CHECK(rtxui::ComponentBase::GetMouseCapturer() == raw_comp_ptr);
  }
  CHECK(rtxui::ComponentBase::GetMouseCapturer() == nullptr);
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

class InputReuseTestComponent
    : public rtxui::Component<InputReuseTestComponent> {
 public:
  void InitReflection() override {
    Import<rtxui::input>();
    rtxui::Component<InputReuseTestComponent>::InitReflection();
  }
  std::string value = "initial";
  std::string_view view = R"(
    <div>
      <input id="test-input" value="{value}" />
    </div>
  )";
  InputReuseTestComponent() { Bind(value); }
};

TEST_CASE("Input component is not recreated when attributes change",
          "[component][input]") {
  auto container = rtxui::Ref<InputReuseTestComponent>::New();
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  device->TriggerResize(80, 24);
  rtxui::Screen screen(container, device);

  auto* input_element = container->Root()->QuerySelector("#test-input");
  REQUIRE(input_element != nullptr);
  auto* input_component = input_element->component();
  REQUIRE(input_component != nullptr);

  // Now change value
  container->value = "updated";
  container->Render();

  auto* new_input_element = container->Root()->QuerySelector("#test-input");
  REQUIRE(new_input_element != nullptr);
  CHECK(new_input_element->component() == input_component);
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

TEST_CASE("Flickering transition is not triggered on input navigation",
          "[component][input][transition]") {
  auto container = rtxui::Ref<InputReuseTestComponent>::New();
  container->Mount();

  auto* input_el = container->Root()->QuerySelector("#test-input");
  REQUIRE(input_el != nullptr);

  // Focus the input
  input_el->set_focused(true);

  // Initial render
  container->Render();

  double now = rtxui::time::GetTimeMs();

  // Complete any focus transition
  input_el->TickTransitions(now + 200.0);
  CHECK(input_el->style.opacity == 1.0f);
  CHECK(input_el->active_transitions.empty());

  // Simulate navigating with ArrowLeft
  auto* input_comp = const_cast<rtxui::ComponentBase*>(input_el->component());
  REQUIRE(input_comp != nullptr);
  auto* input_ptr = dynamic_cast<rtxui::input*>(input_comp);
  REQUIRE(input_ptr != nullptr);
  input_ptr->OnEvent(Event::ArrowLeft());
  input_ptr->Digest();

  // Check that no transition was triggered and it stayed at opacity 1.0
  CHECK(input_el->active_transitions.empty());
  CHECK(input_el->style.opacity == 1.0f);
}

class StyledParentTestComponent
    : public rtxui::Component<StyledParentTestComponent> {
 public:
  void InitReflection() override {
    Import<rtxui::input>();
    rtxui::Component<StyledParentTestComponent>::InitReflection();
  }
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
  auto* input_comp = const_cast<rtxui::ComponentBase*>(input_el->component());
  REQUIRE(input_comp != nullptr);
  auto* input_ptr = dynamic_cast<rtxui::input*>(input_comp);
  REQUIRE(input_ptr != nullptr);
  input_ptr->OnEvent(Event::ArrowLeft());
  input_ptr->Digest();

  // The parent style should still be preserved on the child's root base style!
  CHECK(input_el->base_style.background_color.value() ==
        Color::RGB(100, 100, 100));
}

TEST_CASE("Input hover and focus do not compound when both apply at once",
          "[component][input][style]") {
  // self:hover and self:focus both commonly match simultaneously in
  // practice: clicking an input focuses it while the mouse typically
  // stays over it, so it's hovered too. Regression: both rules used to
  // set background-color via lighten(), which resolves relative to
  // whatever's already in the style being built -- so when both rules
  // applied in the same pass, self:focus lightened the already-lightened
  // self:hover result, producing a washed-out, hard-to-read color instead
  // of the intended focus look. Fixed colors don't have this problem:
  // whichever rule is declared later always wins outright.
  auto container = rtxui::Ref<StyledParentTestComponent>::New();
  container->Mount();
  auto* input_el = container->Root()->QuerySelector("#test-input");
  REQUIRE(input_el != nullptr);

  input_el->set_focused(true);
  container->ResolveTargetStyles();
  Color focus_only = input_el->target_style.background_color.value();

  input_el->set_hovered(true);  // still focused too, as after a real click
  container->ResolveTargetStyles();
  Color hover_and_focus = input_el->target_style.background_color.value();

  CHECK(hover_and_focus == focus_only);
}

class SelectionHighlightTestComponent
    : public rtxui::Component<SelectionHighlightTestComponent> {
 public:
  void InitReflection() override {
    Import<rtxui::input>();
    rtxui::Component<SelectionHighlightTestComponent>::InitReflection();
  }
  std::string_view view = R"html(
    <input id="test-input" class="custom-input" />
    <style>
      .custom-input {
        background-color: rgb(230, 230, 200);
      }
    </style>
  )html";
};

TEST_CASE("Input selection highlight is readable regardless of the "
          "app-chosen background color",
          "[component][input][style]") {
  // .selection is a separate <span>, not self: background-color doesn't
  // inherit, so it can't derive from whatever color the app gives self.
  // It uses a fixed, readable color pair instead of trying to (and it
  // can't currently be made to derive from an app override; see the
  // git history for why lighten()/darken() and CSS custom properties
  // don't work here).
  auto container = rtxui::Ref<SelectionHighlightTestComponent>::New();
  container->Mount();
  container->Digest();

  auto* input_el = container->Root()->QuerySelector("#test-input");
  REQUIRE(input_el != nullptr);
  auto* input_ptr = dynamic_cast<rtxui::input*>(
      const_cast<rtxui::ComponentBase*>(input_el->component()));
  REQUIRE(input_ptr != nullptr);

  input_el->set_focused(true);
  input_ptr->value = "hello world";
  input_ptr->cursor_pos = 0;
  input_ptr->selection_start = -1;
  input_ptr->Digest();

  Event::Keyboard kb;
  kb.special = Event::Keyboard::Special::ArrowRight;
  kb.modifier.shift = true;
  input_ptr->OnEvent(Event(kb));
  input_ptr->Digest();

  auto* selection_el = container->Root()->QuerySelector(".selection");
  REQUIRE(selection_el != nullptr);
  CHECK(selection_el->base_style.background_color.value() ==
        Color::RGB(38, 79, 120));
  CHECK(selection_el->base_style.foreground_color.value() ==
        Color::RGB(255, 255, 255));
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

TEST_CASE("Component.InlineStyleParsingRegression", "[component][style]") {
  class InlineStyleComp : public rtxui::Component<InlineStyleComp> {
   public:
    void InitReflection() override {
      Import<rtxui::div>();
      rtxui::Component<InlineStyleComp>::InitReflection();
    }
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
    void InitReflection() override {
      Bind(rule);
      Import<rtxui::div>();
      rtxui::Component<DynamicStyleComp>::InitReflection();
    }
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

#include "rtxui/component/default/code/code.hpp"
#include "rtxui/component/default/s/s.hpp"
#include "rtxui/component/default/u/u.hpp"

struct FormattingTestComponent
    : public rtxui::Component<FormattingTestComponent> {
  void InitReflection() override {
    Import<rtxui::u>();
    Import<rtxui::s>();
    Import<rtxui::code>();
    rtxui::Component<FormattingTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <div>
      <u>Underlined</u>
      <s>Strikethrough</s>
      <strike>Strike</strike>
      <del>Del</del>
      <code>inline code</code>
    </div>
  )";
};

TEST_CASE("Underline, Strikethrough, and Code Components",
          "[component][u][s][code]") {
  auto container = rtxui::Ref<FormattingTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* u_el = container->Root()->QuerySelector("u");
  REQUIRE(u_el != nullptr);
  auto* u_comp = const_cast<rtxui::ComponentBase*>(u_el->component());
  REQUIRE(u_comp != nullptr);
  auto* u_ptr = dynamic_cast<rtxui::u*>(u_comp);
  REQUIRE(u_ptr != nullptr);

  auto* s_el = container->Root()->QuerySelector("s");
  REQUIRE(s_el != nullptr);
  auto* s_comp = const_cast<rtxui::ComponentBase*>(s_el->component());
  REQUIRE(s_comp != nullptr);
  auto* s_ptr = dynamic_cast<rtxui::s*>(s_comp);
  REQUIRE(s_ptr != nullptr);

  auto* strike_el = container->Root()->QuerySelector("strike");
  REQUIRE(strike_el != nullptr);
  auto* strike_comp = const_cast<rtxui::ComponentBase*>(strike_el->component());
  REQUIRE(strike_comp != nullptr);
  auto* strike_ptr = dynamic_cast<rtxui::strike*>(strike_comp);
  REQUIRE(strike_ptr != nullptr);

  auto* del_el = container->Root()->QuerySelector("del");
  REQUIRE(del_el != nullptr);
  auto* del_comp = const_cast<rtxui::ComponentBase*>(del_el->component());
  REQUIRE(del_comp != nullptr);
  auto* del_ptr = dynamic_cast<rtxui::del*>(del_comp);
  REQUIRE(del_ptr != nullptr);

  auto* code_el = container->Root()->QuerySelector("code");
  REQUIRE(code_el != nullptr);
  auto* code_comp = const_cast<rtxui::ComponentBase*>(code_el->component());
  REQUIRE(code_comp != nullptr);
  auto* code_ptr = dynamic_cast<rtxui::code*>(code_comp);
  REQUIRE(code_ptr != nullptr);
}

#include "rtxui/component/default/pre/pre.hpp"

struct PreTestComponent : public rtxui::Component<PreTestComponent> {
  void InitReflection() override {
    Import<rtxui::pre>();
    rtxui::Component<PreTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <div>
      <pre>Line 1
  Line 2 with spaces
Line 3</pre>
    </div>
  )";
};

TEST_CASE("Pre component whitespace preservation", "[component][pre]") {
  auto container = rtxui::Ref<PreTestComponent>::New();
  container->Mount();

  auto root_box = rtxui::LayoutTreeBuilder::Build(container->Root());
  REQUIRE(root_box != nullptr);

  rtxui::LayoutConstraints viewport = {
      {80, rtxui::MeasureMode::Exactly},
      {24, rtxui::MeasureMode::Exactly},
  };
  auto root_fragment = rtxui::RunLayout({root_box.get()}, viewport);
  REQUIRE(root_fragment != nullptr);

  Texture texture(80, 24);
  rtxui::Paint(root_fragment.get(), texture);

  std::string rendered = texture.Render();
  CHECK(rendered.find("Line 1") != std::string::npos);
  CHECK(rendered.find("  Line 2 with spaces") != std::string::npos);
  CHECK(rendered.find("Line 3") != std::string::npos);
}

struct PreCodeTestComponent : public rtxui::Component<PreCodeTestComponent> {
  void InitReflection() override {
    Import<rtxui::pre>();
    Import<rtxui::code>();
    rtxui::Component<PreCodeTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <div>
      <pre><code>int main() {
  return 0;
}</code></pre>
    </div>
  )";
};

TEST_CASE("Pre code newlines preserved", "[component][pre]") {
  auto container = rtxui::Ref<PreCodeTestComponent>::New();
  container->Mount();

  auto root_box = rtxui::LayoutTreeBuilder::Build(container->Root());
  REQUIRE(root_box != nullptr);

  rtxui::LayoutConstraints viewport = {
      {80, rtxui::MeasureMode::Exactly},
      {24, rtxui::MeasureMode::Exactly},
  };
  auto root_fragment = rtxui::RunLayout({root_box.get()}, viewport);
  REQUIRE(root_fragment != nullptr);

  Texture texture(80, 24);
  rtxui::Paint(root_fragment.get(), texture);

  // Each source line must land on its own row.
  std::string rendered = texture.Render();
  CHECK(rendered.find("int main() {") != std::string::npos);
  CHECK(rendered.find("  return 0;") != std::string::npos);
  CHECK(rendered.find("int main() {   return 0; }") == std::string::npos);
  auto pos_main = rendered.find("int main()");
  auto pos_return = rendered.find("return 0;");
  REQUIRE(pos_main != std::string::npos);
  REQUIRE(pos_return != std::string::npos);
  CHECK(rendered.find('\n', pos_main) < pos_return);
}

struct PreCodeBindingTestComponent
    : public rtxui::Component<PreCodeBindingTestComponent> {
  std::string source = "int main() {\n  return 0;\n}";
  void InitReflection() override {
    Bind(source);
    Import<rtxui::pre>();
    Import<rtxui::code>();
    rtxui::Component<PreCodeBindingTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <div>
      <pre><code>{source}</code></pre>
    </div>
  )";
};

TEST_CASE("Pre code newlines preserved through interpolation",
          "[component][pre]") {
  auto container = rtxui::Ref<PreCodeBindingTestComponent>::New();
  container->Mount();

  SECTION("initial value") {}
  SECTION("value updated after mount") {
    container->source = "// updated\nint main() {\n  return 0;\n}";
    container->Digest();
  }

  auto root_box = rtxui::LayoutTreeBuilder::Build(container->Root());
  REQUIRE(root_box != nullptr);

  rtxui::LayoutConstraints viewport = {
      {80, rtxui::MeasureMode::Exactly},
      {24, rtxui::MeasureMode::Exactly},
  };
  auto root_fragment = rtxui::RunLayout({root_box.get()}, viewport);
  REQUIRE(root_fragment != nullptr);

  Texture texture(80, 24);
  rtxui::Paint(root_fragment.get(), texture);

  std::string rendered = texture.Render();
  auto pos_main = rendered.find("int main()");
  auto pos_return = rendered.find("return 0;");
  REQUIRE(pos_main != std::string::npos);
  REQUIRE(pos_return != std::string::npos);
  CHECK(rendered.find('\n', pos_main) < pos_return);
}

struct InlineSpanBackgroundTestComponent
    : public rtxui::Component<InlineSpanBackgroundTestComponent> {
  void InitReflection() override {
    Import<rtxui::div>();
    Import<rtxui::span>();
    rtxui::Component<InlineSpanBackgroundTestComponent>::InitReflection();
  }
  std::string_view view = R"html(
    <div>before <span class="hl">highlighted</span> after</div>
    <style>
      .hl { background-color: rgb(38, 79, 120); }
    </style>
  )html";
};

TEST_CASE("Inline span background-color is applied when painted",
          "[component][layout][regression]") {
  // Regression: LayoutInlineFlow's optimization for inline elements with no
  // border/padding/margin (e.g. a plain <span>) flattens them and processes
  // their text node directly, but read the *text node's* style for
  // background-color instead of the *span's*. background-color doesn't
  // inherit, so the text node never actually carried it -- the span's own
  // background-color was silently dropped at paint time (while still
  // resolving correctly in base_style, since that part of the pipeline was
  // unaffected). This is what made textarea/input's .selection highlight
  // invisible.
  auto container = rtxui::Ref<InlineSpanBackgroundTestComponent>::New();
  container->Mount();

  auto root_box = rtxui::LayoutTreeBuilder::Build(container->Root());
  REQUIRE(root_box != nullptr);
  rtxui::LayoutConstraints viewport = {
      {40, rtxui::MeasureMode::Exactly},
      {3, rtxui::MeasureMode::Exactly},
  };
  auto root_fragment = rtxui::RunLayout({root_box.get()}, viewport);
  REQUIRE(root_fragment != nullptr);

  Texture texture(40, 3);
  rtxui::Paint(root_fragment.get(), texture);

  std::string rendered = texture.Render();
  auto pos = rendered.find("highlighted");
  REQUIRE(pos != std::string::npos);

  // Find which (x, y) cell that character lands on by re-deriving it from
  // the element's own layout position, since `rendered` embeds ANSI codes.
  auto* span = container->Root()->QuerySelector(".hl");
  REQUIRE(span != nullptr);
  int x = span->absolute_x();
  int y = span->absolute_y();
  CHECK(texture[x, y].background_color == Color::RGB(38, 79, 120));
}

class SliderDemoSquashTestComponent : public rtxui::Component<SliderDemoSquashTestComponent> {
 public:
  int slider_val = 50;

  void InitReflection() override {
    ComponentBase::InitReflection();
    Bind(slider_val);
    Import<rtxui::slider>();
    Import<rtxui::div>();
    Import<rtxui::span>();
  }

  std::string_view view = R"html(
    <div class="container">
      <div class="root">
        <div class="workspace">
          <div class="sidebar">Sidebar</div>
          <div class="main-scroll">
            <div class="card">
              <div class="row">
                <span>Slider:</span>
                <slider id="slider" value="{slider_val}" min="0" max="100" step="5" width="18" />
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
    <style>
      self {
        display: block;
        width: 100%;
        height: 100%;
      }
      .container {
        display: block;
        max-width: 120;
        width: 100%;
        margin-left: auto;
        margin-right: auto;
      }
      .root {
        display: flex;
        flex-direction: column;
        width: 100%;
      }
      .workspace {
        display: flex;
        flex-direction: row;
        width: 100%;
      }
      .sidebar {
        display: block;
        width: 16;
      }
      .main-scroll {
        flex-grow: 1;
        width: 0;
        display: block;
      }
      .card {
        display: block;
      }
      .row {
        display: flex;
        flex-direction: row;
      }
    </style>
  )html";
};

TEST_CASE("Slider interaction layout squashing regression", "[component][slider][layout][regression]") {
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  device->TriggerResize(150, 20);

  auto container = rtxui::Ref<SliderDemoSquashTestComponent>::New();
  rtxui::Screen screen(container, device);
  screen.Draw();

  auto* slider_el = container->Root()->QuerySelector("#slider");
  REQUIRE(slider_el != nullptr);
  auto* scroll_el = container->Root()->QuerySelector(".main-scroll");
  REQUIRE(scroll_el != nullptr);
  auto* container_el = container->Root()->QuerySelector(".container");
  REQUIRE(container_el != nullptr);

  int initial_container_width = container_el->layout_width();
  int initial_scroll_width = scroll_el->layout_width();

  // Click/Drag the slider
  Event::Mouse mouse;
  mouse.button = Event::Mouse::Button::Left;
  mouse.motion = Event::Mouse::Motion::Pressed;
  mouse.x = slider_el->absolute_x() + 5;
  mouse.y = slider_el->absolute_y() + 1;
  screen.Dispatch(Event(mouse));

  // Let's release the mouse too
  mouse.motion = Event::Mouse::Motion::Released;
  screen.Dispatch(Event(mouse));

  // Draw again to render final frame
  screen.Draw();

  int final_container_width = container_el->layout_width();
  int final_scroll_width = scroll_el->layout_width();

  CHECK(final_container_width == initial_container_width);
  CHECK(final_scroll_width == initial_scroll_width);
}

class DetailsTestComponent : public rtxui::Component<DetailsTestComponent> {
 public:
  bool open = false;

  void InitReflection() override {
    Bind(open);
    Import<rtxui::details>();
    Import<rtxui::summary>();
    Import<rtxui::p>();
    rtxui::Component<DetailsTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <details open="{open}">
      <summary>My Title</summary>
      <p>Secret content</p>
    </details>
  )";
};

class DetailsDefaultTestComponent : public rtxui::Component<DetailsDefaultTestComponent> {
 public:
  void InitReflection() override {
    Import<rtxui::details>();
    Import<rtxui::p>();
    rtxui::Component<DetailsDefaultTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <details>
      <p>Secret content</p>
    </details>
  )";
};

TEST_CASE("Details and Summary Components", "[component][details]") {
  auto container = rtxui::Ref<DetailsTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* details_el = container->Root()->QuerySelector("details");
  REQUIRE(details_el != nullptr);
  auto* details_comp = const_cast<rtxui::ComponentBase*>(details_el->component());
  REQUIRE(details_comp != nullptr);
  auto* details_ptr = dynamic_cast<rtxui::details*>(details_comp);
  REQUIRE(details_ptr != nullptr);

  // 1. Initial State (open is false)
  CHECK(details_ptr->open == false);
  CHECK(details_ptr->arrow_char == "▶");
  CHECK(details_ptr->content_class == "closed");

  // 2. Toggle via program API
  details_ptr->Toggle();
  container->Digest();
  screen.Draw();

  CHECK(details_ptr->open == true);
  CHECK(details_ptr->arrow_char == "▼");
  CHECK(details_ptr->content_class == "open");
  CHECK(container->open == true);

  // 3. Toggle back via program API
  details_ptr->Toggle();
  container->Digest();
  screen.Draw();

  CHECK(details_ptr->open == false);
  CHECK(container->open == false);

  // 4. Toggle via Keyboard Event on focused summary-line
  auto* summary_line_el = details_ptr->Root()->QuerySelector(".summary-line");
  REQUIRE(summary_line_el != nullptr);

  // Reset focus
  details_ptr->Root()->Visit([](rtxui::Element& el) { el.set_focused(false); });
  summary_line_el->set_focused(true);

  // Dispatch return key event
  Event return_event = Event::Keyboard({
      Event::Keyboard::Motion::Pressed,
      Event::Keyboard::Special::Return,
  });
  CHECK(details_ptr->OnEvent(return_event) == true);
  container->Digest();
  screen.Draw();

  CHECK(details_ptr->open == true);
  CHECK(container->open == true);

  // Dispatch space key event
  Event space_event = Event::Keyboard::From(' ');
  CHECK(details_ptr->OnEvent(space_event) == true);
  container->Digest();
  screen.Draw();

  CHECK(details_ptr->open == false);
  CHECK(container->open == false);

  // 5. Toggle via Mouse Click on summary-line
  Event::Mouse mouse_click;
  mouse_click.button = Event::Mouse::Button::Left;
  mouse_click.motion = Event::Mouse::Motion::Pressed;
  mouse_click.x = summary_line_el->absolute_x() + 1;
  mouse_click.y = summary_line_el->absolute_y() + 1;
  Event click_event(mouse_click);

  screen.Dispatch(click_event);
  container->Digest();
  screen.Draw();

  CHECK(details_ptr->open == true);
  CHECK(container->open == true);
}

TEST_CASE("Details Component Default Summary", "[component][details]") {
  auto container = rtxui::Ref<DetailsDefaultTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* details_el = container->Root()->QuerySelector("details");
  REQUIRE(details_el != nullptr);
  auto* details_comp = const_cast<rtxui::ComponentBase*>(details_el->component());
  REQUIRE(details_comp != nullptr);
  auto* details_ptr = dynamic_cast<rtxui::details*>(details_comp);
  REQUIRE(details_ptr != nullptr);

  // The summary slot should dynamically contain a TextElement with text "Details"
  auto summary_slot = details_ptr->Slot("summary");
  REQUIRE(summary_slot != nullptr);
  REQUIRE(summary_slot->ChildCount() > 0);

  auto* text_el = dynamic_cast<rtxui::TextElement*>(summary_slot->ChildAt(0));
  REQUIRE(text_el != nullptr);
  CHECK(text_el->text() == "Details");
}

// --- Fieldset & Legend ---
class FieldsetTestComponent : public rtxui::Component<FieldsetTestComponent> {
 public:
  void InitReflection() override {
    Import<rtxui::fieldset>();
    Import<rtxui::legend>();
    Import<rtxui::p>();
    rtxui::Component<FieldsetTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <fieldset>
      <legend>User Profile</legend>
      <p>Fieldset content</p>
    </fieldset>
  )";
};

TEST_CASE("Fieldset and Legend Components", "[component][fieldset]") {
  auto container = rtxui::Ref<FieldsetTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* fieldset_el = container->Root()->QuerySelector("fieldset");
  REQUIRE(fieldset_el != nullptr);
  auto* fieldset_comp = const_cast<rtxui::ComponentBase*>(fieldset_el->component());
  REQUIRE(fieldset_comp != nullptr);
  auto* fieldset_ptr = dynamic_cast<rtxui::fieldset*>(fieldset_comp);
  REQUIRE(fieldset_ptr != nullptr);

  // The legend slot should contain a legend component
  auto legend_slot = fieldset_ptr->Slot("legend");
  REQUIRE(legend_slot != nullptr);
  REQUIRE(legend_slot->ChildCount() > 0);

  auto* legend_el = legend_slot->ChildAt(0);
  CHECK(legend_el->tag() == "legend");
  CHECK(fieldset_ptr->legend_class == "has-legend");
}

// --- Radio ---
class RadioTestComponent : public rtxui::Component<RadioTestComponent> {
 public:
  bool opt_a = false;
  bool opt_b = false;

  void InitReflection() override {
    Bind(opt_a);
    Bind(opt_b);
    Import<rtxui::radio>();
    rtxui::Component<RadioTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <div>
      <radio id="radioA" name="my_group" checked="{opt_a}">Option A</radio>
      <radio id="radioB" name="my_group" checked="{opt_b}">Option B</radio>
    </div>
  )";
};

TEST_CASE("Radio Component Grouping", "[component][radio]") {
  auto container = rtxui::Ref<RadioTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* radioA_el = container->Root()->QuerySelector("#radioA");
  auto* radioB_el = container->Root()->QuerySelector("#radioB");
  REQUIRE(radioA_el != nullptr);
  REQUIRE(radioB_el != nullptr);

  auto* radioA_ptr = dynamic_cast<rtxui::radio*>(const_cast<rtxui::ComponentBase*>(radioA_el->component()));
  auto* radioB_ptr = dynamic_cast<rtxui::radio*>(const_cast<rtxui::ComponentBase*>(radioB_el->component()));
  REQUIRE(radioA_ptr != nullptr);
  REQUIRE(radioB_ptr != nullptr);

  // Initial State: neither is checked
  CHECK(radioA_ptr->checked == false);
  CHECK(radioB_ptr->checked == false);

  // Click on Radio A
  Event clickA = Event::Keyboard::From(' ');
  radioA_el->set_focused(true);
  CHECK(radioA_ptr->OnEvent(clickA) == true);
  container->Digest();
  screen.Draw();

  CHECK(radioA_ptr->checked == true);
  CHECK(radioB_ptr->checked == false);
  CHECK(container->opt_a == true);
  CHECK(container->opt_b == false);

  // Click on Radio B: A should automatically uncheck!
  radioA_el->set_focused(false);
  radioB_el->set_focused(true);
  Event clickB = Event::Keyboard::From(' ');
  CHECK(radioB_ptr->OnEvent(clickB) == true);
  container->Digest();
  screen.Draw();

  CHECK(radioA_ptr->checked == false);
  CHECK(radioB_ptr->checked == true);
  CHECK(container->opt_a == false);
  CHECK(container->opt_b == true);
}

// --- Tabs ---
class TabsTestComponent : public rtxui::Component<TabsTestComponent> {
 public:
  std::string active_tab = "tab1";

  void InitReflection() override {
    Bind(active_tab);
    Import<rtxui::tabs>();
    Import<rtxui::tab_pane>();
    rtxui::Component<TabsTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <tabs value="{active_tab}">
      <tab-pane label="Tab One" name="tab1">Content One</tab-pane>
      <tab-pane label="Tab Two" name="tab2">Content Two</tab-pane>
    </tabs>
  )";
};

TEST_CASE("Tabs and TabPane Components", "[component][tabs]") {
  auto container = rtxui::Ref<TabsTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* tabs_el = container->Root()->QuerySelector("tabs");
  REQUIRE(tabs_el != nullptr);
  auto* tabs_ptr = dynamic_cast<rtxui::tabs*>(const_cast<rtxui::ComponentBase*>(tabs_el->component()));
  REQUIRE(tabs_ptr != nullptr);

  // Initial tab: tab1
  CHECK(tabs_ptr->value == "tab1");

  // Verify headers slot
  auto headers_slot = tabs_ptr->Slot("headers");
  REQUIRE(headers_slot != nullptr);
  REQUIRE(headers_slot->ChildCount() == 2);

  auto* tab1_header = headers_slot->ChildAt(0);
  auto* tab2_header = headers_slot->ChildAt(1);
  CHECK(tab1_header->classes[1] == "active-tab");

  // Find pane elements initially
  Element* pane1_el = nullptr;
  Element* pane2_el = nullptr;
  auto find_panes = [&]() {
    pane1_el = nullptr;
    pane2_el = nullptr;
    container->Root()->Visit([&](Element& el) {
      if (el.tag() == "tab-pane" || el.tag() == "tab_pane") {
        if (el.Attributes().count("name")) {
          auto name = el.Attributes().at("name");
          if (name == "tab1") pane1_el = &el;
          if (name == "tab2") pane2_el = &el;
        }
      }
    });
  };
  find_panes();
  REQUIRE(pane1_el != nullptr);
  REQUIRE(pane2_el != nullptr);

  // Verify initial classes and display styles (tab1 active, tab2 inactive)
  CHECK(pane1_el->classes[0] == "active");
  CHECK(pane1_el->style.display_none == false);
  CHECK(pane2_el->classes[0] == "inactive");
  CHECK(pane2_el->style.display_none == true);

  // Select tab2 via index
  tabs_ptr->SelectTab("1");
  container->Digest();
  screen.Draw();

  CHECK(tabs_ptr->value == "tab2");
  CHECK(container->active_tab == "tab2");

  // Re-find pane elements after re-render/Digest
  find_panes();
  REQUIRE(pane1_el != nullptr);
  REQUIRE(pane2_el != nullptr);

  // Verify switched classes and display styles (tab1 inactive, tab2 active)
  CHECK(pane1_el->classes[0] == "inactive");
  CHECK(pane1_el->style.display_none == true);
  CHECK(pane2_el->classes[0] == "active");
  CHECK(pane2_el->style.display_none == false);
}

TEST_CASE("Tabs Component Interactions", "[component][tabs][interaction]") {
  auto container = rtxui::Ref<TabsTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* tabs_el = container->Root()->QuerySelector("tabs");
  REQUIRE(tabs_el != nullptr);
  auto* tabs_ptr = dynamic_cast<rtxui::tabs*>(const_cast<rtxui::ComponentBase*>(tabs_el->component()));
  REQUIRE(tabs_ptr != nullptr);

  // Initial tab: tab1
  CHECK(tabs_ptr->value == "tab1");

  auto headers_slot = tabs_ptr->Slot("headers");
  REQUIRE(headers_slot != nullptr);
  REQUIRE(headers_slot->ChildCount() == 2);

  auto* tab2_header = headers_slot->ChildAt(1);
  REQUIRE(tab2_header != nullptr);

  // Click on the second tab header (tab2_header)
  Event::Mouse mouse_click;
  mouse_click.button = Event::Mouse::Button::Left;
  mouse_click.motion = Event::Mouse::Motion::Pressed;
  mouse_click.x = tab2_header->absolute_x() + 1;
  mouse_click.y = tab2_header->absolute_y() + 1;
  Event click_event(mouse_click);

  screen.Dispatch(click_event);
  container->Digest();
  screen.Draw();

  // Verify tab switched to tab2
  CHECK(tabs_ptr->value == "tab2");
  CHECK(container->active_tab == "tab2");

  // Verify keyboard interaction: focus the first tab header (tab1_header)
  auto* tab1_header = headers_slot->ChildAt(0);
  REQUIRE(tab1_header != nullptr);

  container->Root()->Visit([](Element& el) { el.set_focused(false); });
  tab1_header->set_focused(true);

  // Press Return key
  Event return_event = Event::Keyboard({
      Event::Keyboard::Motion::Pressed,
      Event::Keyboard::Special::Return,
  });
  screen.Dispatch(return_event);
  container->Digest();
  screen.Draw();

  // Verify tab switched back to tab1
  CHECK(tabs_ptr->value == "tab1");
}

// --- Dialog ---
class DialogTestComponent : public rtxui::Component<DialogTestComponent> {
 public:
  bool dialog_open = false;

  void InitReflection() override {
    Bind(dialog_open);
    Import<rtxui::dialog>();
    Import<rtxui::button>();
    rtxui::Component<DialogTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <dialog open="{dialog_open}" title="Confirm Dialog">
      Body Content
      <button id="dlg-btn">Confirm</button>
    </dialog>
  )";
};

TEST_CASE("Dialog Component Overlay", "[component][dialog]") {
  auto container = rtxui::Ref<DialogTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* dialog_el = container->Root()->QuerySelector("dialog");
  REQUIRE(dialog_el != nullptr);
  auto* dialog_ptr = dynamic_cast<rtxui::dialog*>(const_cast<rtxui::ComponentBase*>(dialog_el->component()));
  REQUIRE(dialog_ptr != nullptr);

  // Initial State: closed
  CHECK(dialog_ptr->open == false);
  CHECK(dialog_ptr->overlay_class == "closed");

  // Open dialog
  container->dialog_open = true;
  container->Digest();
  screen.Draw();

  CHECK(dialog_ptr->open == true);
  CHECK(dialog_ptr->overlay_class == "open");

  // Verify the nested button is present and queryable inside dialog slot
  auto* btn_el = dialog_el->QuerySelector("#dlg-btn");
  REQUIRE(btn_el != nullptr);
  CHECK(btn_el->tag() == "button");

  // Escape cancels the open dialog (and must not fall through to the
  // screen's Escape-to-quit handling).
  screen.Dispatch(Event::Escape());
  CHECK(dialog_ptr->open == false);
  CHECK(dialog_ptr->overlay_class == "closed");
}

TEST_CASE("Dialog Component Layout Centering", "[component][dialog][layout]") {
  auto container = rtxui::Ref<DialogTestComponent>::New();
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  device->TriggerResize(80, 24);
  rtxui::Screen screen(container, device);

  container->dialog_open = true;
  container->Digest();
  screen.Draw();

  auto* overlay_el = container->Root()->QuerySelector(".dialog-overlay");
  auto* box_el = container->Root()->QuerySelector(".dialog-box");
  REQUIRE(overlay_el != nullptr);
  REQUIRE(box_el != nullptr);

  CHECK(overlay_el->layout_width() == 80);
  CHECK(overlay_el->layout_height() == 24);

  int box_w = box_el->layout_width();
  int box_h = box_el->layout_height();
  REQUIRE(box_w > 0);
  REQUIRE(box_h > 0);

  int expected_x = (80 - box_w) / 2;
  int expected_y = (24 - box_h) / 2;

  CHECK(box_el->absolute_x() == expected_x);
  CHECK(box_el->absolute_y() == expected_y);
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
    std::filesystem::last_write_time(temp_file, current_time + std::chrono::seconds(2), ec);
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

// --- Label ---
class LabelTestComponent : public rtxui::Component<LabelTestComponent> {
 public:
  bool cb1_checked = false;
  bool cb2_checked = false;

  void InitReflection() override {
    Bind(cb1_checked);
    Bind(cb2_checked);
    Import<rtxui::label>();
    Import<rtxui::checkbox>();
    Import<rtxui::div>();
    rtxui::Component<LabelTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <div>
      <!-- Explicit association via for -->
      <label id="lbl1" for="cb1">Explicit Label</label>
      <checkbox id="cb1" checked="{cb1_checked}">CB1</checkbox>

      <!-- Implicit association via nesting -->
      <label id="lbl2">
        Implicit Label
        <checkbox id="cb2" checked="{cb2_checked}">CB2</checkbox>
      </label>
    </div>
  )";
};

TEST_CASE("Label Component Interaction", "[component][label]") {
  auto container = rtxui::Ref<LabelTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* lbl1_el = container->Root()->QuerySelector("#lbl1");
  auto* cb1_el = container->Root()->QuerySelector("#cb1");
  auto* lbl2_el = container->Root()->QuerySelector("#lbl2");
  auto* cb2_el = container->Root()->QuerySelector("#cb2");

  REQUIRE(lbl1_el != nullptr);
  REQUIRE(cb1_el != nullptr);
  REQUIRE(lbl2_el != nullptr);
  REQUIRE(cb2_el != nullptr);

  auto* lbl1_comp = const_cast<rtxui::ComponentBase*>(lbl1_el->component());
  auto* lbl2_comp = const_cast<rtxui::ComponentBase*>(lbl2_el->component());
  REQUIRE(lbl1_comp != nullptr);
  REQUIRE(lbl2_comp != nullptr);

  // Initial state: unchecked
  CHECK(container->cb1_checked == false);
  CHECK(container->cb2_checked == false);

  // 1. Click on Explicit Label (lbl1)
  Event::Mouse mouse1;
  mouse1.button = Event::Mouse::Button::Left;
  mouse1.motion = Event::Mouse::Motion::Pressed;
  mouse1.x = lbl1_el->absolute_x() + 1;
  mouse1.y = lbl1_el->absolute_y() + 1;
  Event click1(mouse1);
  
  CHECK(lbl1_comp->OnEvent(click1) == true);
  container->Digest();
  screen.Draw();

  // Verify cb1 toggled, cb2 unchanged, cb1 focused!
  CHECK(container->cb1_checked == true);
  CHECK(container->cb2_checked == false);
  CHECK(cb1_el->focused() == true);

  // 2. Click on Implicit Label (lbl2)
  Event::Mouse mouse2;
  mouse2.button = Event::Mouse::Button::Left;
  mouse2.motion = Event::Mouse::Motion::Pressed;
  mouse2.x = lbl2_el->absolute_x() + 1;
  mouse2.y = lbl2_el->absolute_y() + 1;
  Event click2(mouse2);

  CHECK(lbl2_comp->OnEvent(click2) == true);
  container->Digest();
  screen.Draw();

  // Verify cb2 toggled, cb2 focused!
  CHECK(container->cb1_checked == true);
  CHECK(container->cb2_checked == true);
  CHECK(cb2_el->focused() == true);
}

// --- Tooltip ---
class TooltipTestComponent : public rtxui::Component<TooltipTestComponent> {
 public:
  std::string tooltip_text = "Helpful Info";
  std::string tooltip_place = "top";

  void InitReflection() override {
    Bind(tooltip_text);
    Bind(tooltip_place);
    Import<rtxui::tooltip>();
    Import<rtxui::div>();
    rtxui::Component<TooltipTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <div>
      <tooltip id="tt" content="{tooltip_text}" placement="{tooltip_place}">
        <div id="trigger">Hover Me</div>
      </tooltip>
    </div>
  )";
};

TEST_CASE("Tooltip Component Interaction", "[component][tooltip]") {
  auto container = rtxui::Ref<TooltipTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* tt_el = container->Root()->QuerySelector("#tt");
  auto* trigger_el = container->Root()->QuerySelector("#trigger");
  REQUIRE(tt_el != nullptr);
  REQUIRE(trigger_el != nullptr);

  auto* tt_comp = const_cast<rtxui::ComponentBase*>(tt_el->component());
  auto* tt_ptr = static_cast<rtxui::tooltip*>(tt_comp);
  REQUIRE(tt_ptr != nullptr);

  // Initial state: not hovered, class is hidden
  CHECK(tt_ptr->tooltip_class == "hidden");

  // Simulated hover
  tt_el->set_hovered(true);
  container->Digest();
  screen.Draw();

  // Verify class is now visible
  CHECK(tt_ptr->tooltip_class == "visible");

  // Hover removed
  tt_el->set_hovered(false);
  container->Digest();
  screen.Draw();

  // Verify class is hidden again
  CHECK(tt_ptr->tooltip_class == "hidden");
}

TEST_CASE("Tooltip Placement Styles", "[component][tooltip][alignment]") {
  auto container = rtxui::Ref<TooltipTestComponent>::New();
  rtxui::Screen screen(container);

  auto* tt_el = container->Root()->QuerySelector("#tt");
  REQUIRE(tt_el != nullptr);

  tt_el->set_hovered(true);

  auto check_placement = [&](const std::string& place, auto verify_fn) {
    container->tooltip_place = place;
    container->Digest();
    screen.Draw();

    auto* popup = container->Root()->QuerySelector(".tooltip-popup");
    REQUIRE(popup != nullptr);
    verify_fn(popup->style);
  };

  SECTION("Top Placements") {
    check_placement("top", [](const rtxui::ComputedStyle& style) {
      CHECK(style.bottom.unit == rtxui::Unit::Percent);
      CHECK(style.bottom.value == 100.0f);
      CHECK(style.left.unit == rtxui::Unit::Cells);
      CHECK(style.left.value == 0.0f);
      CHECK(style.right.unit == rtxui::Unit::Cells);
      CHECK(style.right.value == 0.0f);
      CHECK(style.margin_left_auto == true);
      CHECK(style.margin_right_auto == true);
    });

    check_placement("top-start", [](const rtxui::ComputedStyle& style) {
      CHECK(style.bottom.unit == rtxui::Unit::Percent);
      CHECK(style.bottom.value == 100.0f);
      CHECK(style.left.unit == rtxui::Unit::Cells);
      CHECK(style.left.value == 0.0f);
      CHECK(style.right.unit == rtxui::Unit::Auto);
      CHECK(style.margin_left_auto == false);
      CHECK(style.margin_right_auto == true);
    });

    check_placement("top-end", [](const rtxui::ComputedStyle& style) {
      CHECK(style.bottom.unit == rtxui::Unit::Percent);
      CHECK(style.bottom.value == 100.0f);
      CHECK(style.left.unit == rtxui::Unit::Auto);
      CHECK(style.right.unit == rtxui::Unit::Cells);
      CHECK(style.right.value == 0.0f);
      CHECK(style.margin_left_auto == true);
      CHECK(style.margin_right_auto == false);
    });
  }

  SECTION("Bottom Placements") {
    check_placement("bottom", [](const rtxui::ComputedStyle& style) {
      CHECK(style.top.unit == rtxui::Unit::Percent);
      CHECK(style.top.value == 100.0f);
      CHECK(style.left.unit == rtxui::Unit::Cells);
      CHECK(style.left.value == 0.0f);
      CHECK(style.right.unit == rtxui::Unit::Cells);
      CHECK(style.right.value == 0.0f);
      CHECK(style.margin_left_auto == true);
      CHECK(style.margin_right_auto == true);
    });

    check_placement("bottom-start", [](const rtxui::ComputedStyle& style) {
      CHECK(style.top.unit == rtxui::Unit::Percent);
      CHECK(style.top.value == 100.0f);
      CHECK(style.left.unit == rtxui::Unit::Cells);
      CHECK(style.left.value == 0.0f);
      CHECK(style.right.unit == rtxui::Unit::Auto);
      CHECK(style.margin_left_auto == false);
      CHECK(style.margin_right_auto == true);
    });

    check_placement("bottom-end", [](const rtxui::ComputedStyle& style) {
      CHECK(style.top.unit == rtxui::Unit::Percent);
      CHECK(style.top.value == 100.0f);
      CHECK(style.left.unit == rtxui::Unit::Auto);
      CHECK(style.right.unit == rtxui::Unit::Cells);
      CHECK(style.right.value == 0.0f);
      CHECK(style.margin_left_auto == true);
      CHECK(style.margin_right_auto == false);
    });
  }

  SECTION("Left Placements") {
    check_placement("left", [](const rtxui::ComputedStyle& style) {
      CHECK(style.right.unit == rtxui::Unit::Percent);
      CHECK(style.right.value == 100.0f);
      CHECK(style.top.unit == rtxui::Unit::Cells);
      CHECK(style.top.value == 0.0f);
      CHECK(style.bottom.unit == rtxui::Unit::Cells);
      CHECK(style.bottom.value == 0.0f);
      CHECK(style.margin_top_auto == true);
      CHECK(style.margin_bottom_auto == true);
    });

    check_placement("left-start", [](const rtxui::ComputedStyle& style) {
      CHECK(style.right.unit == rtxui::Unit::Percent);
      CHECK(style.right.value == 100.0f);
      CHECK(style.top.unit == rtxui::Unit::Cells);
      CHECK(style.top.value == 0.0f);
      CHECK(style.bottom.unit == rtxui::Unit::Auto);
      CHECK(style.margin_top_auto == false);
      CHECK(style.margin_bottom_auto == true);
    });

    check_placement("left-end", [](const rtxui::ComputedStyle& style) {
      CHECK(style.right.unit == rtxui::Unit::Percent);
      CHECK(style.right.value == 100.0f);
      CHECK(style.bottom.unit == rtxui::Unit::Cells);
      CHECK(style.bottom.value == 0.0f);
      CHECK(style.top.unit == rtxui::Unit::Auto);
      CHECK(style.margin_top_auto == true);
      CHECK(style.margin_bottom_auto == false);
    });
  }

  SECTION("Right Placements") {
    check_placement("right", [](const rtxui::ComputedStyle& style) {
      CHECK(style.left.unit == rtxui::Unit::Percent);
      CHECK(style.left.value == 100.0f);
      CHECK(style.top.unit == rtxui::Unit::Cells);
      CHECK(style.top.value == 0.0f);
      CHECK(style.bottom.unit == rtxui::Unit::Cells);
      CHECK(style.bottom.value == 0.0f);
      CHECK(style.margin_top_auto == true);
      CHECK(style.margin_bottom_auto == true);
    });

    check_placement("right-start", [](const rtxui::ComputedStyle& style) {
      CHECK(style.left.unit == rtxui::Unit::Percent);
      CHECK(style.left.value == 100.0f);
      CHECK(style.top.unit == rtxui::Unit::Cells);
      CHECK(style.top.value == 0.0f);
      CHECK(style.bottom.unit == rtxui::Unit::Auto);
      CHECK(style.margin_top_auto == false);
      CHECK(style.margin_bottom_auto == true);
    });

    check_placement("right-end", [](const rtxui::ComputedStyle& style) {
      CHECK(style.left.unit == rtxui::Unit::Percent);
      CHECK(style.left.value == 100.0f);
      CHECK(style.bottom.unit == rtxui::Unit::Cells);
      CHECK(style.bottom.value == 0.0f);
      CHECK(style.top.unit == rtxui::Unit::Auto);
      CHECK(style.margin_top_auto == true);
      CHECK(style.margin_bottom_auto == false);
    });
  }
}

class TabsCrashPreventionTestComponent : public rtxui::Component<TabsCrashPreventionTestComponent> {
 public:
  std::string active_tab = "tab1";

  void InitReflection() override {
    Bind(active_tab);
    Import<rtxui::tabs>();
    Import<rtxui::tab_pane>();
    rtxui::Component<TabsCrashPreventionTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <tabs value="{active_tab}">
      <tab-pane label="Tab One" name="tab1">Content One</tab-pane>
    </tabs>
  )";
};

TEST_CASE("Tabs SelectTab Crash Prevention", "[component][tabs][bug]") {
  auto container = rtxui::Ref<TabsCrashPreventionTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* tabs_el = container->Root()->QuerySelector("tabs");
  REQUIRE(tabs_el != nullptr);
  auto* tabs_ptr = dynamic_cast<rtxui::tabs*>(const_cast<rtxui::ComponentBase*>(tabs_el->component()));
  REQUIRE(tabs_ptr != nullptr);

  CHECK_NOTHROW(tabs_ptr->SelectTab(""));
  CHECK_NOTHROW(tabs_ptr->SelectTab("invalid"));
  CHECK_NOTHROW(tabs_ptr->SelectTab("999"));
}

class SelectPreservationTestComponent : public rtxui::Component<SelectPreservationTestComponent> {
 public:
  std::string theme = "light";
  void InitReflection() override {
    Bind(theme);
    Import<rtxui::select>();
    Import<rtxui::option>();
    rtxui::Component<SelectPreservationTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <select value="{theme}">
      <option class="custom-class" value="dark">Dark Theme</option>
      <option class="custom-class" value="light">Light Theme</option>
    </select>
  )";
};

TEST_CASE("Select Option Class Preservation", "[component][select][bug]") {
  auto container = rtxui::Ref<SelectPreservationTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* opt_el = container->Root()->QuerySelector("option");
  REQUIRE(opt_el != nullptr);
  
  bool has_custom = std::find(opt_el->classes.begin(), opt_el->classes.end(), "custom-class") != opt_el->classes.end();
  CHECK(has_custom == true);

  container->theme = "dark";
  container->Digest();
  screen.Draw();

  has_custom = std::find(opt_el->classes.begin(), opt_el->classes.end(), "custom-class") != opt_el->classes.end();
  CHECK(has_custom == true);
}

class ComplexSelectorTestApp : public Component<ComplexSelectorTestApp> {
 public:
  void InitReflection() override {
    Import<rtxui::div>();
    Import<rtxui::span>();
    Import<rtxui::p>();
    Component<ComplexSelectorTestApp>::InitReflection();
  }
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

TEST_CASE("CSS Child and Descendant Combinator Matching", "[component][css]") {
  auto app = Ref<ComplexSelectorTestApp>::New();
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  device->TriggerResize(80, 24);
  rtxui::Screen screen(app, device);

  auto* root_div = app->Root();
  REQUIRE(root_div != nullptr);

  auto* direct_span = root_div->QuerySelector("#direct");
  REQUIRE(direct_span != nullptr);
  // It is a descendant of div (div span -> bg red) AND a direct child of div (div > span -> color green)
  CHECK(direct_span->style.background_color.value() == Color::RGB(255, 0, 0));
  CHECK(direct_span->style.foreground_color.value() == Color::RGB(0, 255, 0));

  auto* nested_span = root_div->QuerySelector("#nested");
  REQUIRE(nested_span != nullptr);
  // It is a descendant of div (div span -> bg red) but NOT a direct child (div > span should NOT match, color is default/unset)
  CHECK(nested_span->style.background_color.value() == Color::RGB(255, 0, 0));
  CHECK(!nested_span->style.foreground_color.has_value());
}

namespace rtxui {
void PrintCompilerStyleError(std::string_view source_string,
                             int error_line,
                             int error_column,
                             std::string_view message,
                             std::string_view label);
}

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

class AdvancedCSSFeaturesTestApp : public Component<AdvancedCSSFeaturesTestApp> {
 public:
  void InitReflection() override {
    Import<rtxui::div>();
    Import<rtxui::span>();
    Component<AdvancedCSSFeaturesTestApp>::InitReflection();
  }
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

TEST_CASE("CSS Sibling Combinators and Structural Pseudo-classes", "[component][css]") {
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








