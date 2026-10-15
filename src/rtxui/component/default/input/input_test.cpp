// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/input/input.hpp"

#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "rtxui/base/clock.hpp"
#include "rtxui/color.hpp"
#include "rtxui/component.hpp"
#include "rtxui/component/component_internal.hpp"
#include "rtxui/component/default/input/text_input_base.hpp"
#include "rtxui/component/default/textarea/textarea.hpp"
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

class InputTestComponent : public rtxui::Component<InputTestComponent> {
 public:
  std::string my_text = "hello world";
  InputTestComponent() { Bind(my_text); }
  std::string_view view = R"(
    <input value="{my_text}" />
  )";
};

TEST_CASE("Input Component Basic Interactions", "[component]") {
  auto container = rtxui::Ref<InputTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* input_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("input");
  REQUIRE(input_el != nullptr);

  auto* input_ptr = ComponentAs<rtxui::input>(input_el);
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

class PasswordInputTestComponent
    : public rtxui::Component<PasswordInputTestComponent> {
 public:
  std::string pass = "secret123";
  std::string input_type = "password";
  PasswordInputTestComponent() {
    Bind(pass);
    Bind(input_type);
  }
  std::string_view view = R"(
    <input type="{input_type}" value="{pass}" />
  )";
};

TEST_CASE("Password Input Component Interaction and Masking",
          "[component][input][password]") {
  auto container = rtxui::Ref<PasswordInputTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* input_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("input");
  REQUIRE(input_el != nullptr);

  auto* input_ptr = ComponentAs<rtxui::input>(input_el);
  REQUIRE(input_ptr != nullptr);

  CHECK(input_ptr->value == "secret123");
  CHECK(input_ptr->type == "password");

  // Focus input and digest
  input_el->set_focused(true);
  input_ptr->Digest();

  // Cursor is at 0, display characters should be masked with '*'
  CHECK(input_ptr->cursor_char == "*");
  CHECK(input_ptr->right_unselected == "********");

  // Move cursor right
  input_ptr->OnEvent(Event::ArrowRight());
  input_ptr->Digest();
  CHECK(input_ptr->cursor_pos == 1);
  CHECK(input_ptr->left_unselected == "*");
  CHECK(input_ptr->cursor_char == "*");
  CHECK(input_ptr->right_unselected == "*******");

  // Type a character 'A'
  input_ptr->OnEvent(Event::Keyboard::From('A'));
  input_ptr->Digest();
  // Value retains plain text
  CHECK(input_ptr->value == "sAecret123");
  // Visual masking remains '*'
  CHECK(input_ptr->left_unselected == "**");

  // Backspace
  input_ptr->OnEvent(Event::Backspace());
  input_ptr->Digest();
  CHECK(input_ptr->value == "secret123");
}

TEST_CASE("Input Component Layout Height", "[component]") {
  auto container = rtxui::Ref<InputTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* input_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("input");
  REQUIRE(input_el != nullptr);
  // The layout height should be 1 cell (no borders).
  CHECK(input_el->layout_height() == 1);
}

TEST_CASE("Input Component Advanced Selection and Editing",
          "[component][input]") {
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  auto container = rtxui::Ref<InputTestComponent>::New();
  rtxui::Screen screen = rtxui::ScreenInternals::Make(container, device);
  screen.Draw();

  auto* input_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("input");
  REQUIRE(input_el != nullptr);

  auto* input_ptr = ComponentAs<rtxui::input>(input_el);
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

  SECTION("Triple Click Whole Line Selection") {
    input_ptr->value = "hello world test";
    input_ptr->cursor_pos = 0;
    input_ptr->selection_start = -1;
    input_ptr->Digest();

    int x_pos = input_el->absolute_x() + 1 + 2 + 1;
    Event::Mouse mouse;
    mouse.button = Event::Mouse::Button::Left;
    mouse.x = x_pos;
    mouse.y = input_el->absolute_y() + 1;

    for (int i = 0; i < 2; ++i) {
      mouse.motion = Event::Mouse::Motion::Pressed;
      screen.Dispatch(Event(mouse));
      mouse.motion = Event::Mouse::Motion::Released;
      screen.Dispatch(Event(mouse));
    }

    mouse.motion = Event::Mouse::Motion::Pressed;
    screen.Dispatch(Event(mouse));
    input_ptr->Digest();

    CHECK(input_ptr->selection_start == 0);
    CHECK(input_ptr->cursor_pos == 16);

    mouse.motion = Event::Mouse::Motion::Released;
    screen.Dispatch(Event(mouse));
    input_ptr->Digest();
    CHECK(input_ptr->selection_start == 0);
    CHECK(input_ptr->cursor_pos == 16);
  }

  SECTION("Quadruple Click Whole Paragraph Selection") {
    input_ptr->value = "hello world test";
    input_ptr->cursor_pos = 0;
    input_ptr->selection_start = -1;
    input_ptr->Digest();

    int x_pos = input_el->absolute_x() + 1 + 2 + 1;
    Event::Mouse mouse;
    mouse.button = Event::Mouse::Button::Left;
    mouse.x = x_pos;
    mouse.y = input_el->absolute_y() + 1;

    for (int i = 0; i < 3; ++i) {
      mouse.motion = Event::Mouse::Motion::Pressed;
      screen.Dispatch(Event(mouse));
      mouse.motion = Event::Mouse::Motion::Released;
      screen.Dispatch(Event(mouse));
    }

    mouse.motion = Event::Mouse::Motion::Pressed;
    screen.Dispatch(Event(mouse));
    input_ptr->Digest();

    CHECK(input_ptr->selection_start == 0);
    CHECK(input_ptr->cursor_pos == 16);

    mouse.motion = Event::Mouse::Motion::Released;
    screen.Dispatch(Event(mouse));
    input_ptr->Digest();
    CHECK(input_ptr->selection_start == 0);
    CHECK(input_ptr->cursor_pos == 16);
  }
}

