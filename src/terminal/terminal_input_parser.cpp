// Copyright 2020 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "terminal/terminal_input_parser.hpp"

#include <cstdint>                    // for uint32_t
#include <map>
#include <memory>   // for unique_ptr, allocator
#include <optional> // for std::optional
#include <utility>  // for move
#include <vector>

#include "terminal/event.hpp"  // for Event

std::optional<Event> TerminalInputParser::ToEvent(std::string_view sequence) {
  static const auto& mapping = *new std::map<std::string, Event>{
      {"\x1B[D", Event::ArrowLeft()}, {"\x1B[C", Event::ArrowRight()}, {"\x1B[A", Event::ArrowUp()}, {"\x1B[B", Event::ArrowDown()},
      {"\x1B[1;5D", Event::ArrowLeftCtrl()}, {"\x1B[1;5C", Event::ArrowRightCtrl()}, {"\x1B[1;5A", Event::ArrowUpCtrl()}, {"\x1B[1;5B", Event::ArrowDownCtrl()},
      {"\x7F", Event::Backspace()}, {"\x1B[3~", Event::Delete()}, {"\x1B", Event::Escape()}, {"\n", Event::Return()}, {"\r", Event::Return()}, {"\t", Event::Tab()}, {"\x1B[Z", Event::TabReverse()},
      {"\x1BOP", Event::F1()}, {"\x1BOQ", Event::F2()}, {"\x1BOR", Event::F3()}, {"\x1BOS", Event::F4()}, {"\x1B[15~", Event::F5()}, {"\x1B[17~", Event::F6()}, {"\x1B[18~", Event::F7()}, {"\x1B[19~", Event::F8()}, {"\x1B[20~", Event::F9()}, {"\x1B[21~", Event::F10()}, {"\x1B[23~", Event::F11()}, {"\x1B[24~", Event::F12()},
      {"\x1B[2~", Event::Insert()}, {"\x1B[H", Event::Home()}, {"\x1B[F", Event::End()}, {"\x1B[5~", Event::PageUp()}, {"\x1B[6~", Event::PageDown()},
      {"\x1BOA", Event::ArrowUp()}, {"\x1BOB", Event::ArrowDown()}, {"\x1BOC", Event::ArrowRight()}, {"\x1BOD", Event::ArrowLeft()}, {"\x1BOH", Event::Home()}, {"\x1BOF", Event::End()},
      {"\x1B[[A", Event::F1()}, {"\x1B[[B", Event::F2()}, {"\x1B[[C", Event::F3()}, {"\x1B[[D", Event::F4()}, {"\x1B[[E", Event::F5()},
      {"\x1B[11~", Event::F1()}, {"\x1B[12~", Event::F2()}, {"\x1B[13~", Event::F3()}, {"\x1B[14~", Event::F4()},
      {"\x1BOt", Event::F5()}, {"\x1BOu", Event::F6()}, {"\x1BOv", Event::F7()}, {"\x1BOl", Event::F8()}, {"\x1BOw", Event::F9()}, {"\x1BOx", Event::F10()},
      {"\x1B[M", Event::F1()}, {"\x1B[N", Event::F2()}, {"\x1B[O", Event::F3()}, {"\x1B[P", Event::F4()}, {"\x1B[Q", Event::F5()}, {"\x1B[R", Event::F6()}, {"\x1B[S", Event::F7()}, {"\x1B[T", Event::F8()}, {"\x1B[U", Event::F9()}, {"\x1B[V", Event::F10()}, {"\x1B[W", Event::F11()}, {"\x1B[X", Event::F12()},
      {"\x01", Event::CtrlA()}, {"\x02", Event::CtrlB()}, {"\x03", Event::CtrlC()}, {"\x04", Event::CtrlD()},
      {"\x05", Event::CtrlE()}, {"\x06", Event::CtrlF()}, {"\x07", Event::CtrlG()}, {"\x08", Event::Backspace()},
      {"\x09", Event::Tab()}, {"\x0A", Event::CtrlJ()}, {"\x0B", Event::CtrlK()}, {"\x0C", Event::CtrlL()},
      {"\x0D", Event::Return()}, {"\x0E", Event::CtrlN()}, {"\x0F", Event::CtrlO()}, {"\x10", Event::CtrlP()},
      {"\x11", Event::CtrlQ()}, {"\x12", Event::CtrlR()}, {"\x13", Event::CtrlS()}, {"\x14", Event::CtrlT()},
      {"\x15", Event::CtrlU()}, {"\x16", Event::CtrlV()}, {"\x17", Event::CtrlW()}, {"\x18", Event::CtrlX()},
      {"\x19", Event::CtrlY()}, {"\x1A", Event::CtrlZ()},
      {"\x1B" "a", Event::AltA()}, {"\x1B" "b", Event::AltB()}, {"\x1B" "c", Event::AltC()}, {"\x1B" "d", Event::AltD()},
      {"\x1B" "e", Event::AltE()}, {"\x1B" "f", Event::AltF()}, {"\x1B" "g", Event::AltG()}, {"\x1B" "h", Event::AltH()},
      {"\x1B" "i", Event::AltI()}, {"\x1B" "j", Event::AltJ()}, {"\x1B" "k", Event::AltK()}, {"\x1B" "l", Event::AltL()},
      {"\x1B" "m", Event::AltM()}, {"\x1B" "n", Event::AltN()}, {"\x1B" "o", Event::AltO()}, {"\x1B" "p", Event::AltP()},
      {"\x1B" "q", Event::AltQ()}, {"\x1B" "r", Event::AltR()}, {"\x1B" "s", Event::AltS()}, {"\x1B" "t", Event::AltT()},
      {"\x1B" "u", Event::AltU()}, {"\x1B" "v", Event::AltV()}, {"\x1B" "w", Event::AltW()}, {"\x1B" "x", Event::AltX()},
      {"\x1B" "y", Event::AltY()}, {"\x1B" "z", Event::AltZ()},
      {std::string({8}), Event::Backspace()},
  };
  if (auto it = mapping.find(std::string(sequence)); it != mapping.end()) return it->second;
  return std::nullopt;
}

