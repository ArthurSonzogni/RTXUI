// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/textarea/textarea.hpp"

#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "rtxui/color.hpp"
#include "rtxui/component.hpp"
#include "rtxui/component/component_internal.hpp"
#include "rtxui/component/default/input/text_input_base.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/event.hpp"
#include "rtxui/layout/layout.hpp"
#include "rtxui/layout/layout_tree_builder.hpp"
#include "rtxui/layout/style.hpp"
#include "rtxui/paint/paint.hpp"
#include "rtxui/paint/texture.hpp"
#include "rtxui/refcounted.hpp"
#include "rtxui/screen.hpp"
#include "rtxui/terminal/screen_internal.hpp"
#include "rtxui/terminal/terminal_device.hpp"
#include "rtxui/testing/component_as.hpp"

using namespace rtxui;

namespace {

class TextareaTestComponent : public rtxui::Component<TextareaTestComponent> {
 public:
  std::string my_text = "line one\nline two\nline three";
  TextareaTestComponent() { Bind(my_text); }
  std::string_view view = R"(
    <textarea value="{my_text}" />
  )";
};

TEST_CASE("Textarea Click Does Not Resize", "[component][textarea]") {
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  auto container = rtxui::Ref<TextareaTestComponent>::New();
  rtxui::Screen screen = rtxui::ScreenInternals::Make(container, device);
  screen.Draw();

  auto* ta_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
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

TEST_CASE("Textarea Click While Scrolled Targets The Visible Line",
          "[component][textarea]") {
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  auto container = rtxui::Ref<TextareaTestComponent>::New();

  // 20 short lines ("L0".."L19"), each one row tall -- taller than the
  // textarea's default height (5), so it scrolls vertically.
  std::string content;
  int expected_pos_of_line_15 = 0;
  for (int i = 0; i < 20; ++i) {
    if (i == 15) {
      expected_pos_of_line_15 = static_cast<int>(content.size());
    }
    content += "L" + std::to_string(i);
    if (i != 19) {
      content += "\n";
    }
  }
  container->my_text = content;

  rtxui::Screen screen = rtxui::ScreenInternals::Make(container, device);
  screen.Draw();

  auto* ta_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
  REQUIRE(ta_el != nullptr);
  auto* ta_ptr = ComponentAs<rtxui::textarea>(ta_el);
  REQUIRE(ta_ptr != nullptr);

  // Move the cursor to the end to scroll the view down so line 15 ("L15")
  // is the topmost visible row (scroll_y == 15, viewport height == 5).
  ta_ptr->cursor_pos = static_cast<int>(ta_ptr->value.size());
  ta_ptr->selection_start = -1;
  ta_ptr->Digest();
  screen.Draw();
  REQUIRE(ta_el->scroll_y() == 15);

  // Click the top-left cell of the (scrolled) content area -- this used to
  // reset scroll_y to 0 as a side effect of focusing the clicked element
  // (Screen::ScrollIntoView(), called on every mouse-click focus change,
  // computes the element's position from its fragment's unscrolled
  // content-space bounding box; the textarea's whole multi-line content is
  // one fragment taller than the viewport, so it always resolved to that
  // fragment's top edge and scrolled back up to it), so the click landed on
  // "L0" instead of the visually-clicked "L15".
  Event::Mouse mouse;
  mouse.button = Event::Mouse::Button::Left;
  mouse.motion = Event::Mouse::Motion::Pressed;
  mouse.x = ta_el->absolute_x() + 2;  // inside padding-left:1
  mouse.y = ta_el->absolute_y() + 1;  // top row, 1-based
  screen.Dispatch(Event(mouse));

  CHECK(ta_el->scroll_y() == 15);
  CHECK(ta_ptr->cursor_pos == expected_pos_of_line_15);
}

class GutterTextareaTestComponent
    : public rtxui::Component<GutterTextareaTestComponent> {
 public:
  std::string my_text = "";
  GutterTextareaTestComponent() { Bind(my_text); }
  std::string_view view = R"(
    <textarea value="{my_text}" linenumbers="true" />
  )";
};

TEST_CASE("Textarea Click Accounts For The Line-Number Gutter's Width",
          "[component][textarea][regression]") {
  // linenumbers must be set as a template attribute (not just the C++
  // member) for `self[linenumbers]` in textarea's CSS to switch self into
  // the flex row that actually places the gutter beside the content -- see
  // GutterTextareaTestComponent's view.
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  auto container = rtxui::Ref<GutterTextareaTestComponent>::New();
  container->my_text = "abcdefgh\nijklmnop";

  rtxui::Screen screen = rtxui::ScreenInternals::Make(container, device);
  screen.Draw();

  auto* ta_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
  REQUIRE(ta_el != nullptr);
  auto* ta_ptr = ComponentAs<rtxui::textarea>(ta_el);
  REQUIRE(ta_ptr != nullptr);
  REQUIRE(ta_ptr->show_gutter);

  auto* content_el = ta_el->QuerySelector(".content");
  REQUIRE(content_el != nullptr);
  // The gutter must have actually claimed some of self's width -- otherwise
  // this test can't tell a gutter-aware click from a gutter-oblivious one.
  REQUIRE(content_el->absolute_x() > ta_el->absolute_x());

  // Click on the second character of line 0 ('b'), which sits at column 1
  // inside the content area (i.e. just past the gutter).
  Event::Mouse mouse;
  mouse.button = Event::Mouse::Button::Left;
  mouse.motion = Event::Mouse::Motion::Pressed;
  mouse.x =
      content_el->absolute_x() + 1 + 1;  // +1 for column 1, +1 for 1-based
  mouse.y = content_el->absolute_y() + 1;
  screen.Dispatch(Event(mouse));

  CHECK(ta_ptr->cursor_pos == 1);
  CHECK(ta_ptr->value[ta_ptr->cursor_pos] == 'b');
}