TEST_CASE("Input Component State Preservation", "[component]") {
  auto container = rtxui::Ref<InputTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* input_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("input");
  REQUIRE(input_el != nullptr);
  auto* input_ptr = ComponentAs<rtxui::input>(input_el);
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
  rtxui::ComponentInternals::Mount(*container);

  auto* input_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("input");
  REQUIRE(input_el != nullptr);
  auto* input_ptr = ComponentAs<rtxui::input>(input_el);
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
  rtxui::Screen screen = rtxui::ScreenInternals::Make(container, device);
  screen.Draw();

  auto* input_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("input");
  REQUIRE(input_el != nullptr);
  auto* input_ptr = ComponentAs<rtxui::input>(input_el);
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

TEST_CASE(
    "Input Component Readonly Blocks Mutation But Allows Selection "
    "And Copy",
    "[component][input][readonly]") {
  auto container = rtxui::Ref<InputTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* input_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("input");
  REQUIRE(input_el != nullptr);
  auto* input_ptr = ComponentAs<rtxui::input>(input_el);
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
  std::string_view view = R"(<input disabled="true" readonly="true" />)";
};

TEST_CASE("Input Component disabled/readonly Attributes Parsed From XML",
          "[component][input][disabled][readonly]") {
  auto container = rtxui::Ref<DisabledReadonlyInputTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);
  container->Digest();

  auto* input_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("input");
  REQUIRE(input_el != nullptr);
  auto* input_ptr = ComponentAs<rtxui::input>(input_el);
  REQUIRE(input_ptr != nullptr);

  CHECK(input_ptr->disabled);
  CHECK(input_ptr->readonly);
}

TEST_CASE("Input Component :disabled pseudo-class dims the default style",
          "[component][input][disabled][style]") {
  auto container = rtxui::Ref<InputTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* input_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("input");
  REQUIRE(input_el != nullptr);

  rtxui::ComponentInternals::ResolveTargetStyles(*container);
  float enabled_opacity = input_el->target_style.opacity;

  input_el->set_disabled(true);
  rtxui::ComponentInternals::ResolveTargetStyles(*container);
  float disabled_opacity = input_el->target_style.opacity;

  CHECK(disabled_opacity < enabled_opacity);
}

TEST_CASE("Input Component Placeholder Shows Only When Value Is Empty",
          "[component][input][placeholder]") {
  auto container = rtxui::Ref<InputTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* input_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("input");
  REQUIRE(input_el != nullptr);
  auto* input_ptr = ComponentAs<rtxui::input>(input_el);
  REQUIRE(input_ptr != nullptr);

  input_ptr->placeholder = "Type here...";
  input_ptr->value = "";
  input_ptr->Digest();
  CHECK(input_ptr->placeholder_text == "Type here...");

  // Typing a character hides the placeholder, and never touches its text.
  input_el->set_focused(true);
  input_ptr->OnEvent(Event::Keyboard::From('a'));
  input_ptr->Digest();
  CHECK(input_ptr->value == "a");
  CHECK(input_ptr->placeholder_text == "");
  CHECK(input_ptr->placeholder == "Type here...");

  // Deleting it back to empty brings the placeholder back.
  input_ptr->OnEvent(Event::Backspace());
  input_ptr->Digest();
  CHECK(input_ptr->value == "");
  CHECK(input_ptr->placeholder_text == "Type here...");
}

TEST_CASE("Input Component Placeholder Attribute Parsed From XML",
          "[component][input][placeholder]") {
  class PlaceholderInputTestComponent
      : public rtxui::Component<PlaceholderInputTestComponent> {
   public:
    std::string my_text = "";
    PlaceholderInputTestComponent() { Bind(my_text); }
    std::string_view view = R"(
      <input value="{my_text}" placeholder="Search..." />
    )";
  };

  auto container = rtxui::Ref<PlaceholderInputTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);
  container->Digest();

  auto* input_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("input");
  REQUIRE(input_el != nullptr);
  auto* input_ptr = ComponentAs<rtxui::input>(input_el);
  REQUIRE(input_ptr != nullptr);

  CHECK(input_ptr->placeholder == "Search...");
  CHECK(input_ptr->placeholder_text == "Search...");
}

TEST_CASE("Input Component Placeholder Uses A Dim Fixed Color",
          "[component][input][placeholder][style]") {
  auto container = rtxui::Ref<InputTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* input_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("input");
  REQUIRE(input_el != nullptr);
  auto* input_ptr = ComponentAs<rtxui::input>(input_el);
  REQUIRE(input_ptr != nullptr);

  input_ptr->placeholder = "hint";
  input_ptr->value = "";
  input_ptr->Digest();
  rtxui::ComponentInternals::ResolveTargetStyles(*container);

  auto* placeholder_el = rtxui::ComponentInternals::Root(*container)
                             ->QuerySelector(".placeholder");
  REQUIRE(placeholder_el != nullptr);
  CHECK(placeholder_el->target_style.foreground_color.value() ==
        Color::RGB(150, 150, 150));
}