void TerminalInputParser::Timeout(int time) {
  timeout_ += time; if (timeout_ < 50) return;
  timeout_ = 0; if (pending_.empty()) return;
  if (auto event = ToEvent(pending_)) Send(std::move(*event)); else Send(DROP);
}
void TerminalInputParser::Add(char c) { pending_ += c; timeout_ = 0; position_ = -1; Send(Parse()); }
unsigned char TerminalInputParser::Current() { return pending_[position_]; }
bool TerminalInputParser::Eat() { position_++; return position_ < static_cast<int>(pending_.size()); }
void TerminalInputParser::Send(TerminalInputParser::Output output) {
  if (auto* event = std::get_if<Event>(&output)) { events_.push_back(std::move(*event)); pending_.clear(); return; }
  if (auto* type = std::get_if<Type>(&output)) { if (*type == DROP) pending_.clear(); return; }
}
TerminalInputParser::Output TerminalInputParser::Parse() {
  if (!Eat()) return UNCOMPLETED;
  if (Current() == '\x1B') return ParseESC();
  if (Current() < 32 || Current() == 127) { if (auto event = ToEvent(pending_)) return *event; else return DROP; }
  return ParseUTF8();
}
TerminalInputParser::Output TerminalInputParser::ParseUTF8() {
  auto head = Current();
  unsigned char selector = 0b1000'0000; unsigned char mask = selector;
  unsigned int first_zero = 8;
  for (unsigned int i = 0; i < 8; ++i) {
    mask |= selector; if (!(head & selector)) { first_zero = i; break; } selector >>= 1U;
  }
  auto value = uint32_t(head & ~mask);
  if (first_zero == 1 || first_zero >= 5) return DROP;
  for (unsigned int i = 2; i <= first_zero; ++i) {
    if (!Eat()) return UNCOMPLETED; head = Current();
    if ((head & 0b1100'0000) != 0b1000'0000) return DROP;
    value <<= 6; value += head & 0b0011'1111;
  }
  int extra_byte = 0;
  if (value <= 0x7F) extra_byte = 0; else if (value <= 0x7FF) extra_byte = 1; else if (value <= 0xFFFF) extra_byte = 2; else if (value <= 0x10FFFF) extra_byte = 3; else return DROP;
  if (extra_byte != position_) return DROP;
  return Event::Keyboard::From(value);
}
TerminalInputParser::Output TerminalInputParser::ParseESC() {
  if (!Eat()) return UNCOMPLETED;
  switch (Current()) {
    case 'P': return ParseDCS();
    case '[': return ParseCSI();
    case 'O': { if (!Eat()) return UNCOMPLETED; if (auto event = ToEvent(pending_)) return *event; else return DROP; }
    default: if (auto event = ToEvent(pending_)) return *event; else return Event::Keyboard::From((char)Current());
  }
}
TerminalInputParser::Output TerminalInputParser::ParseDCS() {
  while (true) {
    if (!Eat()) return UNCOMPLETED;
    if (Current() == '\x1B') { if (!Eat()) return UNCOMPLETED; if (Current() == '\\') {
        if (pending_.size() == 10 && pending_[2] == '1' && pending_[3] == '$' && pending_[4] == 'r') return Event::CursorShape{pending_[5] - '0'};
        if (auto event = ToEvent(pending_)) return *event; else return DROP;
    } }
  }
}
TerminalInputParser::Output TerminalInputParser::ParseCSI() {
  bool altered = false; int argument = 0; std::vector<int> arguments;
  while (true) {
    if (!Eat()) return UNCOMPLETED;
    if (Current() == '<') { altered = true; continue; }
    if (Current() == '[') { if (auto event = ToEvent(pending_)) return *event; else continue; }
    if (Current() >= '0' && Current() <= '9') { argument *= 10; argument += Current() - '0'; continue; }
    if (Current() == ';') { arguments.push_back(argument); argument = 0; continue; }
    if (Current() >= '@' && Current() <= '~' && Current() != '<' && Current() != '[') {
      arguments.push_back(argument);
      switch (Current()) {
        case 'M': return ParseMouse(altered, true, std::move(arguments));
        case 'm': return ParseMouse(altered, false, std::move(arguments));
        case 'R': return ParseCursorPosition(std::move(arguments));
        default: if (auto event = ToEvent(pending_)) return *event; else return DROP;
      }
    }
    if (Current() == '\x1B') { if (auto event = ToEvent(pending_)) return *event; else return DROP; }
  }
}
TerminalInputParser::Output TerminalInputParser::ParseMouse(bool altered, bool pressed, std::vector<int> arguments) {
  if (arguments.size() != 3) { if (auto event = ToEvent(pending_)) return *event; else return DROP; }
  Event::Mouse mouse; const int button = arguments[0];
  mouse.motion = (button & 32) ? Event::Mouse::Moved : (pressed ? Event::Mouse::Pressed : Event::Mouse::Released);
  mouse.button = (button & 64) ? Event::Mouse::Button(Event::Mouse::WheelUp + (button & 3)) : Event::Mouse::Button(button & 3);
  mouse.modifier.shift = (button & 4) != 0; mouse.modifier.meta = (button & 8) != 0; mouse.modifier.ctrl = (button & 16) != 0;
  mouse.x = arguments[1]; mouse.y = arguments[2]; return mouse;
}
TerminalInputParser::Output TerminalInputParser::ParseCursorPosition(std::vector<int> arguments) {
  if (arguments.size() != 2) { if (auto event = ToEvent(pending_)) return *event; else return DROP; }
  Event::CursorPosition cursor_position; cursor_position.y = arguments[0]; cursor_position.x = arguments[1]; return cursor_position;
}
void TerminalInputParser::AddEvent(Event event) { events_.push_back(std::move(event)); }
std::optional<Event> TerminalInputParser::GetEvent() {
  if (events_.empty()) return std::nullopt;
  Event event = events_.front(); events_.erase(events_.begin()); return event;
}