TEST_CASE("Textarea Scroll And Click Account For Wrapped (Multi-Row) Lines",
          "[component][textarea][regression]") {
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  auto container = rtxui::Ref<TextareaTestComponent>::New();

  // Each of these 6 logical lines is 80 unbroken characters, much longer
  // than the textarea's content width, so white-space:pre-wrap wraps each
  // one into multiple rows. Before this fix, KeepCursorVisible/OnEventShared
  // treated 1 logical line as 1 row, so scroll_y (and click targeting) were
  // computed in the wrong units entirely once any line wrapped.
  std::string line(80, 'a');
  std::string content;
  for (int i = 0; i < 6; ++i) {
    for (auto& c : line) {
      c = static_cast<char>('a' + i);
    }
    content += line;
    if (i != 5) {
      content += "\n";
    }
  }
  container->my_text = content;

  rtxui::Screen screen = rtxui::ScreenInternals::Make(container, device);
  screen.Draw();

  auto* ta_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
  REQUIRE(ta_el != nullptr);
  auto* ta_ptr = ComponentAs<rtxui::textarea>(ta_el);
  REQUIRE(ta_ptr != nullptr);

  auto* content_el = ta_el->QuerySelector(".content");
  REQUIRE(content_el != nullptr);

  // Move the cursor to the very end (the last character of the last row of
  // the last line, i.e. visual row 17) to force a scroll down.
  ta_ptr->cursor_pos = static_cast<int>(ta_ptr->value.size());
  ta_ptr->selection_start = -1;
  ta_ptr->Digest();
  screen.Draw();

  // self is 40 cells wide (default) with 1 cell of padding on each side and
  // 1 reserved for the scrollbar (overflow-y: scroll) -- 37 cells of actual
  // content width. Each 80-character line therefore wraps into ceil(80/37)
  // = 3 rows, for 18 rows of content total, versus the textarea's default
  // height of 5 -- both asserted here so this test breaks loudly (instead
  // of silently passing on a no-op) if either default ever changes.
  REQUIRE(content_el->layout_width() == 37);
  REQUIRE(content_el->layout_height() == 18);
  REQUIRE(ta_el->layout_height() == 5);

  // Row-based scrolling: scroll_y must reveal row 17 at the bottom of the
  // 5-row viewport, i.e. 17 - 5 + 1 = 13. The pre-fix logical-line-based
  // code computed scroll_y = 1 here instead (5 logical newlines - 5 rows of
  // height + 1), leaving the cursor's actual row far out of view.
  CHECK(ta_el->scroll_y() == 13);

  // Click the top-left visible cell -- an actual on-screen coordinate, so
  // relative to self (ta_el), not to content_el (whose absolute position
  // reflects the *unscrolled* content and is therefore off-screen, above
  // the viewport, once scrolled down). With scroll_y == 13, that cell is
  // row 13, which is the second wrapped row of line 4 ('e' * 80): row 12
  // covers [324, 361), so row 13 starts at grapheme index 361.
  Event::Mouse mouse;
  mouse.button = Event::Mouse::Button::Left;
  mouse.motion = Event::Mouse::Motion::Pressed;
  mouse.x = ta_el->absolute_x() + 2;  // 1-based, inside padding-left:1
  mouse.y = ta_el->absolute_y() + 1;  // 1-based, top visible row
  screen.Dispatch(Event(mouse));

  CHECK(ta_ptr->cursor_pos == 361);
  CHECK(ta_ptr->value[ta_ptr->cursor_pos] == 'e');
}

TEST_CASE("Textarea Click Maps Correctly On A Word-Wrapped Row",
          "[component][textarea][regression]") {
  // Exercises the word-wrap (break-at-last-space) branch of the wrap
  // algorithm, as opposed to the character-emergency-break branch the
  // repeated-letter-line test above exercises.
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  auto container = rtxui::Ref<TextareaTestComponent>::New();

  // "w0 w1 w2 ... w59": with the 37-cell content width established above,
  // this wraps after "w11 " (37 characters, computed independently in
  // Python against a reference greedy word-wrap: break at the last space
  // when the next word doesn't fit), so row 1 starts at grapheme index 38
  // with "w12 w13 w14 w15 ...". 60 words (7 rows total) is enough to make
  // the textarea's default 5-row viewport actually scrollable, unlike a
  // shorter value where scroll_y would just clamp back to 0.
  std::string content;
  for (int i = 0; i < 60; ++i) {
    if (i != 0) {
      content += " ";
    }
    content += "w" + std::to_string(i);
  }
  container->my_text = content;

  rtxui::Screen screen = rtxui::ScreenInternals::Make(container, device);
  screen.Draw();

  auto* ta_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
  REQUIRE(ta_el != nullptr);
  auto* ta_ptr = ComponentAs<rtxui::textarea>(ta_el);
  REQUIRE(ta_ptr != nullptr);
  REQUIRE(ta_ptr->value.substr(38, 3) == "w12");
  REQUIRE(ta_ptr->value.substr(50, 3) == "w15");

  auto* content_el = ta_el->QuerySelector(".content");
  REQUIRE(content_el != nullptr);

  // Scroll row 1 into view directly (bypassing KeepCursorVisible, which is
  // covered by the test above) to isolate the click-to-position mapping.
  ta_el->set_scroll_y(1);
  screen.Draw();
  REQUIRE(ta_el->scroll_y() == 1);

  // Click at actual on-screen coordinates (relative to self, not to
  // content_el -- see the comment in the test above). Column 0 of the (now
  // topmost) row 1 is the "w" of "w12".
  Event::Mouse mouse;
  mouse.button = Event::Mouse::Button::Left;
  mouse.motion = Event::Mouse::Motion::Pressed;
  mouse.x = ta_el->absolute_x() + 2;  // 1-based, inside padding-left:1
  mouse.y = ta_el->absolute_y() + 1;  // 1-based, top visible row
  screen.Dispatch(Event(mouse));
  CHECK(ta_ptr->cursor_pos == 38);

  // Column 12 of that same row (still row 1, since it hasn't wrapped a
  // second time) is the "w" of "w15".
  mouse.x = ta_el->absolute_x() + 2 + 12;
  screen.Dispatch(Event(mouse));
  CHECK(ta_ptr->cursor_pos == 50);
}

