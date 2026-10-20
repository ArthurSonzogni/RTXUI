// Copyright 2020 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/terminal/terminal_input_parser.hpp"

#include <optional>  // for std::optional
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>  // for vector

#include "catch2/catch_test_macros.hpp"  // for TEST_CASE, REQUIRE, CHECK, CHECK_FALSE
#include "rtxui/color.hpp"
#include "rtxui/event.hpp"  // for Event, Event::Return(), Event::ArrowDown(), Event::ArrowLeft(), Event::ArrowRight(), Event::ArrowUp(), Event::Backspace(), Event::End(), Event::Home(), Event::Delete(), Event::F1(), Event::F10(), Event::F11(), Event::F12(), Event::F2(), Event::F3(), Event::F4(), Event::F5(), Event::F6(), Event::F7(), Event::F8(), Event::F9(), Event::PageDown(), Event::PageUp(), Event::Tab(), Event::TabReverse(), Event::Escape()
#include "rtxui/terminal/event_internal.hpp"

using rtxui::Color;
using rtxui::Event;
using rtxui::TerminalInputParser;

// NOLINTBEGIN

// Test char |c| to are trivially converted into |Event::Character()(c)|.
TEST_CASE("Event.Character", "[terminal]") {
  std::vector<char> basic_char;
  for (char c = 'a'; c <= 'z'; ++c) {
    basic_char.push_back(c);
  }
  for (char c = 'A'; c <= 'Z'; ++c) {
    basic_char.push_back(c);
  }

  TerminalInputParser parser;
  for (char c : basic_char) {
    parser.Add(c);
  }

  std::vector<Event> received_events;
  while (auto event = parser.GetEvent()) {
    received_events.push_back(std::move(*event));
  }

  REQUIRE(received_events.size() == basic_char.size());
  for (size_t i = 0; i < basic_char.size(); ++i) {
    auto keyboard = received_events[i].get_if<Event::Keyboard>();
    CHECK(keyboard);
    CHECK(keyboard->codepoint() == basic_char[i]);
    CHECK(keyboard->motion() == Event::Keyboard::Pressed);
    CHECK_FALSE(keyboard->modifier().alt());
    CHECK_FALSE(keyboard->modifier().ctrl());
    CHECK_FALSE(keyboard->modifier().meta());
    CHECK_FALSE(keyboard->modifier().shift());
  }
}

TEST_CASE("Event.EscapeKeyWithoutWaiting", "[terminal]") {
  TerminalInputParser parser;
  parser.Add('');

  CHECK_FALSE(parser.GetEvent().has_value());
}

TEST_CASE("Event.EscapeKeyNotEnoughWait", "[terminal]") {
  TerminalInputParser parser;
  parser.Add('');
  parser.Timeout(49);

  CHECK_FALSE(parser.GetEvent().has_value());
}

TEST_CASE("Event.EscapeKeyEnoughWait", "[terminal]") {
  TerminalInputParser parser;
  parser.Add('');
  parser.Timeout(50);

  auto event = parser.GetEvent();
  REQUIRE(event.has_value());
  CHECK(*event == Event::Escape());
  CHECK_FALSE(parser.GetEvent().has_value());
}

TEST_CASE("Event.EscapeFast", "[terminal]") {
  TerminalInputParser parser;
  parser.Add('');
  parser.Add('a');
  parser.Add('');
  parser.Add('b');
  parser.Timeout(49);

  std::vector<Event> received_events;
  while (auto event = parser.GetEvent()) {
    received_events.push_back(std::move(*event));
  }

  REQUIRE(received_events.size() == 2);
  CHECK(received_events[0] == Event::AltA());
  CHECK(received_events[1] == Event::AltB());
  CHECK_FALSE(parser.GetEvent().has_value());
}

TEST_CASE("Event.MouseLeftClickPressed", "[terminal]") {
  TerminalInputParser parser;
  parser.Add('');
  parser.Add('[');
  parser.Add('0');
  parser.Add(';');
  parser.Add('1');
  parser.Add('2');
  parser.Add(';');
  parser.Add('4');
  parser.Add('2');
  parser.Add('M');

  auto event = parser.GetEvent();
  REQUIRE(event.has_value());
  auto mouse = event->get_if<Event::Mouse>();
  CHECK(mouse);
  CHECK(mouse->button() == Event::Mouse::Left);
  CHECK(mouse->x() == 12);
  CHECK(mouse->y() == 42);
  CHECK(mouse->motion() == Event::Mouse::Pressed);
  CHECK_FALSE(parser.GetEvent().has_value());
}