TEST_CASE("Input Component Maxlength Blocks Typing Past The Limit",
          "[component][input][maxlength]") {
  auto container = rtxui::Ref<InputTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* input_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("input");
  REQUIRE(input_el != nullptr);
  auto* input_ptr = ComponentAs<rtxui::input>(input_el);
  REQUIRE(input_ptr != nullptr);

  input_ptr->value = "";
  input_ptr->cursor_pos = 0;
  input_ptr->maxlength = 3;
  input_el->set_focused(true);
  input_ptr->Digest();

  input_ptr->OnEvent(Event::Keyboard::From('a'));
  input_ptr->Digest();
  input_ptr->OnEvent(Event::Keyboard::From('b'));
  input_ptr->Digest();
  input_ptr->OnEvent(Event::Keyboard::From('c'));
  input_ptr->Digest();
  CHECK(input_ptr->value == "abc");

  // A 4th character has no room and is dropped, but the key is still
  // consumed (matches how a real maxlength field swallows the keystroke
  // rather than beeping/propagating it elsewhere).
  bool handled = input_ptr->OnEvent(Event::Keyboard::From('d'));
  input_ptr->Digest();
  CHECK(handled);
  CHECK(input_ptr->value == "abc");

  // Backspace still works at the limit.
  input_ptr->OnEvent(Event::Backspace());
  input_ptr->Digest();
  CHECK(input_ptr->value == "ab");

  // ... freeing up room for one more character.
  input_ptr->OnEvent(Event::Keyboard::From('z'));
  input_ptr->Digest();
  CHECK(input_ptr->value == "abz");
}

TEST_CASE("Input Component Maxlength Truncates A Paste To Fit",
          "[component][input][maxlength]") {
  // Pasted text arrives as one synthetic keyboard event per character (see
  // TerminalInputParser::EmitPastedText), so each character-insert event
  // already re-checks maxlength -- a paste naturally gets truncated to fit
  // rather than rejected outright, matching real browsers.
  auto container = rtxui::Ref<InputTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* input_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("input");
  REQUIRE(input_el != nullptr);
  auto* input_ptr = ComponentAs<rtxui::input>(input_el);
  REQUIRE(input_ptr != nullptr);

  input_ptr->value = "";
  input_ptr->cursor_pos = 0;
  input_ptr->maxlength = 3;
  input_el->set_focused(true);
  input_ptr->Digest();

  for (char c : std::string("hello")) {
    input_ptr->OnEvent(Event::Keyboard::From(c));
    input_ptr->Digest();
  }
  CHECK(input_ptr->value == "hel");
}

TEST_CASE("Input Component Maxlength Allows Replacing A Selection At The Limit",
          "[component][input][maxlength]") {
  auto container = rtxui::Ref<InputTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* input_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("input");
  REQUIRE(input_el != nullptr);
  auto* input_ptr = ComponentAs<rtxui::input>(input_el);
  REQUIRE(input_ptr != nullptr);

  input_ptr->value = "abc";
  input_ptr->maxlength = 3;
  input_ptr->selection_start = 0;
  input_ptr->cursor_pos = 3;  // whole value selected
  input_el->set_focused(true);
  input_ptr->Digest();

  // At the limit, but replacing the (fully selected) value is allowed
  // because the selection is deleted before the new character is counted.
  input_ptr->OnEvent(Event::Keyboard::From('x'));
  input_ptr->Digest();
  CHECK(input_ptr->value == "x");
}

TEST_CASE("Input Component Maxlength Attribute Parsed From XML",
          "[component][input][maxlength]") {
  class MaxlengthInputTestComponent
      : public rtxui::Component<MaxlengthInputTestComponent> {
   public:
    std::string my_text = "";
    MaxlengthInputTestComponent() { Bind(my_text); }
    std::string_view view = R"(
      <input value="{my_text}" maxlength="5" />
    )";
  };

  auto container = rtxui::Ref<MaxlengthInputTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);
  container->Digest();

  auto* input_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("input");
  REQUIRE(input_el != nullptr);
  auto* input_ptr = ComponentAs<rtxui::input>(input_el);
  REQUIRE(input_ptr != nullptr);

  CHECK(input_ptr->maxlength == 5);
}

TEST_CASE("Input Component Undo/Redo Basic Single Character",
          "[component][input][undo]") {
  auto container = rtxui::Ref<InputTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* input_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("input");
  REQUIRE(input_el != nullptr);
  auto* input_ptr = ComponentAs<rtxui::input>(input_el);
  REQUIRE(input_ptr != nullptr);

  input_ptr->value = "";
  input_ptr->cursor_pos = 0;
  input_el->set_focused(true);
  input_ptr->Digest();

  input_ptr->OnEvent(Event::Keyboard::From('a'));
  input_ptr->Digest();
  CHECK(input_ptr->value == "a");

  input_ptr->OnEvent(Event::CtrlZ());
  input_ptr->Digest();
  CHECK(input_ptr->value == "");
  CHECK(input_ptr->cursor_pos == 0);

  // Redo via Ctrl+Y.
  input_ptr->OnEvent(Event::CtrlY());
  input_ptr->Digest();
  CHECK(input_ptr->value == "a");
  CHECK(input_ptr->cursor_pos == 1);
}

