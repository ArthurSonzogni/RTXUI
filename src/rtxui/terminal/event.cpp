// Copyright 2020 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/terminal/event.hpp"

#include <map>
#include <string>
#include <utility>
#include <variant>

#include "rtxui/core/string.hpp"

// Disable warning for shadowing variable
#ifdef __clang__
#pragma clang diagnostic ignored "-Wshadow"
#elif __GNUC__
#pragma GCC diagnostic ignored "-Wshadow"
#endif

// static
Event::Keyboard Event::Keyboard::From(std::uint32_t cp) {
  Keyboard k; k.codepoint = cp; return k;
}

// static
Event::Keyboard Event::Keyboard::From(char c) {
  return From(static_cast<std::uint32_t>(c));
}

// static
Event::Keyboard Event::Keyboard::From(std::string_view str) {
  uint32_t cp = 0; size_t end = 0; EatCodePoint(str, 0, &end, &cp); return From(cp);
}

std::string Event::Modifier::Print() const {
  std::string out;
  if (alt) out += "alt";
  if (ctrl) { if (!out.empty()) out += "|"; out += "ctrl"; }
  if (meta) { if (!out.empty()) out += "|"; out += "meta"; }
  if (shift) { if (!out.empty()) out += "|"; out += "shift"; }
  return out;
}

std::string Event::Keyboard::Print() const {
  if (special != Special::None) {
    static const auto& special_string = *new std::map<Special, std::string>{
        {Special::ArrowLeft, "ArrowLeft"}, {Special::ArrowRight, "ArrowRight"},
        {Special::ArrowUp, "ArrowUp"}, {Special::ArrowDown, "ArrowDown"},
        {Special::Backspace, "Backspace"}, {Special::Delete, "Delete"},
        {Special::Escape, "Escape"}, {Special::Return, "Return"},
        {Special::Tab, "Tab"}, {Special::TabReverse, "TabReverse"},
        {Special::Insert, "Insert"}, {Special::Home, "Home"},
        {Special::End, "End"}, {Special::PageUp, "PageUp"},
        {Special::PageDown, "PageDown"}, {Special::F1, "F1"},
        {Special::F2, "F2"}, {Special::F3, "F3"}, {Special::F4, "F4"},
        {Special::F5, "F5"}, {Special::F6, "F6"}, {Special::F7, "F7"},
        {Special::F8, "F8"}, {Special::F9, "F9"}, {Special::F10, "F10"},
        {Special::F11, "F11"}, {Special::F12, "F12"},
    };
    return "Keyboard(" + ((std::map<Special, std::string>&)special_string)[special] + ")";
  }
  std::string out = "Keyboard(" + CodePointToString(codepoint);
  std::string mod = modifier.Print();
  if (!mod.empty()) out += " " + mod;
  out += ")";
  return out;
}

std::string Event::Mouse::Print() const {
  static const auto& button_string = *new std::map<Mouse::Button, const char*>{
      {Mouse::Button::Left, "Left"}, {Mouse::Button::Middle, "Middle"},
      {Mouse::Button::Right, "Right"}, {Mouse::Button::WheelUp, "WheelUp"},
      {Mouse::Button::WheelDown, "WheelDown"}, {Mouse::Button::None, "None"},
      {Mouse::Button::WheelLeft, "WheelLeft"}, {Mouse::Button::WheelRight, "WheelRight"},
  };
  static const auto& motion_string = *new std::map<Mouse::Motion, const char*>{
      {Mouse::Motion::Pressed, "Pressed"}, {Mouse::Motion::Released, "Released"},
      {Mouse::Motion::Moved, "Moved"},
  };
  std::string out = "Mouse(" + std::string(((std::map<Mouse::Button, const char*>&)button_string)[button]);
  out += ", " + std::string(((std::map<Mouse::Motion, const char*>&)motion_string)[motion]);
  out += ", x=" + std::to_string(x) + ", y=" + std::to_string(y);
  std::string mod = modifier.Print();
  if (!mod.empty()) out += ", " + mod;
  out += ")";
  return out;
}

std::string Event::Resized::Print() const {
  return "Event::Resized(" + std::to_string(width) + ", " + std::to_string(height) + ")";
}
std::string Event::CursorShape::Print() const {
  return "Event::CursorShape(" + std::to_string(shape) + ")";
}
std::string Event::CursorPosition::Print() const {
  return "Event::CursorPosition(" + std::to_string(x) + ", " + std::to_string(y) + ")";
}

std::string Event::Print() const {
  return std::visit([](const auto& data) { return data.Print(); }, data_);
}

#define RTXUI_IMPL_EVENT(NAME, ...) \
  const Event& Event::NAME() { \
    static const Event& event = *new Event(Event::Keyboard{ __VA_ARGS__ }); \
    return event; \
  }