TEST_CASE("Event.CsiParameterSaturatesInsteadOfOverflowing", "[terminal]") {
  // Escape sequences arrive from outside the process -- a terminal reply, or
  // anything else holding the tty open -- so a long enough run of digits must
  // not be able to overflow the accumulator. Overflow here is undefined
  // behaviour, not a wrapped coordinate.
  const auto feed = [](std::string_view bytes) {
    TerminalInputParser parser;
    for (const char c : bytes) {
      parser.Add(c);
    }
    return parser.GetEvent();
  };

  SECTION("a long coordinate is capped, and stays a coordinate") {
    const std::string digits(40, '9');
    auto event = feed("\x1b[<0;" + digits + ";" + digits + "M");
    REQUIRE(event.has_value());
    auto mouse = event->get_if<Event::Mouse>();
    REQUIRE(mouse);
    // Whatever the cap is, the point is that it is a large positive number
    // rather than a wrapped or negative one.
    CHECK(mouse->x() > 0);
    CHECK(mouse->y() > 0);
  }

  SECTION("a long button number does not wrap either") {
    const std::string digits(40, '9');
    auto event = feed("\x1b[<" + digits + ";10;10M");
    // It need not be a mouse event at all -- an absurd button is free to be
    // rejected. It must simply not have overflowed getting there.
    if (event.has_value()) {
      if (auto mouse = event->get_if<Event::Mouse>()) {
        CHECK(mouse->x() == 10);
        CHECK(mouse->y() == 10);
      }
    }
  }

  SECTION("ordinary coordinates are unaffected") {
    auto event = feed("\x1b[<0;12;42M");
    REQUIRE(event.has_value());
    auto mouse = event->get_if<Event::Mouse>();
    REQUIRE(mouse);
    CHECK(mouse->x() == 12);
    CHECK(mouse->y() == 42);
  }

  SECTION("a long run of digits in any CSI is survivable") {
    // No event is expected; this is about the accumulator, not the sequence.
    (void)feed("\x1b[" + std::string(10000, '9') + "m");
    (void)feed("\x1b[" + std::string(10000, '9') + "A");
    SUCCEED();
  }
}

TEST_CASE("Event.MouseLeftMoved", "[terminal]") {
  TerminalInputParser parser;
  parser.Add('');
  parser.Add('[');
  parser.Add('3');
  parser.Add('2');
  parser.Add(';');
  parser.Add('1');
  parser.Add('2');
  parser.Add(';');
  parser.Add('4');
  parser.Add('2');
  parser.Add('M');

  auto event = parser.GetEvent();
  REQUIRE(event.has_value());
  auto mouse = event->get_if<Event::Mouse>();
  CHECK(mouse);
  CHECK(mouse->button() == Event::Mouse::Left);
  CHECK(mouse->x() == 12);
  CHECK(mouse->y() == 42);
  CHECK(mouse->motion() == Event::Mouse::Moved);
  CHECK_FALSE(parser.GetEvent().has_value());
}

TEST_CASE("Event.MouseLeftClickReleased", "[terminal]") {
  TerminalInputParser parser;
  parser.Add('');
  parser.Add('[');
  parser.Add('0');
  parser.Add(';');
  parser.Add('1');
  parser.Add('2');
  parser.Add(';');
  parser.Add('4');
  parser.Add('2');
  parser.Add('m');

  auto event = parser.GetEvent();
  REQUIRE(event.has_value());
  auto mouse = event->get_if<Event::Mouse>();
  CHECK(mouse);
  CHECK(mouse->button() == Event::Mouse::Left);
  CHECK(mouse->x() == 12);
  CHECK(mouse->y() == 42);
  CHECK(mouse->motion() == Event::Mouse::Released);
  CHECK_FALSE(parser.GetEvent().has_value());
}

TEST_CASE("Event.MouseReporting", "[terminal]") {
  TerminalInputParser parser;
  parser.Add('');
  parser.Add('[');
  parser.Add('1');
  parser.Add('2');
  parser.Add(';');
  parser.Add('4');
  parser.Add('2');
  parser.Add('R');

  // A cursor position report: RTXUI never asks for one, so it is dropped.
  CHECK_FALSE(parser.GetEvent().has_value());
}

TEST_CASE("Event.MouseMiddleClick", "[terminal]") {
  TerminalInputParser parser;
  parser.Add('');
  parser.Add('[');
  parser.Add('3');
  parser.Add('3');
  parser.Add(';');
  parser.Add('1');
  parser.Add('2');
  parser.Add(';');
  parser.Add('4');
  parser.Add('2');
  parser.Add('M');

  auto event = parser.GetEvent();
  REQUIRE(event.has_value());
  auto mouse = event->get_if<Event::Mouse>();
  CHECK(mouse);
  CHECK(mouse->button() == Event::Mouse::Middle);
  CHECK(mouse->x() == 12);
  CHECK(mouse->y() == 42);
  CHECK_FALSE(parser.GetEvent().has_value());
}