TEST_CASE("Input Component Undo/Redo Via Ctrl+Shift+Z",
          "[component][input][undo]") {
  auto container = rtxui::Ref<InputTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* input_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("input");
  REQUIRE(input_el != nullptr);
  auto* input_ptr = ComponentAs<rtxui::input>(input_el);
  REQUIRE(input_ptr != nullptr);

  input_ptr->value = "";
  input_ptr->cursor_pos = 0;
  input_el->set_focused(true);
  input_ptr->Digest();

  input_ptr->OnEvent(Event::Keyboard::From('a'));
  input_ptr->Digest();
  input_ptr->OnEvent(Event::CtrlZ());
  input_ptr->Digest();
  CHECK(input_ptr->value == "");

  Event::Keyboard redo_kb;
  redo_kb.codepoint = 'z';
  redo_kb.modifier.ctrl = true;
  redo_kb.modifier.shift = true;
  input_ptr->OnEvent(Event(redo_kb));
  input_ptr->Digest();
  CHECK(input_ptr->value == "a");
}

TEST_CASE("Input Component Undo Groups Contiguous Typing Into One Step",
          "[component][input][undo]") {
  auto container = rtxui::Ref<InputTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* input_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("input");
  REQUIRE(input_el != nullptr);
  auto* input_ptr = ComponentAs<rtxui::input>(input_el);
  REQUIRE(input_ptr != nullptr);

  input_ptr->value = "";
  input_ptr->cursor_pos = 0;
  input_el->set_focused(true);
  input_ptr->Digest();

  for (char c : std::string("abc")) {
    input_ptr->OnEvent(Event::Keyboard::From(c));
    input_ptr->Digest();
  }
  CHECK(input_ptr->value == "abc");

  // One undo removes the whole contiguously-typed run, not just the 'c'.
  input_ptr->OnEvent(Event::CtrlZ());
  input_ptr->Digest();
  CHECK(input_ptr->value == "");

  // There is exactly one undo step recorded for the whole run: undo is now
  // a no-op.
  input_ptr->OnEvent(Event::CtrlZ());
  input_ptr->Digest();
  CHECK(input_ptr->value == "");
}

TEST_CASE("Input Component Cursor Move Breaks The Undo Group",
          "[component][input][undo]") {
  auto container = rtxui::Ref<InputTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* input_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("input");
  REQUIRE(input_el != nullptr);
  auto* input_ptr = ComponentAs<rtxui::input>(input_el);
  REQUIRE(input_ptr != nullptr);

  input_ptr->value = "";
  input_ptr->cursor_pos = 0;
  input_el->set_focused(true);
  input_ptr->Digest();

  input_ptr->OnEvent(Event::Keyboard::From('a'));
  input_ptr->Digest();
  input_ptr->OnEvent(Event::Keyboard::From('b'));
  input_ptr->Digest();
  CHECK(input_ptr->value == "ab");

  // Moving the cursor between edits must start a new undo group, even
  // though the next edit is still a plain character insert.
  input_ptr->OnEvent(Event::ArrowLeft());
  input_ptr->Digest();
  input_ptr->OnEvent(Event::Keyboard::From('c'));
  input_ptr->Digest();
  CHECK(input_ptr->value == "acb");

  input_ptr->OnEvent(Event::CtrlZ());
  input_ptr->Digest();
  CHECK(input_ptr->value == "ab");  // only the 'c' insert is undone

  input_ptr->OnEvent(Event::CtrlZ());
  input_ptr->Digest();
  CHECK(input_ptr->value == "");  // the earlier "ab" run undoes as one step
}

TEST_CASE(
    "Input Component Undo Restores Deleted Text; Consecutive "
    "Backspaces Merge",
    "[component][input][undo]") {
  auto container = rtxui::Ref<InputTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* input_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("input");
  REQUIRE(input_el != nullptr);
  auto* input_ptr = ComponentAs<rtxui::input>(input_el);
  REQUIRE(input_ptr != nullptr);

  input_ptr->value = "abc";
  input_ptr->cursor_pos = 3;
  input_el->set_focused(true);
  input_ptr->Digest();

  input_ptr->OnEvent(Event::Backspace());
  input_ptr->Digest();
  input_ptr->OnEvent(Event::Backspace());
  input_ptr->Digest();
  CHECK(input_ptr->value == "a");

  // One undo restores the whole contiguous backspace run.
  input_ptr->OnEvent(Event::CtrlZ());
  input_ptr->Digest();
  CHECK(input_ptr->value == "abc");
  CHECK(input_ptr->cursor_pos == 3);
}

TEST_CASE("Input Component New Edit After Undo Clears The Redo Stack",
          "[component][input][undo]") {
  auto container = rtxui::Ref<InputTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* input_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("input");
  REQUIRE(input_el != nullptr);
  auto* input_ptr = ComponentAs<rtxui::input>(input_el);
  REQUIRE(input_ptr != nullptr);

  input_ptr->value = "";
  input_ptr->cursor_pos = 0;
  input_el->set_focused(true);
  input_ptr->Digest();

  input_ptr->OnEvent(Event::Keyboard::From('a'));
  input_ptr->Digest();
  input_ptr->OnEvent(Event::CtrlZ());
  input_ptr->Digest();
  CHECK(input_ptr->value == "");

  input_ptr->OnEvent(Event::Keyboard::From('z'));
  input_ptr->Digest();
  CHECK(input_ptr->value == "z");

  // Redo has nothing to reapply: the 'a' branch was abandoned once a new
  // edit happened.
  input_ptr->OnEvent(Event::CtrlY());
  input_ptr->Digest();
  CHECK(input_ptr->value == "z");
}