class MultiClickTextareaTestComponent
    : public rtxui::Component<MultiClickTextareaTestComponent> {
 public:
  std::string my_text =
      "First line of p1\n"
      "Second line of p1\n"
      "\n"
      "First line of p2\n"
      "Second line of p2";

  MultiClickTextareaTestComponent() { Bind(my_text); }
  std::string_view view = R"(
    <textarea value="{my_text}" />
  )";
};

TEST_CASE("Textarea Multi-Click Line and Paragraph Selection",
          "[component][textarea]") {
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  auto container = rtxui::Ref<MultiClickTextareaTestComponent>::New();
  rtxui::Screen screen = rtxui::ScreenInternals::Make(container, device);
  screen.Draw();

  auto* ta_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
  REQUIRE(ta_el != nullptr);
  auto* ta_ptr = ComponentAs<rtxui::textarea>(ta_el);
  REQUIRE(ta_ptr != nullptr);

  auto* content_el = ta_el->QuerySelector(".content");
  REQUIRE(content_el != nullptr);

  SECTION("Triple Click Selects Current Line") {
    Event::Mouse mouse;
    mouse.button = Event::Mouse::Button::Left;
    mouse.x = content_el->absolute_x() + 1 + 2;
    mouse.y = content_el->absolute_y() + 1 + 1;

    for (int i = 0; i < 2; ++i) {
      mouse.motion = Event::Mouse::Motion::Pressed;
      screen.Dispatch(Event(mouse));
      mouse.motion = Event::Mouse::Motion::Released;
      screen.Dispatch(Event(mouse));
    }

    mouse.motion = Event::Mouse::Motion::Pressed;
    screen.Dispatch(Event(mouse));
    ta_ptr->Digest();

    CHECK(ta_ptr->selection_start == 17);
    CHECK(ta_ptr->cursor_pos == 34);

    mouse.motion = Event::Mouse::Motion::Released;
    screen.Dispatch(Event(mouse));
  }

  SECTION("Quadruple Click Selects Entire Paragraph") {
    Event::Mouse mouse;
    mouse.button = Event::Mouse::Button::Left;
    mouse.x = content_el->absolute_x() + 1 + 2;
    mouse.y = content_el->absolute_y() + 1 + 1;

    for (int i = 0; i < 3; ++i) {
      mouse.motion = Event::Mouse::Motion::Pressed;
      screen.Dispatch(Event(mouse));
      mouse.motion = Event::Mouse::Motion::Released;
      screen.Dispatch(Event(mouse));
    }

    mouse.motion = Event::Mouse::Motion::Pressed;
    screen.Dispatch(Event(mouse));
    ta_ptr->Digest();

    CHECK(ta_ptr->selection_start == 0);
    CHECK(ta_ptr->cursor_pos == 34);

    mouse.motion = Event::Mouse::Motion::Released;
    screen.Dispatch(Event(mouse));
  }

  SECTION("Quadruple Click on Paragraph 2") {
    Event::Mouse mouse;
    mouse.button = Event::Mouse::Button::Left;
    mouse.x = content_el->absolute_x() + 1 + 2;
    mouse.y = content_el->absolute_y() + 1 + 3;

    for (int i = 0; i < 3; ++i) {
      mouse.motion = Event::Mouse::Motion::Pressed;
      screen.Dispatch(Event(mouse));
      mouse.motion = Event::Mouse::Motion::Released;
      screen.Dispatch(Event(mouse));
    }

    mouse.motion = Event::Mouse::Motion::Pressed;
    screen.Dispatch(Event(mouse));
    ta_ptr->Digest();

    CHECK(ta_ptr->selection_start == 36);
    CHECK(ta_ptr->cursor_pos == 70);

    mouse.motion = Event::Mouse::Motion::Released;
    screen.Dispatch(Event(mouse));
  }

  SECTION("Triple Click Drag Selects By Line") {
    Event::Mouse mouse;
    mouse.button = Event::Mouse::Button::Left;
    mouse.x = content_el->absolute_x() + 1 + 2;
    mouse.y = content_el->absolute_y() + 1 + 0;

    for (int i = 0; i < 2; ++i) {
      mouse.motion = Event::Mouse::Motion::Pressed;
      screen.Dispatch(Event(mouse));
      mouse.motion = Event::Mouse::Motion::Released;
      screen.Dispatch(Event(mouse));
    }

    mouse.motion = Event::Mouse::Motion::Pressed;
    screen.Dispatch(Event(mouse));
    ta_ptr->Digest();
    CHECK(ta_ptr->selection_start == 0);
    CHECK(ta_ptr->cursor_pos == 16);

    mouse.motion = Event::Mouse::Motion::Moved;
    mouse.y = content_el->absolute_y() + 1 + 1;
    screen.Dispatch(Event(mouse));
    ta_ptr->Digest();

    CHECK(ta_ptr->selection_start == 0);
    CHECK(ta_ptr->cursor_pos == 34);

    mouse.motion = Event::Mouse::Motion::Released;
    screen.Dispatch(Event(mouse));
  }
}

