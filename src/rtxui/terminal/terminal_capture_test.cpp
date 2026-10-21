// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.

// Replays the bytes real terminals sent, recorded by
// scripts/record_terminal.py into captures/, through TerminalInputParser.
// The Windows ones were recorded the same way in a Windows VM, the gestures
// sent through QEMU.
// Each line of a capture is a gesture's name, a tab, and the bytes the
// terminal sent for it, with \xHH for anything not printable.
#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <system_error>
#include <vector>

#include "catch2/catch_message.hpp"
#include "catch2/catch_test_macros.hpp"
#include "rtxui/event.hpp"
#include "rtxui/terminal/terminal_input_parser.hpp"

using rtxui::Event;
using rtxui::TerminalInputParser;

namespace {

std::string Unescape(const std::string& line) {
  std::string bytes;
  for (size_t i = 0; i < line.size(); ++i) {
    if (line[i] == '\\' && i + 3 < line.size() && line[i + 1] == 'x') {
      bytes += static_cast<char>(std::stoi(line.substr(i + 2, 2), nullptr, 16));
      i += 3;
    } else {
      bytes += line[i];
    }
  }
  return bytes;
}

struct Mouse {
  Event::Mouse::Button button;
  Event::Mouse::Motion motion;
  // The modifier held, as Held() names it.
  std::string held;
};

std::string Held(const Event::Modifier& modifier) {
  std::string held;
  held += modifier.shift() ? "shift " : "";
  held += modifier.ctrl() ? "ctrl " : "";
  held += modifier.alt() ? "alt " : "";
  held += modifier.meta() ? "meta " : "";
  return held;
}

const std::map<std::string, Mouse>& MouseGestures() {
  static const std::map<std::string, Mouse> gestures = {
      {"move", {Event::Mouse::None, Event::Mouse::Moved}},
      {"left_press", {Event::Mouse::Left, Event::Mouse::Pressed}},
      {"left_drag", {Event::Mouse::Left, Event::Mouse::Moved}},
      {"left_release", {Event::Mouse::Left, Event::Mouse::Released}},
      {"move_after_left", {Event::Mouse::None, Event::Mouse::Moved}},
      {"right_press", {Event::Mouse::Right, Event::Mouse::Pressed}},
      {"right_drag", {Event::Mouse::Right, Event::Mouse::Moved}},
      {"right_release", {Event::Mouse::Right, Event::Mouse::Released}},
      {"move_after_right", {Event::Mouse::None, Event::Mouse::Moved}},
      {"middle_press", {Event::Mouse::Middle, Event::Mouse::Pressed}},
      {"middle_release", {Event::Mouse::Middle, Event::Mouse::Released}},
      {"wheel_up", {Event::Mouse::WheelUp, Event::Mouse::Pressed}},
      {"wheel_down", {Event::Mouse::WheelDown, Event::Mouse::Pressed}},
      {"shift_move", {Event::Mouse::None, Event::Mouse::Moved, "shift "}},
      {"ctrl_move", {Event::Mouse::None, Event::Mouse::Moved, "ctrl "}},
      // The mouse encodings call Alt "meta".
      {"alt_move", {Event::Mouse::None, Event::Mouse::Moved, "meta "}},
  };
  return gestures;
}

// The event each key gesture must give; some terminals have two ways.
const std::map<std::string, std::vector<Event>>& KeyGestures() {
  static const std::map<std::string, std::vector<Event>> gestures = {
      {"a", {Event::a()}},
      {"shift_a", {Event::A()}},
      {"ctrl_a", {Event::CtrlA()}},
      // xterm sends Alt+A as `á` unless its metaSendsEscape is set.
      {"alt_a", {Event::AltA(), Event(Event::Keyboard::From(0xE1U))}},
      {"e_acute", {Event(Event::Keyboard::From(0xE9U))}},
      {"up", {Event::ArrowUp()}},
      {"down", {Event::ArrowDown()}},
      {"right", {Event::ArrowRight()}},
      {"left", {Event::ArrowLeft()}},
      {"ctrl_left", {Event::ArrowLeftCtrl()}},
      {"ctrl_right", {Event::ArrowRightCtrl()}},
      // VS Code sends Ctrl+Up for Alt+Up.
      {"alt_up", {Event::ArrowUpAlt(), Event::ArrowUpCtrl()}},
      {"home", {Event::Home()}},
      {"end", {Event::End()}},
      {"page_up", {Event::PageUp()}},
      {"page_down", {Event::PageDown()}},
      {"insert", {Event::Insert()}},
      {"delete", {Event::Delete()}},
      {"backspace", {Event::Backspace()}},
      {"return", {Event::Return()}},
      {"tab", {Event::Tab()}},
      {"shift_tab", {Event::TabReverse()}},
      {"escape", {Event::Escape()}},
      {"f1", {Event::F1()}},
      {"f2", {Event::F2()}},
      {"f3", {Event::F3()}},
      {"f4", {Event::F4()}},
      {"f5", {Event::F5()}},
      {"f12", {Event::F12()}},
  };
  return gestures;
}

std::vector<std::filesystem::path> Captures() {
  std::vector<std::filesystem::path> captures;
  std::error_code error;
  for (const auto& entry : std::filesystem::directory_iterator(
           RTXUI_TERMINAL_CAPTURES_DIR, error)) {
    if (entry.path().extension() == ".txt") {
      captures.push_back(entry.path());
    }
  }
  std::ranges::sort(captures);
  return captures;
}

}  // namespace