TEST_CASE(
    "Input Component Pasted Text Undoes As One Step, Separate From "
    "Surrounding Typing",
    "[component][input][undo]") {
  auto container = rtxui::Ref<InputTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* input_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("input");
  REQUIRE(input_el != nullptr);
  auto* input_ptr = ComponentAs<rtxui::input>(input_el);
  REQUIRE(input_ptr != nullptr);

  input_ptr->value = "";
  input_ptr->cursor_pos = 0;
  input_el->set_focused(true);
  input_ptr->Digest();

  input_ptr->OnEvent(Event::Keyboard::From('a'));
  input_ptr->Digest();

  for (char c : std::string("xyz")) {
    Event::Keyboard kb = Event::Keyboard::From(c);
    kb.from_paste = true;
    input_ptr->OnEvent(Event(kb));
    input_ptr->Digest();
  }
  CHECK(input_ptr->value == "axyz");

  input_ptr->OnEvent(Event::Keyboard::From('b'));
  input_ptr->Digest();
  CHECK(input_ptr->value == "axyzb");

  // Undo order: the trailing manual 'b', then the whole paste as one
  // step, then the leading manual 'a' -- three separate steps, not five.
  input_ptr->OnEvent(Event::CtrlZ());
  input_ptr->Digest();
  CHECK(input_ptr->value == "axyz");

  input_ptr->OnEvent(Event::CtrlZ());
  input_ptr->Digest();
  CHECK(input_ptr->value == "a");

  input_ptr->OnEvent(Event::CtrlZ());
  input_ptr->Digest();
  CHECK(input_ptr->value == "");
}

TEST_CASE("Input Component Undo/Redo Blocked While Readonly Or Disabled",
          "[component][input][undo][readonly][disabled]") {
  auto container = rtxui::Ref<InputTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* input_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("input");
  REQUIRE(input_el != nullptr);
  auto* input_ptr = ComponentAs<rtxui::input>(input_el);
  REQUIRE(input_ptr != nullptr);

  input_ptr->value = "";
  input_ptr->cursor_pos = 0;
  input_el->set_focused(true);
  input_ptr->Digest();

  input_ptr->OnEvent(Event::Keyboard::From('a'));
  input_ptr->Digest();
  CHECK(input_ptr->value == "a");

  input_ptr->readonly = true;
  input_ptr->Digest();
  input_ptr->OnEvent(Event::CtrlZ());
  input_ptr->Digest();
  CHECK(input_ptr->value == "a");  // undo is blocked

  input_ptr->readonly = false;
  input_ptr->disabled = true;
  input_ptr->Digest();
  bool handled = input_ptr->OnEvent(Event::CtrlZ());
  input_ptr->Digest();
  CHECK_FALSE(handled);
  CHECK(input_ptr->value == "a");
}

TEST_CASE("Input Component Undo With Empty History Is A No-op",
          "[component][input][undo]") {
  auto container = rtxui::Ref<InputTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* input_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("input");
  REQUIRE(input_el != nullptr);
  auto* input_ptr = ComponentAs<rtxui::input>(input_el);
  REQUIRE(input_ptr != nullptr);

  input_el->set_focused(true);
  input_ptr->Digest();

  std::string original = input_ptr->value;
  bool handled = input_ptr->OnEvent(Event::CtrlZ());
  input_ptr->Digest();
  CHECK(handled);
  CHECK(input_ptr->value == original);
}

TEST_CASE("Input Component Cut Is Undoable", "[component][input][undo]") {
  auto container = rtxui::Ref<InputTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* input_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("input");
  REQUIRE(input_el != nullptr);
  auto* input_ptr = ComponentAs<rtxui::input>(input_el);
  REQUIRE(input_ptr != nullptr);

  input_ptr->value = "hello world";
  input_ptr->selection_start = 0;
  input_ptr->cursor_pos = 5;  // selects "hello"
  input_el->set_focused(true);
  input_ptr->Digest();

  input_ptr->OnEvent(Event::CtrlX());
  input_ptr->Digest();
  CHECK(input_ptr->value == " world");

  input_ptr->OnEvent(Event::CtrlZ());
  input_ptr->Digest();
  CHECK(input_ptr->value == "hello world");
}

TEST_CASE("Input Click Does Not Resize", "[component][input]") {
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  auto container = rtxui::Ref<InputTestComponent>::New();
  rtxui::Screen screen = rtxui::ScreenInternals::Make(container, device);
  screen.Draw();

  auto* input_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("input");
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
  TextareaTestComponent() { Bind(my_text); }
  std::string_view view = R"(
    <textarea value="{my_text}" />
  )";
};

TEST_CASE(
    "Input Component Selection/Cursor/Placeholder Expose Part "
    "Attributes For External ::part() Styling",
    "[component][input][part]") {
  auto container = rtxui::Ref<InputTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* input_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("input");
  REQUIRE(input_el != nullptr);
  auto* input_ptr = ComponentAs<rtxui::input>(input_el);
  REQUIRE(input_ptr != nullptr);

  input_ptr->selection_start = 0;
  input_ptr->cursor_pos = 5;  // selects "hello" out of "hello world"
  input_ptr->Digest();

  auto* selection_el = input_el->QuerySelector(".selection");
  REQUIRE(selection_el != nullptr);
  const std::string* selection_part = selection_el->GetAttribute("part");
  REQUIRE(selection_part != nullptr);
  CHECK(*selection_part == "selection");

  auto* cursor_el = input_el->QuerySelector(".cursor");
  REQUIRE(cursor_el != nullptr);
  const std::string* cursor_part = cursor_el->GetAttribute("part");
  REQUIRE(cursor_part != nullptr);
  CHECK(cursor_part->find("cursor") != std::string::npos);

  auto* placeholder_el = input_el->QuerySelector(".placeholder");
  REQUIRE(placeholder_el != nullptr);
  const std::string* placeholder_part = placeholder_el->GetAttribute("part");
  REQUIRE(placeholder_part != nullptr);
  CHECK(*placeholder_part == "placeholder");
}

}  // namespace

