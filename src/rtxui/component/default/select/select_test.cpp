// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/select/select.hpp"

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "rtxui/color.hpp"
#include "rtxui/component.hpp"
#include "rtxui/component/component_internal.hpp"
#include "rtxui/component/default/option/option.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/event.hpp"
#include "rtxui/headless.hpp"
#include "rtxui/layout/style.hpp"
#include "rtxui/refcounted.hpp"
#include "rtxui/screen.hpp"
#include "rtxui/terminal/event_internal.hpp"
#include "rtxui/terminal/screen_internal.hpp"
#include "rtxui/terminal/terminal_device.hpp"
#include "rtxui/testing/component_as.hpp"

using namespace rtxui;

namespace {

class SelectTestComponent : public rtxui::Component<SelectTestComponent> {
 public:
  std::string my_theme = "light";

  SelectTestComponent() { Bind(my_theme); }
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

  auto* select_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("select");
  REQUIRE(select_el != nullptr);
  auto* select_ptr = ComponentAs<rtxui::select>(select_el);
  REQUIRE(select_ptr != nullptr);

  // Check initial state
  std::cout << "DOM TREE:\n"
            << rtxui::ComponentInternals::Root(*container)->Print()
            << std::endl;
  std::cout << "select_el absolute_x=" << select_el->absolute_x()
            << " absolute_y=" << select_el->absolute_y()
            << " width=" << select_el->layout_width()
            << " height=" << select_el->layout_height() << std::endl;
  std::cout << "select_ptr->Root() absolute_x="
            << rtxui::ComponentInternals::Root(*select_ptr)->absolute_x()
            << " absolute_y="
            << rtxui::ComponentInternals::Root(*select_ptr)->absolute_y()
            << " width="
            << rtxui::ComponentInternals::Root(*select_ptr)->layout_width()
            << " height="
            << rtxui::ComponentInternals::Root(*select_ptr)->layout_height()
            << std::endl;
  CHECK(select_ptr->value == "light");
  CHECK(select_ptr->selected_label == "Light Theme");
  CHECK(select_ptr->is_open == false);

  // Click on the select element to open it
  select_el->set_focused(true);
  MouseData mouse_click;
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
  Event down_event =
      KeyboardData{.special = Event::Keyboard::Special::ArrowDown};
  CHECK(select_ptr->OnEvent(down_event) == true);
  container->Digest();
  screen.Draw();
  CHECK(select_ptr->hovered_index == 2);

  // Use keyboard: Enter to select "solarized"
  Event enter_event = KeyboardData{.special = Event::Keyboard::Special::Return};
  CHECK(select_ptr->OnEvent(enter_event) == true);
  container->Digest();
  screen.Draw();

  // Verify states after selection
  CHECK(select_ptr->is_open == false);
  CHECK(select_ptr->value == "solarized");
  CHECK(container->my_theme == "solarized");
  CHECK(select_ptr->selected_label == "Solarized");

  // Re-open the select to make the option elements visible and laid out
  MouseData reopen_click;
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
  auto* dark_option_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("option");
  REQUIRE(dark_option_el != nullptr);
  auto* dark_option_ptr = ComponentAs<rtxui::option>(dark_option_el);
  REQUIRE(dark_option_ptr != nullptr);
  CHECK(dark_option_ptr->value == "dark");

