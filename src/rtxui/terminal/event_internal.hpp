// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_TERMINAL_EVENT_INTERNAL_HPP_
#define RTXUI_TERMINAL_EVENT_INTERNAL_HPP_

#include <cstdint>
#include <variant>

#include "rtxui/event.hpp"

namespace rtxui {

// What an Event holds. The public classes only read it; the library and its
// tests build events from these, which convert to an Event:
//
//   MouseData mouse;
//   mouse.button = Event::Mouse::Left;
//   mouse.x = 3;
//   screen.Dispatch(mouse);
struct ModifierData {
  bool alt = false;
  bool ctrl = false;
  bool meta = false;
  bool shift = false;
  bool operator==(const ModifierData&) const = default;
};

namespace internal {

struct KeyboardData {
  std::uint32_t codepoint = 0;
  Event::Keyboard::Special special = Event::Keyboard::None;
  ModifierData modifier;
  Event::Keyboard::Motion motion = Event::Keyboard::Pressed;
  // Set only by TerminalInputParser::EmitPastedText, for components (e.g.
  // TextInputBase's undo grouping) that need to tell a pasted character
  // apart from one the user actually typed, even though both arrive as
  // the same kind of single-codepoint event.
  bool from_paste = false;
  bool operator==(const KeyboardData&) const = default;
  operator Event() const;  // NOLINT(google-explicit-constructor)
};

struct MouseData {
  Event::Mouse::Button button = Event::Mouse::None;
  Event::Mouse::Motion motion = Event::Mouse::Pressed;
  ModifierData modifier;
  int x = 0, y = 0;
  float sub_cell_x = 0.0f, sub_cell_y = 0.0f;
  int pixel_x = 0, pixel_y = 0;
  bool is_pixel_precise = false;
  bool operator==(const MouseData&) const = default;
  operator Event() const;  // NOLINT(google-explicit-constructor)
};

struct ResizedData {
  int width = 0, height = 0;
  bool operator==(const ResizedData&) const = default;
  operator Event() const;  // NOLINT(google-explicit-constructor)
};

struct EventData {
  std::variant<KeyboardData, MouseData, ResizedData> value;
};

}  // namespace internal

using internal::KeyboardData;
using internal::MouseData;
using internal::ResizedData;

struct EventInternals {
  static Event Make(KeyboardData data);
  static Event Make(MouseData data);
  static Event Make(ResizedData data);

  // What `event` holds, to read or to copy and change.
  static const KeyboardData& Data(const Event::Keyboard& event);
  static const MouseData& Data(const Event::Mouse& event);
  static const ResizedData& Data(const Event::Resized& event);

  static Event::Modifier Make(ModifierData data);
  static ModifierData Data(Event::Modifier modifier);
};

}  // namespace rtxui

#endif  // RTXUI_TERMINAL_EVENT_INTERNAL_HPP_