TEST_CASE("Input Click Layout Regression Test",
          "[component][input][regression]") {
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  auto container = rtxui::Ref<InputTestComponent>::New();
  rtxui::Screen screen = rtxui::ScreenInternals::Make(container, device);
  screen.Draw();

  auto* input_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("input");
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

TEST_CASE("Input Cursor Vertical Line Regression Test",
          "[component][input][cursor][regression]") {
  auto container = rtxui::Ref<InputTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* input_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("input");
  REQUIRE(input_el != nullptr);

  auto* input_ptr = ComponentAs<rtxui::input>(input_el);
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

namespace {

class SmallInput : public rtxui::Component<SmallInput>,
                   public rtxui::TextInputBase {
 public:
  SmallInput() {
    Bind(left_text);
    Bind(cursor_char);
    Bind(right_text);
    Bind(cursor_class);
  }
  std::string_view Setup() {
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
  SmallInputTestComponent() {
    Bind(my_text);
    Import<SmallInput>();
  }
  std::string_view view = R"(
    <SmallInput value="{my_text}" />
  )";
};

}  // namespace

TEST_CASE("Input Scrolling Regression Test",
          "[component][input][scroll][regression]") {
  auto container = rtxui::Ref<SmallInputTestComponent>::New();
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  rtxui::Screen screen = rtxui::ScreenInternals::Make(container, device);

  auto* input_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("SmallInput");
  REQUIRE(input_el != nullptr);

  auto* input_ptr = ComponentAs<SmallInput>(input_el);
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

namespace {

class InputReuseTestComponent
    : public rtxui::Component<InputReuseTestComponent> {
 public:
  std::string value = "initial";
  std::string_view view = R"(
    <div>
      <input id="test-input" value="{value}" />
    </div>
  )";
  InputReuseTestComponent() { Bind(value); }
};

}  // namespace

TEST_CASE("Input component is not recreated when attributes change",
          "[component][input]") {
  auto container = rtxui::Ref<InputReuseTestComponent>::New();
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  device->TriggerResize(80, 24);
  rtxui::Screen screen = rtxui::ScreenInternals::Make(container, device);

  auto* input_element =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("#test-input");
  REQUIRE(input_element != nullptr);
  auto* input_component = input_element->component();
  REQUIRE(input_component != nullptr);

  // Now change value
  container->value = "updated";
  rtxui::ComponentInternals::Render(*container);

  auto* new_input_element =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("#test-input");
  REQUIRE(new_input_element != nullptr);
  CHECK(new_input_element->component() == input_component);
}

TEST_CASE("Flickering transition is not triggered on input navigation",
          "[component][input][transition]") {
  // Regression guard for the flicker bug fixed in 8addbd9 ("remove
  // hover/focus color transitions on input/textarea"): input's self:focus
  // opacity/transition rules were deleted outright (a washed-out look when
  // hover+focus both applied), so opacity is now a constant 0.8 regardless
  // of focus state, and no transition ever starts. This still guards
  // against a future focus-triggered transition being re-added without
  // re-checking that flicker.
  auto container = rtxui::Ref<InputReuseTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* input_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("#test-input");
  REQUIRE(input_el != nullptr);

  // Focus the input
  input_el->set_focused(true);

  // Initial render
  rtxui::ComponentInternals::Render(*container);

  double now = rtxui::time::GetTimeMs();

  // Complete any focus transition
  input_el->TickTransitions(now + 200.0);
  CHECK(input_el->style.opacity == 0.8f);
  CHECK(input_el->active_transitions.empty());

  // Simulate navigating with ArrowLeft
  auto* input_ptr = ComponentAs<rtxui::input>(input_el);
  REQUIRE(input_ptr != nullptr);
  input_ptr->OnEvent(Event::ArrowLeft());
  input_ptr->Digest();

  // Check that no transition was triggered and opacity stayed constant
  CHECK(input_el->active_transitions.empty());
  CHECK(input_el->style.opacity == 0.8f);
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
  rtxui::ComponentInternals::Mount(*container);
  auto* input_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("#test-input");
  REQUIRE(input_el != nullptr);

  input_el->set_focused(true);
  rtxui::ComponentInternals::ResolveTargetStyles(*container);
  Color focus_only = input_el->target_style.background_color.value();

  input_el->set_hovered(true);  // still focused too, as after a real click
  rtxui::ComponentInternals::ResolveTargetStyles(*container);
  Color hover_and_focus = input_el->target_style.background_color.value();

  CHECK(hover_and_focus == focus_only);
}

namespace {

class SelectionHighlightTestComponent
    : public rtxui::Component<SelectionHighlightTestComponent> {
 public:
  std::string_view view = R"html(
    <input id="test-input" class="custom-input" />
    <style>
      .custom-input {
        background-color: rgb(230, 230, 200);
      }
    </style>
  )html";
};

}  // namespace

TEST_CASE(
    "Input selection highlight is readable regardless of the "
    "app-chosen background color",
    "[component][input][style]") {
  // .selection is a separate <span>, not self: background-color doesn't
  // inherit, so it can't derive from whatever color the app gives self.
  // It uses a fixed, readable color pair instead of trying to (and it
  // can't currently be made to derive from an app override; see the
  // git history for why lighten()/darken() and CSS custom properties
  // don't work here).
  auto container = rtxui::Ref<SelectionHighlightTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);
  container->Digest();

  auto* input_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("#test-input");
  REQUIRE(input_el != nullptr);
  auto* input_ptr = ComponentAs<rtxui::input>(input_el);
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

  auto* selection_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector(".selection");
  REQUIRE(selection_el != nullptr);
  CHECK(selection_el->base_style.background_color.value() ==
        Color::RGB(38, 79, 120));
  CHECK(selection_el->base_style.foreground_color.value() ==
        Color::RGB(255, 255, 255));
}