  MouseData option_mouse_click;
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

class SelectDisabledOptionTestComponent
    : public rtxui::Component<SelectDisabledOptionTestComponent> {
 public:
  std::string my_value = "a";

  SelectDisabledOptionTestComponent() { Bind(my_value); }
  std::string_view view = R"(
    <select value="{my_value}">
      <option value="a">A</option>
      <option id="opt-b" value="b" disabled="true">B</option>
      <option value="c">C</option>
    </select>
  )";
};

TEST_CASE(
    "Select skips a disabled <option> for click, keyboard select, and "
    "keyboard navigation",
    "[component][select][option][disabled][regression]") {
  auto container = rtxui::Ref<SelectDisabledOptionTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* select_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("select");
  auto* select_ptr = ComponentAs<rtxui::select>(select_el);
  REQUIRE(select_ptr != nullptr);

  // Open the dropdown by clicking the select button (hovered_index starts
  // on "a", index 0) - options are only actually laid out (and have a
  // meaningful absolute position) once the dropdown is open.
  select_el->set_focused(true);
  MouseData click_select;
  click_select.button = Event::Mouse::Button::Left;
  click_select.motion = Event::Mouse::Motion::Pressed;
  click_select.x = select_el->absolute_x() + 2;
  click_select.y = select_el->absolute_y() + 1;
  CHECK(select_ptr->OnEvent(Event(click_select)) == true);
  container->Digest();
  screen.Draw();
  REQUIRE(select_ptr->is_open);
  CHECK(select_ptr->hovered_index == 0);

  // Clicking the now-visible disabled option doesn't select it.
  auto* b_option_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("#opt-b");
  REQUIRE(b_option_el != nullptr);
  MouseData click_b;
  click_b.button = Event::Mouse::Button::Left;
  click_b.motion = Event::Mouse::Motion::Pressed;
  click_b.x = b_option_el->absolute_x() + 1;
  click_b.y = b_option_el->absolute_y() + 1;
  container->OnEvent(Event(click_b));
  container->Digest();
  CHECK(select_ptr->value == "a");

  // ArrowDown skips disabled "b" (index 1) and lands on "c" (index 2).
  select_ptr->OnEvent(
      KeyboardData{.special = Event::Keyboard::Special::ArrowDown});
  CHECK(select_ptr->hovered_index == 2);

  // ArrowDown again wraps around, skipping "b" again, back to "a" (index 0).
  select_ptr->OnEvent(
      KeyboardData{.special = Event::Keyboard::Special::ArrowDown});
  CHECK(select_ptr->hovered_index == 0);

  // Force-hover the disabled option directly and confirm Enter doesn't
  // select it (dropdown stays open, value unchanged).
  select_ptr->hovered_index = 1;
  select_ptr->OnEvent(
      KeyboardData{.special = Event::Keyboard::Special::Return});
  CHECK(select_ptr->value == "a");
  CHECK(select_ptr->is_open);

  // Closed-state ArrowDown also skips the disabled option.
  select_ptr->OnEvent(
      KeyboardData{.special = Event::Keyboard::Special::Escape});
  REQUIRE_FALSE(select_ptr->is_open);
  select_ptr->OnEvent(
      KeyboardData{.special = Event::Keyboard::Special::ArrowDown});
  CHECK(select_ptr->value == "c");
}

TEST_CASE(
    "Select Component Exposes Button/Dropdown Part Attributes For "
    "External ::part() Styling",
    "[component][select][part]") {
  auto container = rtxui::Ref<SelectTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* select_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("select");
  REQUIRE(select_el != nullptr);

  auto* btn_el = select_el->QuerySelector(".select-btn");
  REQUIRE(btn_el != nullptr);
  const std::string* btn_part = btn_el->GetAttribute("part");
  REQUIRE(btn_part != nullptr);
  CHECK(*btn_part == "select-btn");

  auto* label_el = select_el->QuerySelector(".select-label");
  REQUIRE(label_el != nullptr);
  const std::string* label_part = label_el->GetAttribute("part");
  REQUIRE(label_part != nullptr);
  CHECK(*label_part == "select-label");

  auto* arrow_el = select_el->QuerySelector(".select-arrow");
  REQUIRE(arrow_el != nullptr);
  const std::string* arrow_part = arrow_el->GetAttribute("part");
  REQUIRE(arrow_part != nullptr);
  CHECK(*arrow_part == "select-arrow");

  auto* dropdown_el = select_el->QuerySelector(".dropdown-list");
  REQUIRE(dropdown_el != nullptr);
  const std::string* dropdown_part = dropdown_el->GetAttribute("part");
  REQUIRE(dropdown_part != nullptr);
  CHECK(*dropdown_part == "dropdown-list");
}

TEST_CASE("Select Component Positioning and Mouse Hover Alignment",
          "[component][select][position][hover]") {
  auto container = rtxui::Ref<SelectTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* select_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("select");
  REQUIRE(select_el != nullptr);
  auto* select_ptr = ComponentAs<rtxui::select>(select_el);
  REQUIRE(select_ptr != nullptr);

  // Click on the select element to open it
  select_el->set_focused(true);
  MouseData mouse_click;
  mouse_click.button = Event::Mouse::Button::Left;
  mouse_click.motion = Event::Mouse::Motion::Pressed;
  mouse_click.x = select_el->absolute_x() + 2;
  mouse_click.y = select_el->absolute_y() + 1;
  Event click_event(mouse_click);
  CHECK(select_ptr->OnEvent(click_event) == true);
  container->Digest();
  screen.Draw();
  std::cout << "TEST INFO: select_el pointer=" << select_el
            << " rtxui::ComponentInternals::Root(*select_ptr)="
            << rtxui::ComponentInternals::Root(*select_ptr)
            << " select_el->classes=";
  for (const auto& c : select_el->classes()) {
    std::cout << c << " ";
  }
  std::cout << "\nTEST INFO: z_index value="
            << select_el->base_style.z_index.value_or(-99)
            << " has_val=" << select_el->base_style.z_index.has_value()
            << std::endl;
  CHECK(select_el->base_style.z_index.value_or(0) == 1000);

  // Verify the absolute coordinates of the dropdown-list
  auto* dropdown_el = rtxui::ComponentInternals::Root(*container)
                          ->QuerySelector(".dropdown-list");
  REQUIRE(dropdown_el != nullptr);
  CHECK(dropdown_el->absolute_x() == select_el->absolute_x());
  CHECK(dropdown_el->absolute_y() ==
        select_el->absolute_y() + select_el->layout_height());

  auto options = select_ptr->GetOptions();
  REQUIRE(options.size() == 3);
  auto* opt0 = options[0].element;
  auto* opt1 = options[1].element;

  // Initially "light" (opt1) is selected and hovered, and "dark" (opt0) is not.
  CHECK(opt1->base_style.background_color.value_or(Color()) ==
        Color::RGB(59, 130, 246));
  CHECK(!opt0->base_style.background_color.has_value());

  // Verify options are hoverable by moving mouse
  auto* dark_option_el =
      rtxui::ComponentInternals::Root(*container)
          ->QuerySelector("option");  // first option is "dark" at index 0
  REQUIRE(dark_option_el != nullptr);
  int dark_x = dark_option_el->absolute_x();
  int dark_y = dark_option_el->absolute_y();

  MouseData mouse_move;
  mouse_move.button = Event::Mouse::Button::None;
  mouse_move.motion = Event::Mouse::Motion::Moved;
  mouse_move.x = dark_x + 1;  // 1-indexed for screen
  mouse_move.y = dark_y + 1;  // 1-indexed for screen
  Event move_event(mouse_move);

  CHECK(select_ptr->OnEvent(move_event) == true);
  container->Digest();
  screen.Draw();

  // hovered_index should now be 0 ("dark")
  CHECK(select_ptr->hovered_index == 0);

  // Now "dark" (opt0) is hovered (blue) and "light" (opt1) is only selected
  // (slate)
  CHECK(opt0->base_style.background_color.value_or(Color()) ==
        Color::RGB(59, 130, 246));
  CHECK(opt1->base_style.background_color.value_or(Color()) ==
        Color::RGB(51, 65, 85));
}

class SelectZIndexTestComponent
    : public rtxui::Component<SelectZIndexTestComponent> {
 public:
  std::string my_theme = "light";

  SelectZIndexTestComponent() { Bind(my_theme); }
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

TEST_CASE("Select Component Z-index sibling overlap",
          "[component][select][z-index]") {
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  device->TriggerResize(40, 10);

  auto container = rtxui::Ref<SelectZIndexTestComponent>::New();
  rtxui::Screen screen = rtxui::ScreenInternals::Make(container, device);
  screen.Draw();

  auto* select_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("select");
  REQUIRE(select_el != nullptr);
  auto* select_ptr = ComponentAs<rtxui::select>(select_el);
  REQUIRE(select_ptr != nullptr);

  // Click on the select element to open it
  select_el->set_focused(true);
  MouseData mouse_click;
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
  // The first option in the dropdown is "Dark Theme", which should render at
  // absolute_y = 1 (covering "SIBLING_CONTENT_LINE_1" which also starts at
  // absolute_y = 1). Therefore, "Dark Theme" must be present in the output, and
  // "SIBLING_CONTENT_LINE_1" must NOT.
  CHECK(output.find("Dark Theme") != std::string::npos);
  CHECK(output.find("SIBLING_CONTENT_LINE_1") == std::string::npos);
}

class SelectPreservationTestComponent
    : public rtxui::Component<SelectPreservationTestComponent> {
 public:
  std::string theme = "light";
  SelectPreservationTestComponent() { Bind(theme); }
  std::string_view view = R"(
    <select value="{theme}">
      <option class="custom-class" value="dark">Dark Theme</option>
      <option class="custom-class" value="light">Light Theme</option>
    </select>
  )";
};

}  // namespace