TEST_CASE("Event.MouseRightClick", "[terminal]") {
  TerminalInputParser parser;
  parser.Add('');
  parser.Add('[');
  parser.Add('3');
  parser.Add('4');
  parser.Add(';');
  parser.Add('1');
  parser.Add('2');
  parser.Add(';');
  parser.Add('4');
  parser.Add('2');
  parser.Add('M');

  auto event = parser.GetEvent();
  REQUIRE(event.has_value());
  auto mouse = event->get_if<Event::Mouse>();
  CHECK(mouse);
  CHECK(mouse->button() == Event::Mouse::Right);
  CHECK(mouse->x() == 12);
  CHECK(mouse->y() == 42);
  CHECK_FALSE(parser.GetEvent().has_value());
}

// The bytes urxvt 9.31 sends, with `?1003h` and `?1006h`, for: a move, a
// left click, a move, a right drag, then a move. It reports a move with no
// button held as 31 before any press, and as a drag of the button released
// last afterwards.
TEST_CASE("Event.MouseUrxvtMotionWithoutButton", "[terminal]") {
  TerminalInputParser parser;
  for (char c :
       std::string_view("\x1b[<31;2;2M"
                        "\x1b[<0;8;5M\x1b[<32;12;5M\x1b[<0;15;5m\x1b[<32;18;5M"
                        "\x1b[<2;22;5M\x1b[<34;25;5M\x1b[<2;25;5m\x1b[<34;28;5M"
                        "\x1b[<65;30;5M\x1b[<97;31;5M")) {
    parser.Add(c);
  }
  struct Expected {
    Event::Mouse::Button button;
    Event::Mouse::Motion motion;
    int x;
  };
  const Expected expected[] = {
      {Event::Mouse::None, Event::Mouse::Moved, 2},
      {Event::Mouse::Left, Event::Mouse::Pressed, 8},
      {Event::Mouse::Left, Event::Mouse::Moved, 12},
      {Event::Mouse::Left, Event::Mouse::Released, 15},
      {Event::Mouse::None, Event::Mouse::Moved, 18},
      {Event::Mouse::Right, Event::Mouse::Pressed, 22},
      {Event::Mouse::Right, Event::Mouse::Moved, 25},
      {Event::Mouse::Right, Event::Mouse::Released, 25},
      {Event::Mouse::None, Event::Mouse::Moved, 28},
      {Event::Mouse::WheelDown, Event::Mouse::Pressed, 30},
      {Event::Mouse::None, Event::Mouse::Moved, 31},
  };
  for (const Expected& e : expected) {
    auto event = parser.GetEvent();
    REQUIRE(event.has_value());
    auto mouse = event->get_if<Event::Mouse>();
    REQUIRE(mouse);
    CHECK(mouse->button() == e.button);
    CHECK(mouse->motion() == e.motion);
    CHECK(mouse->x() == e.x);
    CHECK_FALSE(mouse->modifier().shift());
    CHECK_FALSE(mouse->modifier().meta());
    CHECK_FALSE(mouse->modifier().ctrl());
  }
  CHECK_FALSE(parser.GetEvent().has_value());
}

// The keys urxvt encodes its own way (captures/urxvt-9.31.txt).
TEST_CASE("Event.RxvtKeys", "[terminal]") {
  const std::pair<std::string_view, Event> keys[] = {
      {"\x1BOa", Event::ArrowUpCtrl()},
      {"\x1BOb", Event::ArrowDownCtrl()},
      {"\x1BOc", Event::ArrowRightCtrl()},
      {"\x1BOd", Event::ArrowLeftCtrl()},
      {"\x1B[7~", Event::Home()},
      {"\x1B[8~", Event::End()},
      {"\x1B\x1B[A", Event::ArrowUpAlt()},
      {"\x1B\x1B[B", Event::ArrowDownAlt()},
      {"\x1B\x1B[C", Event::ArrowRightAlt()},
      {"\x1B\x1B[D", Event::ArrowLeftAlt()},
  };
  for (const auto& [sequence, expected] : keys) {
    TerminalInputParser parser;
    for (char c : sequence) {
      parser.Add(c);
    }
    auto event = parser.GetEvent();
    REQUIRE(event.has_value());
    CHECK(*event == expected);
    CHECK_FALSE(parser.GetEvent().has_value());
  }
}

// tmux, screen and the Linux console (captures/tmux-3.6.txt).
TEST_CASE("Event.MultiplexerHomeEnd", "[terminal]") {
  const std::pair<std::string_view, Event> keys[] = {
      {"\x1B[1~", Event::Home()},
      {"\x1B[4~", Event::End()},
  };
  for (const auto& [sequence, expected] : keys) {
    TerminalInputParser parser;
    for (char c : sequence) {
      parser.Add(c);
    }
    auto event = parser.GetEvent();
    REQUIRE(event.has_value());
    CHECK(*event == expected);
    CHECK_FALSE(parser.GetEvent().has_value());
  }
}