TEST_CASE("TextInputBase Ctrl+Home and Ctrl+End",
          "[component][input][navigation]") {
  auto container = rtxui::Ref<TextareaTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);
  auto* textarea_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
  REQUIRE(textarea_el != nullptr);
  auto* textarea_ptr = ComponentAs<rtxui::textarea>(textarea_el);
  REQUIRE(textarea_ptr != nullptr);

  textarea_ptr->value = "line 1\nline 2\nline 3";
  textarea_ptr->cursor_pos = 10;
  textarea_el->set_focused(true);
  textarea_ptr->Digest();

  // Ctrl+Home moves to 0
  Event::Keyboard kb;
  kb.special = Event::Keyboard::Special::Home;
  kb.modifier.ctrl = true;
  textarea_ptr->OnEvent(Event(kb));
  textarea_ptr->Digest();
  CHECK(textarea_ptr->cursor_pos == 0);
  CHECK(textarea_ptr->selection_start == -1);

  // Ctrl+End moves to end
  kb.special = Event::Keyboard::Special::End;
  textarea_ptr->OnEvent(Event(kb));
  textarea_ptr->Digest();
  CHECK(textarea_ptr->cursor_pos ==
        static_cast<int>(textarea_ptr->value.size()));
  CHECK(textarea_ptr->selection_start == -1);

  // Shift+Ctrl+Home selects to 0
  kb.special = Event::Keyboard::Special::Home;
  kb.modifier.shift = true;
  textarea_ptr->OnEvent(Event(kb));
  textarea_ptr->Digest();
  CHECK(textarea_ptr->cursor_pos == 0);
  CHECK(textarea_ptr->selection_start ==
        static_cast<int>(textarea_ptr->value.size()));

  // Shift+Ctrl+End expands selection to end
  kb.special = Event::Keyboard::Special::End;
  textarea_ptr->OnEvent(Event(kb));
  textarea_ptr->Digest();
  CHECK(textarea_ptr->cursor_pos ==
        static_cast<int>(textarea_ptr->value.size()));
  CHECK(textarea_ptr->selection_start ==
        static_cast<int>(textarea_ptr->value.size()));
}

TEST_CASE("TextInputBase Smart Home", "[component][input][navigation]") {
  auto container = rtxui::Ref<TextareaTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);
  auto* textarea_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
  REQUIRE(textarea_el != nullptr);
  auto* textarea_ptr = ComponentAs<rtxui::textarea>(textarea_el);
  REQUIRE(textarea_ptr != nullptr);

  textarea_ptr->value = "    code here";
  textarea_ptr->cursor_pos = 8;  // inside "code"
  textarea_el->set_focused(true);
  textarea_ptr->Digest();

  Event::Keyboard kb;
  kb.special = Event::Keyboard::Special::Home;

  // First Home jumps to first non-whitespace (index 4)
  textarea_ptr->OnEvent(Event(kb));
  textarea_ptr->Digest();
  CHECK(textarea_ptr->cursor_pos == 4);

  // Second Home toggles to column 0 (index 0)
  textarea_ptr->OnEvent(Event(kb));
  textarea_ptr->Digest();
  CHECK(textarea_ptr->cursor_pos == 0);

  // Third Home jumps back to index 4
  textarea_ptr->OnEvent(Event(kb));
  textarea_ptr->Digest();
  CHECK(textarea_ptr->cursor_pos == 4);

  // Shift+Home from index 8 selects to 4, then 0
  textarea_ptr->cursor_pos = 8;
  textarea_ptr->selection_start = -1;
  kb.modifier.shift = true;
  textarea_ptr->OnEvent(Event(kb));
  textarea_ptr->Digest();
  CHECK(textarea_ptr->cursor_pos == 4);
  CHECK(textarea_ptr->selection_start == 8);

  textarea_ptr->OnEvent(Event(kb));
  textarea_ptr->Digest();
  CHECK(textarea_ptr->cursor_pos == 0);
  CHECK(textarea_ptr->selection_start == 8);
}

TEST_CASE("TextInputBase Duplicate Line and Selection",
          "[component][input][duplicate]") {
  auto container = rtxui::Ref<TextareaTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);
  auto* textarea_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
  REQUIRE(textarea_el != nullptr);
  auto* textarea_ptr = ComponentAs<rtxui::textarea>(textarea_el);
  REQUIRE(textarea_ptr != nullptr);

  textarea_ptr->value = "hello\nworld";
  textarea_el->set_focused(true);

  // Duplicate line without selection via Ctrl+D
  textarea_ptr->cursor_pos = 2;
  textarea_ptr->Digest();

  Event::Keyboard ctrl_d;
  ctrl_d.codepoint = 'd';
  ctrl_d.modifier.ctrl = true;

  textarea_ptr->OnEvent(Event(ctrl_d));
  textarea_ptr->Digest();
  CHECK(textarea_ptr->value == "hello\nhello\nworld");

  // Duplicate selection via Shift+Alt+Down
  textarea_ptr->selection_start = 0;
  textarea_ptr->cursor_pos = 5;  // "hello"
  textarea_ptr->Digest();

  Event::Keyboard shift_alt_down;
  shift_alt_down.special = Event::Keyboard::Special::ArrowDown;
  shift_alt_down.modifier.shift = true;
  shift_alt_down.modifier.alt = true;

  textarea_ptr->OnEvent(Event(shift_alt_down));
  textarea_ptr->Digest();
  CHECK(textarea_ptr->value == "hellohello\nhello\nworld");
  CHECK(textarea_ptr->selection_start == 5);
  CHECK(textarea_ptr->cursor_pos == 10);
}