TEST_CASE(
    "Textarea click mapping doesn't lose width when a wrapped word's "
    "overflow is detected several characters past the last space",
    "[component][textarea][regression]") {
  // Regression: text_input_base.cpp's ComputeRowStarts mirrors
  // layout.cpp's word-wrap algorithm for mouse click-to-cursor mapping,
  // and had the same stale-last-space bug (see the layout.cpp fix earlier
  // this session): breaking at a last space several characters before the
  // overflow point reset cur_col back to col_start, discarding the
  // already-scanned characters' width. That undercounted how much of the
  // next (very long) word fit per row, so every row boundary after the
  // first wrap point drifted - a click that should land on grapheme index
  // 67 landed on 74 instead (7 characters lost, matching the 7 unbreakable
  // characters that fit on the first row before it overflowed).
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  auto container = rtxui::Ref<TextareaTestComponent>::New();

  // 10 short words (30 chars incl. trailing space) followed by a single
  // 50-character unbreakable run. With the 37-cell content width
  // established by the tests above: row0 = "w0 w1 ... w9" (29 chars,
  // trailing space dropped), row1 = the first 37 x's (indices 30-66,
  // pushed whole past the last space since the word doesn't fit after
  // "w9 "), row2 = the remaining 13 x's starting at index 67.
  std::string content = "w0 w1 w2 w3 w4 w5 w6 w7 w8 w9 ";  // 30 chars
  content += std::string(50, 'x');
  container->my_text = content;

  rtxui::Screen screen = rtxui::ScreenInternals::Make(container, device);
  screen.Draw();

  auto* ta_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
  REQUIRE(ta_el != nullptr);
  auto* ta_ptr = ComponentAs<rtxui::textarea>(ta_el);
  REQUIRE(ta_ptr != nullptr);
  auto* content_el = ta_el->QuerySelector(".content");
  REQUIRE(content_el != nullptr);
  REQUIRE(content_el->layout_width() == 37);

  Event::Mouse mouse;
  mouse.button = Event::Mouse::Button::Left;
  mouse.motion = Event::Mouse::Motion::Pressed;
  mouse.x = ta_el->absolute_x() + 2;      // 1-based, inside padding-left:1
  mouse.y = ta_el->absolute_y() + 1 + 2;  // 1-based, row index 2 (0-based)
  screen.Dispatch(Event(mouse));

  CHECK(ta_ptr->cursor_pos == 67);
}

TEST_CASE("Textarea ComputeRowStarts correctly wraps past the second line",
          "[component][textarea][regression]") {
  // Regression: text_input_base.cpp's ComputeRowStarts assigned last_space_col
  // + 1 into col_start instead of advancing col_start += last_space_col + 1.
  // When col_start was 0 (first wrap), assignment and addition produced the
  // same result, so the first two lines wrapped correctly. From the third line
  // onwards, col_start reset to a small relative offset instead of advancing
  // monotonically, causing premature line breaks and wrong click mapping.
  class NarrowTextareaComponent
      : public rtxui::Component<NarrowTextareaComponent> {
   public:
    std::string text =
        "word01 word02 word03 word04 word05 word06 word07 word08";
    NarrowTextareaComponent() { Bind(text); }
    std::string_view view = R"(
      <textarea value="{text}" style="width: 17; padding: 0 1;" />
    )";
  };

  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  auto container = rtxui::Ref<NarrowTextareaComponent>::New();
  rtxui::Screen screen = rtxui::ScreenInternals::Make(container, device);
  screen.Draw();

  auto* ta_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
  REQUIRE(ta_el != nullptr);
  auto* ta_ptr = ComponentAs<rtxui::textarea>(ta_el);
  REQUIRE(ta_ptr != nullptr);
  auto* content_el = ta_el->QuerySelector(".content");
  REQUIRE(content_el != nullptr);
  REQUIRE(content_el->layout_width() == 14);

  // Row 0: "word01 word02 " (indices 0..13)
  // Row 1: "word03 word04 " (indices 14..27)
  // Row 2: "word05 word06 " (indices 28..41)
  // Row 3: "word07 word08"  (indices 42..55)
  Event::Mouse mouse;
  mouse.button = Event::Mouse::Button::Left;
  mouse.motion = Event::Mouse::Motion::Pressed;
  mouse.x = content_el->absolute_x() + 1;
  mouse.y = content_el->absolute_y() + 1 + 2;  // row 2
  screen.Dispatch(Event(mouse));

  CHECK(ta_ptr->cursor_pos == 28);

  // Click on Row 3, col 0 ("word07"):
  mouse.y = content_el->absolute_y() + 1 + 3;  // row 3
  screen.Dispatch(Event(mouse));

  CHECK(ta_ptr->cursor_pos == 42);
}