// screen holds an Escape back and sends it with the next key
// (captures/screen-4.09.01.txt).
TEST_CASE("Event.EscapeSentWithTheNextKey", "[terminal]") {
  TerminalInputParser parser;
  for (char c : std::string_view("\x1B\x1BOQ\x1B\x1B[A")) {
    parser.Add(c);
  }
  CHECK(parser.GetEvent() == Event::Escape());
  CHECK(parser.GetEvent() == Event::F2());
  // rxvt's Alt+Up still reads as one.
  CHECK(parser.GetEvent() == Event::ArrowUpAlt());
  CHECK_FALSE(parser.GetEvent().has_value());
}

// A run of Escapes is parsed again for every byte until it completes. Reading
// it recursively, one level per Escape, overflowed the stack on a long run
// (found by the TerminalInputParser fuzzer).
TEST_CASE("Event.LongRunOfEscapes", "[terminal]") {
  constexpr int kEscapes = 100'000;
  TerminalInputParser parser;
  for (int i = 0; i < kEscapes; ++i) {
    parser.Add('\x1B');
  }
  for (char c : std::string_view("[A")) {
    parser.Add(c);
  }
  // Every Escape but the last two is a key of its own; those two and "[A"
  // are rxvt's Alt+Up.
  int escapes = 0;
  std::optional<Event> event = parser.GetEvent();
  while (event == Event::Escape()) {
    ++escapes;
    event = parser.GetEvent();
  }
  CHECK(escapes == kEscapes - 2);
  CHECK(event == Event::ArrowUpAlt());
  CHECK_FALSE(parser.GetEvent().has_value());
}

// A terminal that does not know the SGR encoding sends `ESC[M` and three
// bytes, each 32 more than the button, the column and the row.
TEST_CASE("Event.MouseX10", "[terminal]") {
  TerminalInputParser parser;
  const auto report = [](int button, int x, int y) {
    return std::string("\x1B[M") + static_cast<char>(32 + button) +
           static_cast<char>(32 + x) + static_cast<char>(32 + y);
  };
  for (char c : report(0, 10, 5) + report(32, 11, 5) + report(3, 12, 5) +
                    report(35, 13, 5) + report(2 + 16, 14, 5) +
                    report(3 + 16, 14, 5) + report(65, 200, 6)) {
    parser.Add(c);
  }
  struct Expected {
    Event::Mouse::Button button;
    Event::Mouse::Motion motion;
    int x;
    int y;
  };
  const Expected expected[] = {
      {Event::Mouse::Left, Event::Mouse::Pressed, 10, 5},
      {Event::Mouse::Left, Event::Mouse::Moved, 11, 5},
      {Event::Mouse::Left, Event::Mouse::Released, 12, 5},
      {Event::Mouse::None, Event::Mouse::Moved, 13, 5},
      {Event::Mouse::Right, Event::Mouse::Pressed, 14, 5},
      {Event::Mouse::Right, Event::Mouse::Released, 14, 5},
      {Event::Mouse::WheelDown, Event::Mouse::Pressed, 200, 6},
  };
  for (const Expected& e : expected) {
    auto event = parser.GetEvent();
    REQUIRE(event.has_value());
    INFO(event->Print());
    auto mouse = event->get_if<Event::Mouse>();
    REQUIRE(mouse);
    CHECK(mouse->button() == e.button);
    CHECK(mouse->motion() == e.motion);
    CHECK(mouse->x() == e.x);
    CHECK(mouse->y() == e.y);
  }
  CHECK_FALSE(parser.GetEvent().has_value());

  // `ESC[M` alone is still some consoles' F1.
  for (char c : std::string_view("\x1B[M")) {
    parser.Add(c);
  }
  parser.Timeout(1000);
  CHECK(parser.GetEvent() == Event::F1());
}