TEST_CASE("TextInputBase Delete Entire Line",
          "[component][input][delete_line]") {
  auto container = rtxui::Ref<TextareaTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);
  auto* textarea_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
  REQUIRE(textarea_el != nullptr);
  auto* textarea_ptr = ComponentAs<rtxui::textarea>(textarea_el);
  REQUIRE(textarea_ptr != nullptr);

  textarea_ptr->value = "line 1\nline 2\nline 3";
  textarea_el->set_focused(true);
  textarea_ptr->cursor_pos = 8;  // in line 2
  textarea_ptr->Digest();

  Event::Keyboard ctrl_shift_k;
  ctrl_shift_k.codepoint = 'k';
  ctrl_shift_k.modifier.ctrl = true;
  ctrl_shift_k.modifier.shift = true;

  textarea_ptr->OnEvent(Event(ctrl_shift_k));
  textarea_ptr->Digest();
  CHECK(textarea_ptr->value == "line 1\nline 3");

  // In single-line input: clears entire input
  auto input_cont = rtxui::Ref<InputTestComponent>::New();
  rtxui::ComponentInternals::Mount(*input_cont);
  auto* input_el =
      rtxui::ComponentInternals::Root(*input_cont)->QuerySelector("input");
  REQUIRE(input_el != nullptr);
  auto* input_ptr = ComponentAs<rtxui::input>(input_el);
  REQUIRE(input_ptr != nullptr);

  input_ptr->value = "clear me";
  input_el->set_focused(true);
  input_ptr->Digest();

  input_ptr->OnEvent(Event(ctrl_shift_k));
  input_ptr->Digest();
  CHECK(input_ptr->value == "");
  CHECK(input_ptr->cursor_pos == 0);
}

TEST_CASE("TextInputBase Auto-wrap Selection With Delimiters",
          "[component][input][wrap]") {
  auto container = rtxui::Ref<InputTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);
  auto* input_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("input");
  REQUIRE(input_el != nullptr);
  auto* input_ptr = ComponentAs<rtxui::input>(input_el);
  REQUIRE(input_ptr != nullptr);

  input_ptr->value = "hello world";
  input_ptr->selection_start = 6;
  input_ptr->cursor_pos = 11;  // "world"
  input_el->set_focused(true);
  input_ptr->Digest();

  // Wrap with ()
  input_ptr->OnEvent(Event::Keyboard::From('('));
  input_ptr->Digest();
  CHECK(input_ptr->value == "hello (world)");
  CHECK(input_ptr->selection_start == 7);
  CHECK(input_ptr->cursor_pos == 12);

  // Wrap again with []
  input_ptr->OnEvent(Event::Keyboard::From('['));
  input_ptr->Digest();
  CHECK(input_ptr->value == "hello ([world])");
  CHECK(input_ptr->selection_start == 8);
  CHECK(input_ptr->cursor_pos == 13);

  // Wrap again with quotes
  input_ptr->OnEvent(Event::Keyboard::From('"'));
  input_ptr->Digest();
  CHECK(input_ptr->value == "hello ([\"world\"])");

  // Undo un-wraps
  input_ptr->OnEvent(Event::CtrlZ());
  input_ptr->Digest();
  CHECK(input_ptr->value == "hello ([world])");
}

TEST_CASE("TextInputBase Select On Focus",
          "[component][input][select_on_focus]") {
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  auto container = rtxui::Ref<InputTestComponent>::New();
  rtxui::Screen screen = rtxui::ScreenInternals::Make(container, device);
  screen.Draw();

  auto* input_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("input");
  REQUIRE(input_el != nullptr);
  auto* input_ptr = ComponentAs<rtxui::input>(input_el);
  REQUIRE(input_ptr != nullptr);

  input_ptr->value = "quick test";
  input_ptr->select_on_focus = true;
  input_ptr->Digest();
  screen.Draw();

  CHECK(input_ptr->selection_start == -1);

  // Focus the input
  input_el->set_focused(true);
  input_ptr->Digest();
  screen.Draw();

  CHECK(input_ptr->selection_start == 0);
  CHECK(input_ptr->cursor_pos == 10);

  // Click on already-focused input does not re-select all
  int x_pos = input_el->absolute_x() + 1 + 2 + 1;
  Event::Mouse mouse;
  mouse.button = Event::Mouse::Button::Left;
  mouse.x = x_pos;
  mouse.y = input_el->absolute_y() + 1;
  mouse.motion = Event::Mouse::Motion::Pressed;
  screen.Dispatch(Event(mouse));
  mouse.motion = Event::Mouse::Motion::Released;
  screen.Dispatch(Event(mouse));
  screen.Draw();

  CHECK(input_ptr->selection_start == -1);
}
