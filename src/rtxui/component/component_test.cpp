// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/internal/component.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <iostream>

#include "rtxui/component/default_components_internal.hpp"
#include "rtxui/core/string.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/internal/screen.hpp"

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
          background-color: red;
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
      <p> This is a page: </p>
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
  // With 1 cell border and 1 cell padding on left and right, inner visible
  // width is 6.
  input_ptr->value = "123456789";
  input_ptr->cursor_pos = 7;
  input_ptr->Digest();
  // cursor_col = 7. scroll_x should adjust to 7 - 6 + 1 = 2.
  CHECK(input_el->scroll_x() == 2);
}

TEST_CASE("Input Component Layout Height", "[component]") {
  auto container = rtxui::Ref<InputTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* input_el = container->Root()->QuerySelector("input");
  REQUIRE(input_el != nullptr);
  // The layout height should be 3 cells: 1 cell for text content, plus 2 cells
  // for top/bottom borders.
  CHECK(input_el->layout_height() == 3);
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

}  // namespace