TEST_CASE("Select Option Class Preservation", "[component][select][bug]") {
  auto container = rtxui::Ref<SelectPreservationTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* opt_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("option");
  REQUIRE(opt_el != nullptr);

  bool has_custom =
      std::find(opt_el->classes().begin(), opt_el->classes().end(),
                "custom-class") != opt_el->classes().end();
  CHECK(has_custom == true);

  container->theme = "dark";
  container->Digest();
  screen.Draw();

  has_custom = std::find(opt_el->classes().begin(), opt_el->classes().end(),
                         "custom-class") != opt_el->classes().end();
  CHECK(has_custom == true);
}

namespace {

// Two selects, one row apart: the first one's list, once open, covers the
// second.
class TwoSelects : public rtxui::Component<TwoSelects> {
 public:
  std::string a = "1";
  std::string b = "x";

  TwoSelects() {
    Bind(a);
    Bind(b);
  }
  std::string_view view = R"(
    <div><select value="{a}">
      <option value="1">one</option>
      <option value="2">two</option>
      <option value="3">three</option>
    </select></div>
    <div>gap</div>
    <div><select value="{b}">
      <option value="x">ex</option>
      <option value="y">why</option>
    </select></div>
    <div>end</div>
  )";
};

// Two selects side by side: neither list covers the other select.
class SideBySide : public rtxui::Component<SideBySide> {
 public:
  std::string_view view = R"(
    <div style="display: flex; gap: 2;">
      <select value="1"><option value="1">one</option></select>
      <select value="x"><option value="x">ex</option></select>
    </div>
  )";
};