RTXUI_IMPL_EVENT(ArrowLeft, .special = Event::Keyboard::ArrowLeft)
RTXUI_IMPL_EVENT(ArrowRight, .special = Event::Keyboard::ArrowRight)
RTXUI_IMPL_EVENT(ArrowUp, .special = Event::Keyboard::ArrowUp)
RTXUI_IMPL_EVENT(ArrowDown, .special = Event::Keyboard::ArrowDown)
RTXUI_IMPL_EVENT(ArrowLeftCtrl, .special = Event::Keyboard::ArrowLeft, .modifier = {.ctrl = true})
RTXUI_IMPL_EVENT(ArrowRightCtrl, .special = Event::Keyboard::ArrowRight, .modifier = {.ctrl = true})
RTXUI_IMPL_EVENT(ArrowUpCtrl, .special = Event::Keyboard::ArrowUp, .modifier = {.ctrl = true})
RTXUI_IMPL_EVENT(ArrowDownCtrl, .special = Event::Keyboard::ArrowDown, .modifier = {.ctrl = true})
RTXUI_IMPL_EVENT(ArrowLeftAlt, .special = Event::Keyboard::ArrowLeft, .modifier = {.alt = true})
RTXUI_IMPL_EVENT(ArrowRightAlt, .special = Event::Keyboard::ArrowRight, .modifier = {.alt = true})
RTXUI_IMPL_EVENT(ArrowUpAlt, .special = Event::Keyboard::ArrowUp, .modifier = {.alt = true})
RTXUI_IMPL_EVENT(ArrowDownAlt, .special = Event::Keyboard::ArrowDown, .modifier = {.alt = true})
RTXUI_IMPL_EVENT(Backspace, .special = Event::Keyboard::Backspace)
RTXUI_IMPL_EVENT(BackspaceCtrl, .special = Event::Keyboard::Backspace, .modifier = {.ctrl = true})
RTXUI_IMPL_EVENT(BackspaceAlt, .special = Event::Keyboard::Backspace, .modifier = {.alt = true})
RTXUI_IMPL_EVENT(Delete, .special = Event::Keyboard::Delete)
RTXUI_IMPL_EVENT(DeleteCtrl, .special = Event::Keyboard::Delete, .modifier = {.ctrl = true})
RTXUI_IMPL_EVENT(DeleteAlt, .special = Event::Keyboard::Delete, .modifier = {.alt = true})
RTXUI_IMPL_EVENT(Escape, .special = Event::Keyboard::Escape)
RTXUI_IMPL_EVENT(Return, .special = Event::Keyboard::Return)
RTXUI_IMPL_EVENT(Tab, .special = Event::Keyboard::Tab)
RTXUI_IMPL_EVENT(TabReverse, .special = Event::Keyboard::TabReverse)
RTXUI_IMPL_EVENT(F1, .special = Event::Keyboard::F1)
RTXUI_IMPL_EVENT(F2, .special = Event::Keyboard::F2)
RTXUI_IMPL_EVENT(F3, .special = Event::Keyboard::F3)
RTXUI_IMPL_EVENT(F4, .special = Event::Keyboard::F4)
RTXUI_IMPL_EVENT(F5, .special = Event::Keyboard::F5)
RTXUI_IMPL_EVENT(F6, .special = Event::Keyboard::F6)
RTXUI_IMPL_EVENT(F7, .special = Event::Keyboard::F7)
RTXUI_IMPL_EVENT(F8, .special = Event::Keyboard::F8)
RTXUI_IMPL_EVENT(F9, .special = Event::Keyboard::F9)
RTXUI_IMPL_EVENT(F10, .special = Event::Keyboard::F10)
RTXUI_IMPL_EVENT(F11, .special = Event::Keyboard::F11)
RTXUI_IMPL_EVENT(F12, .special = Event::Keyboard::F12)
RTXUI_IMPL_EVENT(Insert, .special = Event::Keyboard::Insert)
RTXUI_IMPL_EVENT(Home, .special = Event::Keyboard::Home)
RTXUI_IMPL_EVENT(End, .special = Event::Keyboard::End)
RTXUI_IMPL_EVENT(PageUp, .special = Event::Keyboard::PageUp)
RTXUI_IMPL_EVENT(PageDown, .special = Event::Keyboard::PageDown)

#define RTXUI_IMPL_LETTER(L, UC, CP) \
  RTXUI_IMPL_EVENT(L, .codepoint = CP) \
  RTXUI_IMPL_EVENT(UC, .codepoint = CP, .modifier = {.shift = true}) \
  RTXUI_IMPL_EVENT(Ctrl##UC, .codepoint = CP, .modifier = {.ctrl = true}) \
  RTXUI_IMPL_EVENT(Alt##UC, .codepoint = CP, .modifier = {.alt = true}) \
  RTXUI_IMPL_EVENT(CtrlAlt##UC, .codepoint = CP, .modifier = {.alt = true, .ctrl = true})

RTXUI_IMPL_LETTER(a, A, 'a') RTXUI_IMPL_LETTER(b, B, 'b') RTXUI_IMPL_LETTER(c, C, 'c') RTXUI_IMPL_LETTER(d, D, 'd')
RTXUI_IMPL_LETTER(e, E, 'e') RTXUI_IMPL_LETTER(f, F, 'f') RTXUI_IMPL_LETTER(g, G, 'g') RTXUI_IMPL_LETTER(h, H, 'h')
RTXUI_IMPL_LETTER(i, I, 'i') RTXUI_IMPL_LETTER(j, J, 'j') RTXUI_IMPL_LETTER(k, K, 'k') RTXUI_IMPL_LETTER(l, L, 'l')
RTXUI_IMPL_LETTER(m, M, 'm') RTXUI_IMPL_LETTER(n, N, 'n') RTXUI_IMPL_LETTER(o, O, 'o') RTXUI_IMPL_LETTER(p, P, 'p')
RTXUI_IMPL_LETTER(q, Q, 'q') RTXUI_IMPL_LETTER(r, R, 'r') RTXUI_IMPL_LETTER(s, S, 's') RTXUI_IMPL_LETTER(t, T, 't')
RTXUI_IMPL_LETTER(u, U, 'u') RTXUI_IMPL_LETTER(v, V, 'v') RTXUI_IMPL_LETTER(w, W, 'w') RTXUI_IMPL_LETTER(x, X, 'x')
RTXUI_IMPL_LETTER(y, Y, 'y') RTXUI_IMPL_LETTER(z, Z, 'z')