TEST_CASE("Event.UTF8", "[terminal]") {
  struct {
    std::vector<unsigned char> input;
    bool valid;
  } kTestCase[] = {
      // Basic characters.
      {{'a'}, true},
      {{'z'}, true},
      {{'A'}, true},
      {{'Z'}, true},
      {{'0'}, true},
      {{'9'}, true},

      // UTF-8 of various size:
      {{0b0100'0001}, true},
      {{0b1100'0010, 0b1000'0000}, true},
      {{0b1110'0010, 0b1000'0000, 0b1000'0000}, true},
      {{0b1111'0010, 0b1000'0000, 0b1000'0000, 0b1000'0000}, true},

      // Overlong UTF-8 encoding:
      {{0b1100'0000, 0b1000'0000}, false},
      {{0b1110'0000, 0b1000'0000, 0b1000'0000}, false},
      {{0b1111'0000, 0b1000'0000, 0b1000'0000, 0b1000'0000}, false},

      // Test limits in between the various legal regions
      // https://unicode.org/versions/corrigendum1.html
      // Limit in between the valid and ina
      // {{0x7F}, true}, => Special sequence.
      {{0x80}, false},
      // ---
      {{0xC1, 0x80}, false},
      {{0xC2, 0x7F}, false},
      {{0xC2, 0x80}, true},
      // ---
      {{0xDF, 0xBF}, true},
      {{0xDF, 0xC0}, false},
      // ---
      {{0xE0, 0x9F, 0x80}, false},
      {{0xE0, 0xA0, 0x7F}, false},
      {{0xE0, 0xA0, 0x80}, true},
      // ---
      {{0xE0, 0xBF, 0xBF}, true},
      // ---
      {{0xE1, 0x7F, 0x80}, false},
      {{0xE1, 0x80, 0x7f}, false},
      {{0xE1, 0x80, 0x80}, true},
      // --
      {{0xEF, 0xBF, 0xBF}, true},
      {{0xEF, 0xC0, 0xBF}, false},
      {{0xEF, 0xBF, 0xC0}, false},
      // --
      {{0xF0, 0x90, 0x80}, false},
      {{0xF0, 0x8F, 0x80, 0x80}, false},
      {{0xF0, 0x90, 0x80, 0x7F}, false},
      {{0xF0, 0x90, 0x80, 0x80}, true},
      // --
      {{0xF1, 0x80, 0x80, 0x80}, true},
      // --
      {{0xF1, 0xBF, 0xBF, 0xBF}, true},
      // --
      {{0xF2, 0x80, 0x80, 0x80}, true},
      // --
      {{0xF4, 0x8F, 0xBF, 0xBF}, true},
      {{0xF4, 0x90, 0xBF, 0xBF}, false},

  };
  for (auto test : kTestCase) {
    TerminalInputParser parser;
    for (auto input : test.input) {
      parser.Add(input);
    }

    auto event = parser.GetEvent();
    if (test.valid) {
      REQUIRE(event.has_value());
      auto keyboard = event->get_if<Event::Keyboard>();
      CHECK(keyboard);
      CHECK_FALSE(parser.GetEvent().has_value());
    } else {
      CHECK_FALSE(event.has_value());
    }
  }
}

TEST_CASE("Event.NewLine", "[terminal]") {
  for (char newline : {'\r', '\n'}) {
    TerminalInputParser parser;
    parser.Add(newline);

    auto event = parser.GetEvent();
    REQUIRE(event.has_value());
    CHECK(*event == Event::Return());
    CHECK_FALSE(parser.GetEvent().has_value());
  }
}

TEST_CASE("Event.Keyboard", "[terminal]") {
  auto str = [](std::string input) {
    std::vector<unsigned char> output;
    for (auto it : input) {
      output.push_back(it);
    }
    return output;
  };

  struct {
    std::vector<unsigned char> input;
    Event expected;
  } kTestCase[] = {
      // Arrow (default cursor mode)
      {str("[A"), Event::ArrowUp()},
      {str("[B"), Event::ArrowDown()},
      {str("[C"), Event::ArrowRight()},
      {str("[D"), Event::ArrowLeft()},
      {str("[H"), Event::Home()},
      {str("[F"), Event::End()},

      // Arrow (application cursor mode)
      {str("\x1BOA"), Event::ArrowUp()},
      {str("\x1BOB"), Event::ArrowDown()},
      {str("\x1BOC"), Event::ArrowRight()},
      {str("\x1BOD"), Event::ArrowLeft()},
      {str("\x1BOH"), Event::Home()},
      {str("\x1BOF"), Event::End()},

      // Ctrl/Alt Arrow
      {str("\x1B[1;5A"), Event::ArrowUpCtrl()},
      {str("\x1B[1;5B"), Event::ArrowDownCtrl()},
      {str("\x1B[1;5C"), Event::ArrowRightCtrl()},
      {str("\x1B[1;5D"), Event::ArrowLeftCtrl()},
      {str("\x1B[1;3A"), Event::ArrowUpAlt()},
      {str("\x1B[1;3B"), Event::ArrowDownAlt()},
      {str("\x1B[1;3C"), Event::ArrowRightAlt()},
      {str("\x1B[1;3D"), Event::ArrowLeftAlt()},

      // Backspace & Quirk for:
      // https://github.com/ArthurSonzogni/FTXUI/issues/508
      {{127}, Event::Backspace()},
      {{8}, Event::Backspace()},
      {str("\x1B\x7F"), Event::BackspaceAlt()},
      {str("\x1B\x08"), Event::BackspaceAlt()},
      {str("\x1B[127;5u"), Event::BackspaceCtrl()},
      {str("\x1B[8;5u"), Event::BackspaceCtrl()},
      {str("\x1B[127;3u"), Event::BackspaceAlt()},
      {str("\x1B[8;3u"), Event::BackspaceAlt()},

      // Delete
      {str("\x1B[3~"), Event::Delete()},
      {str("\x1B[3;5~"), Event::DeleteCtrl()},
      {str("\x1B[3;3~"), Event::DeleteAlt()},

      // Return
      {{13}, Event::Return()},
      {{10}, Event::Return()},

      // Tabs:
      {{9}, Event::Tab()},
      {{27, 91, 90}, Event::TabReverse()},

      // Function keys
      {str("\x1BOP"), Event::F1()},
      {str("\x1BOQ"), Event::F2()},
      {str("\x1BOR"), Event::F3()},
      {str("\x1BOS"), Event::F4()},
      {str("\x1B[15~"), Event::F5()},
      {str("\x1B[17~"), Event::F6()},
      {str("\x1B[18~"), Event::F7()},
      {str("\x1B[19~"), Event::F8()},
      {str("\x1B[20~"), Event::F9()},
      {str("\x1B[21~"), Event::F10()},
      {str("\x1B[23~"), Event::F11()},
      {str("\x1B[24~"), Event::F12()},

      // Function keys for virtual terminal:
      {str("\x1B[[A"), Event::F1()},
      {str("\x1B[[B"), Event::F2()},
      {str("\x1B[[C"), Event::F3()},
      {str("\x1B[[D"), Event::F4()},
      {str("\x1B[[E"), Event::F5()},

      // Function keys for xterm-r5, xterm-r6, rxvt
      {str("\x1B[11~"), Event::F1()},
      {str("\x1B[12~"), Event::F2()},
      {str("\x1B[13~"), Event::F3()},
      {str("\x1B[14~"), Event::F4()},

      // Function keys for vt100
      {str("\x1BOt"), Event::F5()},
      {str("\x1BOu"), Event::F6()},
      {str("\x1BOv"), Event::F7()},
      {str("\x1BOl"), Event::F8()},
      {str("\x1BOw"), Event::F9()},
      {str("\x1BOx"), Event::F10()},

      // Function keys for scoansi
      {str("\x1B[M"), Event::F1()},
      {str("\x1B[N"), Event::F2()},
      {str("\x1B[O"), Event::F3()},
      {str("\x1B[P"), Event::F4()},
      {str("\x1B[Q"), Event::F5()},
      {str("\x1B[R"), Event::F6()},
      {str("\x1B[S"), Event::F7()},
      {str("\x1B[T"), Event::F8()},
      {str("\x1B[U"), Event::F9()},
      {str("\x1B[V"), Event::F10()},
      {str("\x1B[W"), Event::F11()},
      {str("\x1B[X"), Event::F12()},

      // Page up and down:
      {str("\x1B[5~"), Event::PageUp()},
      {str("\x1B[6~"), Event::PageDown()},

      // Custom:

  };

  for (auto test : kTestCase) {
    TerminalInputParser parser;
    for (auto input : test.input) {
      parser.Add(input);
    }
    // `ESC[M`, F1 on some consoles, may also start an X10 mouse report.
    parser.Timeout(1000);
    auto event = parser.GetEvent();
    REQUIRE(event.has_value());
    CHECK(*event == test.expected);
    CHECK_FALSE(parser.GetEvent().has_value());
  }
}

TEST_CASE("Event.DeviceControlString", "[terminal]") {
  TerminalInputParser parser;
  parser.Add(27);   // ESC
  parser.Add(80);   // P
  parser.Add(49);   // 1
  parser.Add(36);   // $
  parser.Add(114);  // r
  parser.Add(49);   // 1
  parser.Add(32);   // SP
  parser.Add(113);  // q
  parser.Add(27);   // ESC
  parser.Add(92);   // (backslash)

  // A cursor shape report: RTXUI never asks for one, so it is dropped.
  CHECK_FALSE(parser.GetEvent().has_value());
}

TEST_CASE("Event.BracketedPaste", "[terminal][paste]") {
  TerminalInputParser parser;
  std::string sequence = "\x1B[200~hi\nb\x1B[201~";
  for (char c : sequence) {
    parser.Add(c);
  }

  std::vector<Event> received_events;
  while (auto event = parser.GetEvent()) {
    received_events.push_back(std::move(*event));
  }

  // "hi" + a plain '\n' character (from '\n') + "b", nothing from the
  // start/end markers themselves. A plain '\n' codepoint, not
  // Event::Return(): the latter is "user pressed Enter" and carries over
  // the current line's indentation, which pasted text must not get (it
  // already has its own).
  REQUIRE(received_events.size() == 4);
  CHECK(received_events[0].get_if<Event::Keyboard>()->codepoint() == 'h');
  CHECK(received_events[1].get_if<Event::Keyboard>()->codepoint() == 'i');
  auto newline_kb = received_events[2].get_if<Event::Keyboard>();
  REQUIRE(newline_kb);
  CHECK(newline_kb->codepoint() == '\n');
  CHECK(newline_kb->special() == Event::Keyboard::Special::None);
  CHECK(received_events[3].get_if<Event::Keyboard>()->codepoint() == 'b');

  // Every event synthesized from pasted text (including the plain '\n')
  // is marked from_paste, so components can tell it apart from the user
  // actually typing (e.g. TextInputBase's undo grouping).
  for (const auto& event : received_events) {
    CHECK(event.get_if<Event::Keyboard>()->from_paste());
  }
}

TEST_CASE("Event.BracketedPasteDoesNotInterpretContentAsEscapeSequences",
          "[terminal][paste]") {
  // Pasted text containing what looks like an escape sequence (e.g. copied
  // from a terminal session log) must be inserted as literal characters,
  // not interpreted -- that's the entire point of bracketed paste mode.
  TerminalInputParser parser;
  std::string sequence = "\x1B[200~\x1B[Ax\x1B[201~";
  for (char c : sequence) {
    parser.Add(c);
  }

  std::vector<Event> received_events;
  while (auto event = parser.GetEvent()) {
    received_events.push_back(std::move(*event));
  }

  REQUIRE(received_events.size() == 4);
  CHECK(received_events[0].get_if<Event::Keyboard>()->codepoint() == '\x1B');
  CHECK(received_events[1].get_if<Event::Keyboard>()->codepoint() == '[');
  CHECK(received_events[2].get_if<Event::Keyboard>()->codepoint() == 'A');
  CHECK(received_events[3].get_if<Event::Keyboard>()->codepoint() == 'x');
}

TEST_CASE("Event.BracketedPasteUtf8", "[terminal][paste]") {
  TerminalInputParser parser;
  std::string sequence = "\x1B[200~é\x1B[201~";
  for (char c : sequence) {
    parser.Add(c);
  }

  auto event = parser.GetEvent();
  REQUIRE(event.has_value());
  auto keyboard = event->get_if<Event::Keyboard>();
  REQUIRE(keyboard);
  CHECK(keyboard->codepoint() ==
        0xE9);  // U+00E9 LATIN SMALL LETTER E WITH ACUTE
  CHECK_FALSE(parser.GetEvent().has_value());
}

TEST_CASE("Event.OSCRepliesAreSwallowed", "[terminal][osc]") {
  // Regression: ESC ] had no handler, so a terminal replying to a query --
  // its background color, the clipboard, the window title -- fell through to
  // the unknown-escape path and was delivered to the application one
  // character at a time. Asking a terminal a question typed the answer into
  // whatever had focus.
  auto events_for = [](std::string_view sequence) {
    TerminalInputParser parser;
    for (char c : sequence) {
      parser.Add(c);
    }
    std::vector<Event> events;
    while (auto event = parser.GetEvent()) {
      events.push_back(std::move(*event));
    }
    return events;
  };

  SECTION("terminated by ST") {
    // A reply to the OSC 11 background-color query.
    CHECK(events_for("\x1B]11;rgb:0d0d/1111/1717\x1B\\").empty());
  }

  SECTION("terminated by BEL") {
    CHECK(events_for("\x1B]11;rgb:0000/0000/0000\x07").empty());
  }

  SECTION("the keypress after a reply still arrives") {
    auto events = events_for("\x1B]11;rgb:0d0d/1111/1717\x07x");
    REQUIRE(events.size() == 1);
    CHECK(events[0].get_if<Event::Keyboard>()->codepoint() == 'x');
  }

  SECTION("an unterminated reply consumes the rest rather than emitting it") {
    CHECK(events_for("\x1B]11;rgb:0d0d").empty());
  }
}

TEST_CASE("The terminal's background color is read from its OSC 11 reply",
          "[terminal][parser]") {
  auto background_for = [](std::string_view sequence) {
    TerminalInputParser parser;
    for (char c : sequence) {
      parser.Add(c);
    }
    return parser.TakeBackgroundColor();
  };
  CHECK(background_for("\x1B]11;rgb:0d0d/1111/1717\x1B\\") ==
        Color::RGB(0x0d, 0x11, 0x17));
  CHECK(background_for("\x1B]11;rgb:ff/80/00\x07") ==
        Color::RGB(0xff, 0x80, 0x00));
  // One to four hex digits a channel, each scaled to the full range.
  CHECK(background_for("\x1B]11;rgb:f/8/0\x07") ==
        Color::RGB(0xff, 0x88, 0x00));
  CHECK(background_for("\x1B]11;rgb:ffff/0000/8000\x07") ==
        Color::RGB(0xff, 0x00, 0x80));

  CHECK_FALSE(background_for("\x1B]11;rgb:0d0d/1111\x07"));
  CHECK_FALSE(background_for("\x1B]11;rgb:0g/00/00\x07"));
  CHECK_FALSE(background_for("\x1B]11;rgb:00000/00/00\x07"));
  CHECK_FALSE(background_for("\x1B]10;rgb:00/00/00\x07"));  // Foreground.
  CHECK_FALSE(background_for("x"));

  // Taken once.
  TerminalInputParser parser;
  for (char c : std::string_view("\x1B]11;rgb:00/00/00\x07")) {
    parser.Add(c);
  }
  CHECK(parser.TakeBackgroundColor());
  CHECK_FALSE(parser.TakeBackgroundColor());
}

TEST_CASE("The terminal's text color is read from its OSC 10 reply",
          "[terminal][parser]") {
  auto foreground_for = [](std::string_view sequence) {
    TerminalInputParser parser;
    for (char c : sequence) {
      parser.Add(c);
    }
    CHECK_FALSE(parser.GetEvent());
    return parser.TakeForegroundColor();
  };
  CHECK(foreground_for("\x1B]10;rgb:0d0d/1111/1717\x1B\\") ==
        Color::RGB(0x0d, 0x11, 0x17));
  CHECK(foreground_for("\x1B]10;rgb:ff/80/00\x07") ==
        Color::RGB(0xff, 0x80, 0x00));
  CHECK_FALSE(foreground_for("\x1B]10;rgb:0d0d/1111\x07"));
  CHECK_FALSE(foreground_for("\x1B]11;rgb:00/00/00\x07"));  // Background.
  CHECK_FALSE(foreground_for("\x1B]100;rgb:00/00/00\x07"));
}

// --- Event construction ---

TEST_CASE("Event converts only from its three kinds", "[event]") {
  static_assert(std::is_constructible_v<Event, Event::Keyboard>);
  static_assert(std::is_constructible_v<Event, Event::Mouse>);
  static_assert(std::is_constructible_v<Event, Event::Resized>);

  static_assert(!std::is_constructible_v<Event, int>);
  static_assert(!std::is_constructible_v<Event, const char*>);
  static_assert(!std::is_constructible_v<Event, std::string>);

  static_assert(std::is_copy_constructible_v<Event>);
  static_assert(std::is_move_constructible_v<Event>);
  static_assert(!std::is_default_constructible_v<Event>);

  Event::Modifier none;
  CHECK(none == Event::Modifier());
  CHECK_FALSE(none ==
              rtxui::EventInternals::Make(rtxui::ModifierData{.ctrl = true}));

  Event event = rtxui::KeyboardData{.codepoint = 'x'};
  REQUIRE(event.get_if<Event::Keyboard>().has_value());
  CHECK(event.get_if<Event::Keyboard>()->codepoint() == 'x');
  CHECK_FALSE(event.get_if<Event::Mouse>().has_value());
}

TEST_CASE("Parsed keys equal the named Event constants", "[terminal][event]") {
  // Apps match keys by comparing against Event::ArrowUp(), Event::CtrlA(),
  // ...; a constant that differs from what the parser produces for the same
  // key never matches, silently.
  auto parse = [](std::string_view bytes) {
    TerminalInputParser parser;
    for (char c : bytes) {
      parser.Add(c);
    }
    auto event = parser.GetEvent();
    REQUIRE(event.has_value());
    CHECK_FALSE(parser.GetEvent().has_value());  // Exactly one event.
    return *event;
  };
  CHECK(parse("a") == Event::a());
  CHECK(parse("A") == Event::A());
  CHECK(parse("\x01") == Event::CtrlA());
  CHECK(parse("\033a") == Event::AltA());  // ESC, then a.
  CHECK(parse("\r") == Event::Return());
  CHECK(parse("\t") == Event::Tab());
  CHECK(parse("\x1b[Z") == Event::TabReverse());
  CHECK(parse("\x7f") == Event::Backspace());
  CHECK(parse("\x1b\x7f") == Event::BackspaceAlt());
  CHECK(parse("\x1b[A") == Event::ArrowUp());
  CHECK(parse("\x1b[1;5A") == Event::ArrowUpCtrl());
  CHECK(parse("\x1b[1;3D") == Event::ArrowLeftAlt());
  CHECK(parse("\x1b[3~") == Event::Delete());
  CHECK(parse("\x1b[3;5~") == Event::DeleteCtrl());
  CHECK(parse("\x1b[2~") == Event::Insert());
  CHECK(parse("\x1b[H") == Event::Home());
  CHECK(parse("\x1b[F") == Event::End());
  CHECK(parse("\x1b[5~") == Event::PageUp());
  CHECK(parse("\x1b[6~") == Event::PageDown());
  CHECK(parse("\x1bOP") == Event::F1());
  CHECK(parse("\x1b[24~") == Event::F12());
}

// NOLINTEND