std::string Line(const std::string& text, int row) {
  size_t start = 0;
  for (int i = 0; i < row; ++i) {
    start = text.find('\n', start) + 1;
  }
  return text.substr(start, text.find('\n', start) - start);
}

}  // namespace

TEST_CASE("Clicking another select while one is open opens it",
          "[component][select][regression]") {
  // The open select closed on the outside click and consumed it, so the
  // select clicked never saw it: it took a second click to open.
  auto app = rtxui::Ref<SideBySide>::New();
  rtxui::TimelineScreen screen(app, 40, 4);
  screen.Click(1, 0);  // Opens the first.
  REQUIRE(Line(screen.Text(), 0) == " one         ▴    ex          ▾");

  screen.Click(18, 0);  // The second.
  CHECK(Line(screen.Text(), 0) == " one         ▾    ex          ▴");
}

TEST_CASE("A closed select's options ignore clicks",
          "[component][select][regression]") {
  // Hidden options kept their last position, so a click where one had been
  // drawn picked it, with the list closed.
  auto app = rtxui::Ref<TwoSelects>::New();
  rtxui::TimelineScreen screen(app, 20, 8);
  screen.Click(1, 0);  // Opens.
  screen.Click(1, 0);  // Closes.
  screen.Click(1, 3);  // Where "three" was.
  CHECK(app->a == "1");
}

TEST_CASE("An open select's list hides what it covers",
          "[component][select][regression]") {
  // The list was a translucent tint over the page, so the text under it --
  // here the second select's arrow -- showed through.
  auto app = rtxui::Ref<TwoSelects>::New();
  rtxui::TimelineScreen screen(app, 20, 8);
  screen.Click(1, 0);
  CHECK(Line(screen.Text(), 2).find("▾") == std::string::npos);
  CHECK(screen.BackgroundAt(1, 2).a == 255);
}

namespace {

class LongSelect : public rtxui::Component<LongSelect> {
 public:
  std::string_view view = R"(
    <select value="a">
      <option value="a">a</option><option value="b">b</option>
      <option value="c">c</option><option value="d">d</option>
      <option value="e">e</option><option value="f">f</option>
      <option value="g">g</option><option value="h">h</option>
    </select>
    <style>
      select::part(dropdown-list) { max-height: 3; overflow-y: scroll; }
    </style>
  )";
};

}  // namespace

TEST_CASE("A select's list scrolls to the option the arrows reach",
          "[component][select][regression]") {
  // A list taller than its max-height scrolled with the wheel only: the
  // highlight moved on past its bottom edge, out of sight.
  auto app = rtxui::Ref<LongSelect>::New();
  rtxui::TimelineScreen screen(app, 20, 6);
  screen.Input("\t\r");  // Focus and open.
  for (int i = 0; i < 6; ++i) {
    screen.Input("\x1b[B");  // Down, to "g".
  }
  const std::string text = screen.Text();
  CHECK(Line(text, 1).find('e') != std::string::npos);
  CHECK(Line(text, 3).find('g') != std::string::npos);
}