TEST_CASE("Textarea Component Readonly Blocks Enter And Tab Indent",
          "[component][textarea][readonly]") {
  auto container = rtxui::Ref<TextareaTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* textarea_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
  REQUIRE(textarea_el != nullptr);
  auto* textarea_ptr = ComponentAs<rtxui::textarea>(textarea_el);
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
  rtxui::ComponentInternals::Mount(*container);

  auto* textarea_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
  REQUIRE(textarea_el != nullptr);
  auto* textarea_ptr = ComponentAs<rtxui::textarea>(textarea_el);
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

TEST_CASE("Textarea Component Placeholder Supports Multiple Lines",
          "[component][textarea][placeholder]") {
  auto container = rtxui::Ref<TextareaTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* textarea_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
  REQUIRE(textarea_el != nullptr);
  auto* textarea_ptr = ComponentAs<rtxui::textarea>(textarea_el);
  REQUIRE(textarea_ptr != nullptr);

  textarea_ptr->placeholder = "Line one\nLine two";
  textarea_ptr->value = "";
  textarea_ptr->Digest();
  CHECK(textarea_ptr->placeholder_text == "Line one\nLine two");

  textarea_ptr->value = "not empty";
  textarea_ptr->Digest();
  CHECK(textarea_ptr->placeholder_text == "");
}

TEST_CASE("Textarea Component Maxlength Blocks Enter And Tab Past The Limit",
          "[component][textarea][maxlength]") {
  auto container = rtxui::Ref<TextareaTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* textarea_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
  REQUIRE(textarea_el != nullptr);
  auto* textarea_ptr = ComponentAs<rtxui::textarea>(textarea_el);
  REQUIRE(textarea_ptr != nullptr);

  textarea_ptr->value = "ab";
  textarea_ptr->cursor_pos = 2;
  textarea_ptr->maxlength = 2;
  textarea_el->set_focused(true);
  textarea_ptr->Digest();

  Event::Keyboard tab_kb;
  tab_kb.special = Event::Keyboard::Special::Tab;
  textarea_ptr->OnEvent(Event(tab_kb));
  textarea_ptr->Digest();
  CHECK(textarea_ptr->value == "ab");  // no room for the tab character

  textarea_ptr->OnEvent(Event::Return());
  textarea_ptr->Digest();
  CHECK(textarea_ptr->value == "ab");  // no room for the newline either
}

TEST_CASE("Textarea Component Line Numbers Off By Default",
          "[component][textarea][linenumbers]") {
  auto container = rtxui::Ref<TextareaTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* textarea_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
  REQUIRE(textarea_el != nullptr);
  auto* textarea_ptr = ComponentAs<rtxui::textarea>(textarea_el);
  REQUIRE(textarea_ptr != nullptr);

  textarea_ptr->Digest();
  CHECK_FALSE(textarea_ptr->show_gutter);
  CHECK(textarea_ptr->gutter_lines.empty());
}

TEST_CASE("Textarea Component Line Numbers Absolute Mode",
          "[component][textarea][linenumbers]") {
  auto container = rtxui::Ref<TextareaTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* textarea_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
  REQUIRE(textarea_el != nullptr);
  auto* textarea_ptr = ComponentAs<rtxui::textarea>(textarea_el);
  REQUIRE(textarea_ptr != nullptr);

  // value == "line one\nline two\nline three"; cursor_pos 10 lands inside
  // "line two" (the second logical line).
  textarea_ptr->linenumbers = "true";
  textarea_ptr->cursor_pos = 10;
  textarea_ptr->Digest();

  REQUIRE(textarea_ptr->show_gutter);
  REQUIRE(textarea_ptr->gutter_lines.size() == 3);
  CHECK(textarea_ptr->gutter_lines[0].text == "1");
  CHECK(textarea_ptr->gutter_lines[1].text == "2");
  CHECK(textarea_ptr->gutter_lines[2].text == "3");
  CHECK(textarea_ptr->gutter_lines[0].css_class == "line-number");
  CHECK(textarea_ptr->gutter_lines[1].css_class == "line-number active");
  CHECK(textarea_ptr->gutter_lines[2].css_class == "line-number");
  CHECK(textarea_ptr->gutter_width == 1);
}

TEST_CASE("Textarea Component Line Numbers Relative Mode",
          "[component][textarea][linenumbers]") {
  auto container = rtxui::Ref<TextareaTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* textarea_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
  REQUIRE(textarea_el != nullptr);
  auto* textarea_ptr = ComponentAs<rtxui::textarea>(textarea_el);
  REQUIRE(textarea_ptr != nullptr);

  // Active line (index 1) shows its absolute number; the others show their
  // distance from it.
  textarea_ptr->linenumbers = "relative";
  textarea_ptr->cursor_pos = 10;
  textarea_ptr->Digest();

  REQUIRE(textarea_ptr->gutter_lines.size() == 3);
  CHECK(textarea_ptr->gutter_lines[0].text == "1");
  CHECK(textarea_ptr->gutter_lines[1].text == "2");
  CHECK(textarea_ptr->gutter_lines[2].text == "1");
}

TEST_CASE("Textarea Component Line Numbers Line Start Offset",
          "[component][textarea][linenumbers]") {
  auto container = rtxui::Ref<TextareaTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* textarea_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
  REQUIRE(textarea_el != nullptr);
  auto* textarea_ptr = ComponentAs<rtxui::textarea>(textarea_el);
  REQUIRE(textarea_ptr != nullptr);

  textarea_ptr->linenumbers = "true";
  textarea_ptr->line_start = 41;
  textarea_ptr->Digest();

  REQUIRE(textarea_ptr->gutter_lines.size() == 3);
  CHECK(textarea_ptr->gutter_lines[0].text == "41");
  CHECK(textarea_ptr->gutter_lines[1].text == "42");
  CHECK(textarea_ptr->gutter_lines[2].text == "43");
  CHECK(textarea_ptr->gutter_width == 2);
}

TEST_CASE("Textarea Component Line Numbers Line End Blanks Beyond Range",
          "[component][textarea][linenumbers]") {
  auto container = rtxui::Ref<TextareaTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* textarea_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
  REQUIRE(textarea_el != nullptr);
  auto* textarea_ptr = ComponentAs<rtxui::textarea>(textarea_el);
  REQUIRE(textarea_ptr != nullptr);

  textarea_ptr->linenumbers = "true";
  textarea_ptr->line_end = 2;
  textarea_ptr->Digest();

  REQUIRE(textarea_ptr->gutter_lines.size() == 3);
  CHECK(textarea_ptr->gutter_lines[0].text == "1");
  CHECK(textarea_ptr->gutter_lines[1].text == "2");
  // Line 3 is past line_end: blank gutter cell (whitespace only).
  CHECK(textarea_ptr->gutter_lines[2].text.find_first_not_of(' ') ==
        std::string::npos);
}

TEST_CASE("Textarea Component Highlight Current Line Off By Default",
          "[component][textarea][highlight_current_line]") {
  auto container = rtxui::Ref<TextareaTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* textarea_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
  REQUIRE(textarea_el != nullptr);
  auto* textarea_ptr = ComponentAs<rtxui::textarea>(textarea_el);
  REQUIRE(textarea_ptr != nullptr);

  textarea_ptr->Digest();
  CHECK(textarea_ptr->content_line_highlights.empty());
}

TEST_CASE("Textarea Component Highlight Current Line Marks The Cursor's Line",
          "[component][textarea][highlight_current_line]") {
  auto container = rtxui::Ref<TextareaTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* textarea_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
  REQUIRE(textarea_el != nullptr);
  auto* textarea_ptr = ComponentAs<rtxui::textarea>(textarea_el);
  REQUIRE(textarea_ptr != nullptr);

  // value == "line one\nline two\nline three"; cursor_pos 10 lands inside
  // "line two" (the second logical line), independent of linenumbers.
  textarea_ptr->highlight_current_line = true;
  textarea_ptr->cursor_pos = 10;
  textarea_ptr->Digest();

  REQUIRE(textarea_ptr->content_line_highlights.size() == 3);
  CHECK(textarea_ptr->content_line_highlights[0].css_class == "line");
  CHECK(textarea_ptr->content_line_highlights[1].css_class ==
        "line current-line");
  CHECK(textarea_ptr->content_line_highlights[2].css_class == "line");

  // Moving the cursor to line 0 moves the highlight with it.
  textarea_ptr->cursor_pos = 0;
  textarea_ptr->Digest();
  CHECK(textarea_ptr->content_line_highlights[0].css_class ==
        "line current-line");
  CHECK(textarea_ptr->content_line_highlights[1].css_class == "line");
}

TEST_CASE("Textarea Component Highlight Current Line Works With Line Numbers",
          "[component][textarea][highlight_current_line][linenumbers]") {
  auto container = rtxui::Ref<TextareaTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* textarea_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
  REQUIRE(textarea_el != nullptr);
  auto* textarea_ptr = ComponentAs<rtxui::textarea>(textarea_el);
  REQUIRE(textarea_ptr != nullptr);

  textarea_ptr->linenumbers = "true";
  textarea_ptr->highlight_current_line = true;
  textarea_ptr->cursor_pos = 10;
  textarea_ptr->Digest();

  REQUIRE(textarea_ptr->show_gutter);
  REQUIRE(textarea_ptr->gutter_lines.size() == 3);
  REQUIRE(textarea_ptr->content_line_highlights.size() == 3);
  CHECK(textarea_ptr->gutter_lines[1].css_class == "line-number active");
  CHECK(textarea_ptr->content_line_highlights[1].css_class ==
        "line current-line");
}

TEST_CASE(
    "Textarea Component Highlight Current Line Stays Aligned When Lines Wrap",
    "[component][textarea][highlight_current_line][wrap]") {
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  auto container = rtxui::Ref<TextareaTestComponent>::New();

  // "line 1 " followed by 35 'a's wraps across 2 rows in a 37-cell content box.
  // Then "\nline 2" is the second logical line (third visual row).
  std::string text = "line 1 " + std::string(35, 'a') + "\nline 2";
  container->my_text = text;

  rtxui::Screen screen = rtxui::ScreenInternals::Make(container, device);
  screen.Draw();

  auto* textarea_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
  REQUIRE(textarea_el != nullptr);
  auto* textarea_ptr = ComponentAs<rtxui::textarea>(textarea_el);
  REQUIRE(textarea_ptr != nullptr);

  textarea_ptr->linenumbers = "true";
  textarea_ptr->line_wrap = "subline";
  textarea_ptr->highlight_current_line = true;
  // Place cursor in "line 2"
  textarea_ptr->cursor_pos = static_cast<int>(text.find("line 2"));
  textarea_ptr->Digest();

  REQUIRE(textarea_ptr->content_line_highlights.size() == 3);
  // Row 0 and 1 belong to logical line 1 (the first row and its wrapped
  // continuation).
  CHECK(textarea_ptr->content_line_highlights[0].css_class == "line");
  CHECK(textarea_ptr->content_line_highlights[1].css_class == "line");
  // Row 2 is the active row containing "line 2".
  CHECK(textarea_ptr->content_line_highlights[2].css_class ==
        "line current-line");

  // Gutter lines should also align across all 3 visual rows.
  REQUIRE(textarea_ptr->gutter_lines.size() == 3);
  CHECK(textarea_ptr->gutter_lines[0].css_class == "line-number");
  CHECK(textarea_ptr->gutter_lines[1].css_class == "line-number wrapped");
  CHECK(textarea_ptr->gutter_lines[2].css_class == "line-number active");
}

TEST_CASE(
    "Textarea Component Selection/Cursor/Placeholder Expose Part "
    "Attributes For External ::part() Styling",
    "[component][textarea][part]") {
  auto container = rtxui::Ref<TextareaTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* textarea_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
  REQUIRE(textarea_el != nullptr);
  auto* textarea_ptr = ComponentAs<rtxui::textarea>(textarea_el);
  REQUIRE(textarea_ptr != nullptr);

  textarea_ptr->selection_start = 0;
  textarea_ptr->cursor_pos = 4;  // selects "line" out of "line one..."
  textarea_ptr->Digest();

  auto* selection_el = textarea_el->QuerySelector(".selection");
  REQUIRE(selection_el != nullptr);
  const std::string* selection_part = selection_el->GetAttribute("part");
  REQUIRE(selection_part != nullptr);
  CHECK(*selection_part == "selection");

  auto* cursor_el = textarea_el->QuerySelector(".cursor");
  REQUIRE(cursor_el != nullptr);
  const std::string* cursor_part = cursor_el->GetAttribute("part");
  REQUIRE(cursor_part != nullptr);
  CHECK(cursor_part->find("cursor") != std::string::npos);

  auto* placeholder_el = textarea_el->QuerySelector(".placeholder");
  REQUIRE(placeholder_el != nullptr);
  const std::string* placeholder_part = placeholder_el->GetAttribute("part");
  REQUIRE(placeholder_part != nullptr);
  CHECK(*placeholder_part == "placeholder");
}

TEST_CASE("Textarea Component Enter And Tab Each Form Their Own Undo Step",
          "[component][textarea][undo]") {
  auto container = rtxui::Ref<TextareaTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* textarea_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
  REQUIRE(textarea_el != nullptr);
  auto* textarea_ptr = ComponentAs<rtxui::textarea>(textarea_el);
  REQUIRE(textarea_ptr != nullptr);

  textarea_ptr->value = "ab";
  textarea_ptr->cursor_pos = 2;
  textarea_el->set_focused(true);
  textarea_ptr->Digest();

  textarea_ptr->OnEvent(Event::Return());
  textarea_ptr->Digest();
  CHECK(textarea_ptr->value == "ab\n");

  Event::Keyboard tab_kb;
  tab_kb.special = Event::Keyboard::Special::Tab;
  textarea_ptr->OnEvent(Event(tab_kb));
  textarea_ptr->Digest();
  CHECK(textarea_ptr->value == "ab\n  ");

  // Two separate undo steps: first the tab, then the newline.
  textarea_ptr->OnEvent(Event::CtrlZ());
  textarea_ptr->Digest();
  CHECK(textarea_ptr->value == "ab\n");

  textarea_ptr->OnEvent(Event::CtrlZ());
  textarea_ptr->Digest();
  CHECK(textarea_ptr->value == "ab");
}

TEST_CASE("Textarea Component Basic Typing", "[component][textarea]") {
  auto container = rtxui::Ref<TextareaTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* ta_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
  REQUIRE(ta_el != nullptr);
  auto* ta_ptr = ComponentAs<rtxui::textarea>(ta_el);
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
  rtxui::ComponentInternals::Mount(*container);

  auto* ta_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
  auto* ta_ptr = ComponentAs<rtxui::textarea>(ta_el);
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
  rtxui::ComponentInternals::Mount(*container);

  auto* ta_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
  auto* ta_ptr = ComponentAs<rtxui::textarea>(ta_el);
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
  rtxui::ComponentInternals::Mount(*container);

  auto* ta_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
  auto* ta_ptr = ComponentAs<rtxui::textarea>(ta_el);
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
  rtxui::ComponentInternals::Mount(*container);

  auto* ta_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
  auto* ta_ptr = ComponentAs<rtxui::textarea>(ta_el);
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
  rtxui::ComponentInternals::Mount(*container);

  auto* ta_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
  auto* ta_ptr = ComponentAs<rtxui::textarea>(ta_el);
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

TEST_CASE(
    "Textarea selection highlight actually renders (not just resolves "
    "in base_style)",
    "[component][textarea][regression]") {
  // Regression: the .selection span's background-color correctly resolved
  // in base_style, but was silently dropped at paint time by a
  // LayoutInlineFlow bug (see "Inline span background-color is applied
  // when painted"). Verify the real painted output, not just the resolved
  // style, for both a single-line selection and one spanning multiple
  // lines within the same <span>.
  auto container = rtxui::Ref<TextareaTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* ta_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
  auto* ta_ptr = ComponentAs<rtxui::textarea>(ta_el);
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
    auto root_box = rtxui::LayoutTreeBuilder::Build(
        rtxui::ComponentInternals::Root(*container));
    rtxui::LayoutConstraints viewport = {
        {40, rtxui::MeasureMode::Exactly},
        {10, rtxui::MeasureMode::Exactly},
    };
    auto root_fragment = rtxui::RunLayout({root_box.get()}, viewport);
    Texture texture(40, 10);
    rtxui::Paint(root_fragment, texture);
    auto* selection_el = rtxui::ComponentInternals::Root(*container)
                             ->QuerySelector(".selection");
    if (!selection_el) {
      return false;
    }
    Color bg = texture[selection_el->absolute_x(), selection_el->absolute_y()]
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

}  // namespace

TEST_CASE("Textarea Click Layout Regression Test",
          "[component][textarea][regression]") {
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  auto container = rtxui::Ref<TextareaTestComponent>::New();
  rtxui::Screen screen = rtxui::ScreenInternals::Make(container, device);
  screen.Draw();

  auto* ta_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
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

namespace {

class SmallTextarea : public rtxui::Component<SmallTextarea>,
                      public rtxui::TextInputBase {
 public:
  SmallTextarea() {
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
  SmallTextareaTestComponent() {
    Bind(my_text);
    Import<SmallTextarea>();
  }
  std::string_view view = R"(
    <SmallTextarea value="{my_text}" />
  )";
};

}  // namespace

TEST_CASE("Textarea Scrolling Regression Test",
          "[component][textarea][scroll][regression]") {
  auto container = rtxui::Ref<SmallTextareaTestComponent>::New();
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  rtxui::Screen screen = rtxui::ScreenInternals::Make(container, device);

  auto* ta_el = rtxui::ComponentInternals::Root(*container)
                    ->QuerySelector("SmallTextarea");
  REQUIRE(ta_el != nullptr);

  auto* ta_ptr = ComponentAs<SmallTextarea>(ta_el);
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

TEST_CASE("Textarea PageUp and PageDown", "[component][textarea][navigation]") {
  auto container = rtxui::Ref<TextareaTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);
  auto* textarea_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
  REQUIRE(textarea_el != nullptr);
  auto* textarea_ptr = ComponentAs<rtxui::textarea>(textarea_el);
  REQUIRE(textarea_ptr != nullptr);

  std::string content;
  for (int i = 0; i < 30; ++i) {
    content += "line " + std::to_string(i) + "\n";
  }
  textarea_ptr->value = content;
  textarea_ptr->cursor_pos = 0;
  textarea_el->set_focused(true);
  textarea_ptr->Digest();

  Event::Keyboard page_down;
  page_down.special = Event::Keyboard::Special::PageDown;
  textarea_ptr->OnEvent(Event(page_down));
  textarea_ptr->Digest();

  // PageDown jumped down by default page size (10 lines of "line X\n", 70
  // chars)
  CHECK(textarea_ptr->cursor_pos == 70);

  Event::Keyboard page_up;
  page_up.special = Event::Keyboard::Special::PageUp;
  textarea_ptr->OnEvent(Event(page_up));
  textarea_ptr->Digest();

  CHECK(textarea_ptr->cursor_pos == 0);

  // Shift+PageDown creates selection across pages
  page_down.modifier.shift = true;
  textarea_ptr->OnEvent(Event(page_down));
  textarea_ptr->Digest();
  CHECK(textarea_ptr->selection_start == 0);
  CHECK(textarea_ptr->cursor_pos == 70);
}

TEST_CASE("Textarea Block Indent and Outdent",
          "[component][textarea][indent]") {
  auto container = rtxui::Ref<TextareaTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);
  auto* textarea_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
  REQUIRE(textarea_el != nullptr);
  auto* textarea_ptr = ComponentAs<rtxui::textarea>(textarea_el);
  REQUIRE(textarea_ptr != nullptr);

  textarea_ptr->value = "alpha\nbeta\ngamma";
  textarea_el->set_focused(true);

  // Select alpha and beta
  textarea_ptr->selection_start = 0;
  textarea_ptr->cursor_pos = 10;  // into beta
  textarea_ptr->Digest();

  Event::Keyboard tab_kb;
  tab_kb.special = Event::Keyboard::Special::Tab;
  textarea_ptr->OnEvent(Event(tab_kb));
  textarea_ptr->Digest();

  CHECK(textarea_ptr->value == "  alpha\n  beta\ngamma");

  // Outdent both lines
  tab_kb.modifier.shift = true;
  textarea_ptr->OnEvent(Event(tab_kb));
  textarea_ptr->Digest();

  CHECK(textarea_ptr->value == "alpha\nbeta\ngamma");

  // Outdent 4-space indent: first shift+tab removes 2 spaces, second removes
  // remaining 2
  textarea_ptr->value = "    alpha\n    beta";
  textarea_ptr->selection_start = 0;
  textarea_ptr->cursor_pos = static_cast<int>(textarea_ptr->value.size());
  textarea_ptr->Digest();

  textarea_ptr->OnEvent(Event(tab_kb));
  textarea_ptr->Digest();
  CHECK(textarea_ptr->value == "  alpha\n  beta");

  textarea_ptr->OnEvent(Event(tab_kb));
  textarea_ptr->Digest();
  CHECK(textarea_ptr->value == "alpha\nbeta");
}

TEST_CASE("Textarea Move Line Up and Down",
          "[component][textarea][move_line]") {
  auto container = rtxui::Ref<TextareaTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);
  auto* textarea_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
  REQUIRE(textarea_el != nullptr);
  auto* textarea_ptr = ComponentAs<rtxui::textarea>(textarea_el);
  REQUIRE(textarea_ptr != nullptr);

  textarea_ptr->value = "line 1\nline 2\nline 3";
  textarea_el->set_focused(true);

  // Place cursor in line 2
  textarea_ptr->cursor_pos = 8;
  textarea_ptr->Digest();

  Event::Keyboard alt_up;
  alt_up.special = Event::Keyboard::Special::ArrowUp;
  alt_up.modifier.alt = true;

  // Move line 2 up (swaps with line 1)
  textarea_ptr->OnEvent(Event(alt_up));
  textarea_ptr->Digest();
  CHECK(textarea_ptr->value == "line 2\nline 1\nline 3");

  // Move up again at top line is a no-op
  textarea_ptr->OnEvent(Event(alt_up));
  textarea_ptr->Digest();
  CHECK(textarea_ptr->value == "line 2\nline 1\nline 3");

  // Move line 2 down (swaps with line 1)
  Event::Keyboard alt_down;
  alt_down.special = Event::Keyboard::Special::ArrowDown;
  alt_down.modifier.alt = true;

  textarea_ptr->OnEvent(Event(alt_down));
  textarea_ptr->Digest();
  CHECK(textarea_ptr->value == "line 1\nline 2\nline 3");

  // Move line 2 down again (swaps with line 3)
  textarea_ptr->OnEvent(Event(alt_down));
  textarea_ptr->Digest();
  CHECK(textarea_ptr->value == "line 1\nline 3\nline 2");

  // Move down again at bottom is a no-op
  textarea_ptr->OnEvent(Event(alt_down));
  textarea_ptr->Digest();
  CHECK(textarea_ptr->value == "line 1\nline 3\nline 2");
}

TEST_CASE("Textarea Indentation Uses Spaces And Mouse Click Matches",
          "[component][textarea][indent]") {
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  auto container = rtxui::Ref<TextareaTestComponent>::New();
  rtxui::Screen screen = rtxui::ScreenInternals::Make(container, device);
  screen.Draw();

  auto* ta_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("textarea");
  REQUIRE(ta_el != nullptr);
  auto* ta_ptr = ComponentAs<rtxui::textarea>(ta_el);
  REQUIRE(ta_ptr != nullptr);

  auto* content_el = ta_el->QuerySelector(".content");
  REQUIRE(content_el != nullptr);

  ta_el->set_focused(true);
  ta_ptr->value = "hello";
  ta_ptr->cursor_pos = 0;
  ta_ptr->selection_start = -1;
  ta_ptr->Digest();
  screen.Draw();

  // Press Tab: should insert 2 spaces
  Event::Keyboard tab_kb;
  tab_kb.special = Event::Keyboard::Special::Tab;
  ta_ptr->OnEvent(Event(tab_kb));
  ta_ptr->Digest();
  screen.Draw();

  CHECK(ta_ptr->value == "  hello");
  CHECK(ta_ptr->cursor_pos == 2);

  // Mouse click at column 0 (first space)
  auto click_at = [&](int col) {
    Event::Mouse mouse;
    mouse.button = Event::Mouse::Button::Left;
    mouse.x = content_el->absolute_x() + 1 + col;
    mouse.y = content_el->absolute_y() + 1;
    mouse.motion = Event::Mouse::Motion::Pressed;
    screen.Dispatch(Event(mouse));
    mouse.motion = Event::Mouse::Motion::Released;
    screen.Dispatch(Event(mouse));
    ta_ptr->Digest();
    screen.Draw();
  };

  click_at(0);
  CHECK(ta_ptr->cursor_pos == 0);

  click_at(1);
  CHECK(ta_ptr->cursor_pos == 1);

  click_at(2);
  CHECK(ta_ptr->cursor_pos == 2);

  click_at(3);
  CHECK(ta_ptr->cursor_pos == 3);

  click_at(7);
  CHECK(ta_ptr->cursor_pos == 7);
}