TEST_CASE("Every terminal capture is read as the gestures it recorded",
          "[terminal][captures]") {
  const auto captures = Captures();
  REQUIRE_FALSE(captures.empty());
  for (const auto& capture : captures) {
    // One parser for the whole capture, as one screen reads one terminal:
    // what a terminal sent before can change how it is read.
    TerminalInputParser parser;
    // What the last key gesture should have given, when nothing came: screen
    // holds an Escape back and sends it with the next key.
    std::vector<Event> held;
    std::ifstream file(capture);
    std::string line;
    while (std::getline(file, line)) {
      if (line.empty() || line[0] == '#') {
        continue;
      }
      const size_t tab = line.find('\t');
      REQUIRE(tab != std::string::npos);
      const std::string gesture = line.substr(0, tab);
      const std::string bytes = Unescape(line.substr(tab + 1));
      INFO(capture.filename().string() << ": " << gesture);
      for (char c : bytes) {
        parser.Add(c);
      }
      // A lone Escape waits to be told apart from the start of a sequence.
      parser.Timeout(1000);
      std::vector<Event> events;
      while (auto event = parser.GetEvent()) {
        events.push_back(*event);
      }

      if (gesture == "reply_colors") {
        CHECK(events.empty());
        // screen answers for the background only.
        CHECK(parser.TakeBackgroundColor().has_value() ==
              bytes.contains("]11;"));
        CHECK(parser.TakeForegroundColor().has_value() ==
              bytes.contains("]10;"));
      } else if (gesture == "reply_cursor_position") {
        CHECK(events.empty());
        CHECK(parser.TakeCursorPositions().size() == 1);
      } else if (gesture == "reply_pixel_mode") {
        CHECK(events.empty());
        if (!bytes.empty()) {
          CHECK(parser.TakePixelMouse().has_value());
        }
      } else if (bytes.empty()) {
        // The terminal kept the gesture for itself, like tilix's Alt+Up
        // shortcut: nothing reaches the application to read.
        held = KeyGestures().contains(gesture) ? KeyGestures().at(gesture)
                                               : std::vector<Event>{};
        continue;
      } else if (MouseGestures().contains(gesture)) {
        const Mouse expected = MouseGestures().at(gesture);
        CHECK_FALSE(events.empty());
        // What may follow a press or a release, where the pointer is: Windows
        // Terminal and VS Code report a drag of the button pressed, conhost
        // and VS Code a motion with no button once it is released.
        const auto trailing_button = expected.motion == Event::Mouse::Pressed
                                         ? expected.button
                                         : Event::Mouse::None;
        for (size_t i = 0; i < events.size(); ++i) {
          const Event& event = events[i];
          INFO(event.Print());
          auto mouse = event.get_if<Event::Mouse>();
          CHECK(mouse);
          if (!mouse) {
            continue;
          }
          if (i > 0 && expected.motion != Event::Mouse::Moved &&
              mouse->motion() == Event::Mouse::Moved &&
              mouse->button() == trailing_button) {
            continue;
          }
          CHECK(mouse->button() == expected.button);
          CHECK(mouse->motion() == expected.motion);
          // urxvt reports no modifier with a motion.
          const std::string held = Held(mouse->modifier());
          CHECK((held == expected.held || held.empty()));
        }
      } else {
        REQUIRE(KeyGestures().contains(gesture));
        const auto& expected = KeyGestures().at(gesture);
        if (!events.empty() &&
            std::ranges::find(held, events.front()) != held.end()) {
          events.erase(events.begin());
        }
        for (const Event& event : events) {
          INFO(event.Print());
          CHECK(std::ranges::find(expected, event) != expected.end());
        }
        CHECK(events.size() == 1);
      }
      held.clear();
    }
  }
}
