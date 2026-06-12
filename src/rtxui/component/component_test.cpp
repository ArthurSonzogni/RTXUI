// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/internal/component.hpp"
#include "rtxui/paint/color.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <iostream>

#include "rtxui/component/default_components_internal.hpp"
#include "rtxui/core/string.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/internal/screen.hpp"
#include "rtxui/layout/layout.hpp"
#include "rtxui/layout/layout_tree_builder.hpp"
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
    pos += 3; // '─' is 3 bytes in UTF-8
  }
  
  if (track_count < 15) { // Allow some slack but ensure it's not squashed to ~3
    FAIL("Slider squashed. Track count=" << track_count << "\nOutput:\n" << output);
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
  // With no border and 1 cell padding on left and right, inner visible width is 8.
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

TEST_CASE("Input Component Advanced Selection and Editing", "[component][input]") {
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
    int x_pos = input_el->absolute_x() + 1 + 2 + 1; // 1 (border/padding) + 2 (index) + 1 (1-based)
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

TEST_CASE("Input Click Does Not Resize", "[component][input]") {
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  auto container = rtxui::Ref<InputTestComponent>::New();
  rtxui::Screen screen(container, device);
  screen.Draw();

  auto* input_el = container->Root()->QuerySelector("input");
  REQUIRE(input_el != nullptr);

  int initial_width  = input_el->layout_width();
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
  CHECK(input_el->layout_width()  == initial_width);
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

  int initial_width  = ta_el->layout_width();
  int initial_height = ta_el->layout_height();
  // The textarea should have a proper fixed size (width: 40, height: 5 default).
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
  CHECK(ta_el->layout_width()  == initial_width);
  CHECK(ta_el->layout_height() == initial_height);
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
  int track_w = std::max(2, slider_ptr->width);   // 11

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

  // Move to the centre of the track — y deliberately far off (capture ignores it)
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
  // Send click to option
  CHECK(dark_option_ptr->OnEvent(option_click_event) == true);
  container->Digest();
  screen.Draw();

  // Check state has been updated to "dark"
  CHECK(select_ptr->value == "dark");
  CHECK(container->my_theme == "dark");
  CHECK(select_ptr->selected_label == "Dark Theme");
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

class MarkdownListTestContainer : public rtxui::Component<MarkdownListTestContainer> {
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
  CHECK(RemoveWhitespace(print_disc) == "<ulid=\"ul_disc\"><li><span>•</span>item1</li><li><span>•</span>item2</li></ul>");

  // 2. Nested unordered list (outer disc -> "• ", inner circle -> "○ ")
  auto* ul_nested = root->QuerySelector("#ul_nested");
  REQUIRE(ul_nested != nullptr);
  auto print_nested = ul_nested->Print();
  CHECK(RemoveWhitespace(print_nested) == "<ulid=\"ul_nested\"><li><span>•</span>outer1<ul><li><span>○</span>inner1</li><li><span>○</span>inner2</li></ul></li></ul>");

  // 3. Ordered list with decimal numbering ("1. ", "2. ")
  auto* ol_decimal = root->QuerySelector("#ol_decimal");
  REQUIRE(ol_decimal != nullptr);
  auto print_decimal = ol_decimal->Print();
  CHECK(RemoveWhitespace(print_decimal) == "<olid=\"ol_decimal\"><li><span>1.</span>first</li><li><span>2.</span>second</li></ol>");

  // 4. Custom list-style-type: square ("■ ")
  auto* ul_custom_square = root->QuerySelector("#ul_custom_square");
  REQUIRE(ul_custom_square != nullptr);
  auto print_square = ul_custom_square->Print();
  CHECK(RemoveWhitespace(print_square) == "<ulid=\"ul_custom_square\"><li><span>■</span>squareitem</li></ul>");

  // 5. Custom list-style-type: none ("")
  auto* ul_custom_none = root->QuerySelector("#ul_custom_none");
  REQUIRE(ul_custom_none != nullptr);
  auto print_none = ul_custom_none->Print();
  CHECK(RemoveWhitespace(print_none) == "<ulid=\"ul_custom_none\"><li><span></span>noneitem</li></ul>");
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

TEST_CASE("CSS Tag and Class Selector Combination (e.g., button.active)", "[component][css]") {
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
    if (line.find("│") != std::string::npos || line.find("●") != std::string::npos) {
      rows_with_slider++;
    }
  }
  
  if (rows_with_slider < 5) {
    FAIL("Vertical slider should span 5 rows. Output:\n" << output);
  }
  CHECK(rows_with_slider >= 5);
}

TEST_CASE("Style caching regression test for multi-component resolution", "[component][style]") {
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

  // Child style (self) should still apply, but parent class style (.test-child) should be gone.
  CHECK(child_el->style.foreground_color.value() == Color::RGB(255, 0, 0));
  CHECK(!child_el->style.background_color.has_value());
}

TEST_CASE("Input Click Layout Regression Test", "[component][input][regression]") {
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

TEST_CASE("Textarea Click Layout Regression Test", "[component][textarea][regression]") {
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

TEST_CASE("Input Cursor Vertical Line Regression Test", "[component][input][cursor][regression]") {
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

class SmallInput : public rtxui::Component<SmallInput>, public rtxui::TextInputBase {
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

class SmallInputTestComponent : public rtxui::Component<SmallInputTestComponent> {
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

class SmallTextarea : public rtxui::Component<SmallTextarea>, public rtxui::TextInputBase {
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

class SmallTextareaTestComponent : public rtxui::Component<SmallTextareaTestComponent> {
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

TEST_CASE("Input Scrolling Regression Test", "[component][input][scroll][regression]") {
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

TEST_CASE("Textarea Scrolling Regression Test", "[component][textarea][scroll][regression]") {
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

TEST_CASE("Mouse capture is released when component is destroyed", "[component]") {
  ComponentBase* raw_comp_ptr = nullptr;
  {
    auto comp = rtxui::Ref<rtxui::slider>::New();
    comp->CaptureMouse();
    raw_comp_ptr = comp.get();
    CHECK(rtxui::ComponentBase::GetMouseCapturer() == raw_comp_ptr);
  }
  CHECK(rtxui::ComponentBase::GetMouseCapturer() == nullptr);
}

class HoverActiveTestComponent : public rtxui::Component<HoverActiveTestComponent> {
 public:
  std::string_view view = R"(
    <div id="test-node">Test</div>
  )";
};

TEST_CASE("Hovered and Active states are preserved across Render", "[component]") {
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

