// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/component.hpp"
#include "rtxui/component/default_components.hpp"
#include "rtxui/terminal/screen.hpp"
#include "rtxui/dom/element.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <iostream>

#include "rtxui/core/string.hpp"

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
    BindComputed(double_count);
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
  counter->Import("increment", [counter]() { counter->increment(); });

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
  input_el->set_layout_width(10); // total layout width of 10 cells
  // With 1 cell border and 1 cell padding on left and right, inner visible width is 6.
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
  // The layout height should be 3 cells: 1 cell for text content, plus 2 cells for top/bottom borders.
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

}  // namespace
