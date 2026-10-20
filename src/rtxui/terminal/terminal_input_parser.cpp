// Copyright 2020 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/terminal/terminal_input_parser.hpp"

#include <algorithm>  // for std::min
#include <charconv>   // for from_chars
#include <cstddef>
#include <cstdint>  // for uint32_t
#include <map>
#include <optional>  // for std::optional
#include <string>
#include <string_view>
#include <system_error>
#include <utility>  // for move
#include <variant>
#include <vector>

#include "rtxui/base/string.hpp"  // for EatCodePoint
#include "rtxui/color.hpp"
#include "rtxui/event.hpp"
#include "rtxui/terminal/event_internal.hpp"  // for Event

namespace rtxui {

std::optional<Event> TerminalInputParser::ToEvent(std::string_view sequence) {
  static const auto& mapping = *new std::map<std::string, Event>{
      {"\x1B[D", Event::ArrowLeft()},
      {"\x1B[C", Event::ArrowRight()},
      {"\x1B[A", Event::ArrowUp()},
      {"\x1B[B", Event::ArrowDown()},
      {"\x1B[1;5D", Event::ArrowLeftCtrl()},
      {"\x1B[1;5C", Event::ArrowRightCtrl()},
      {"\x1B[1;5A", Event::ArrowUpCtrl()},
      {"\x1B[1;5B", Event::ArrowDownCtrl()},
      {"\x1B[1;3D", Event::ArrowLeftAlt()},
      {"\x1B[1;3C", Event::ArrowRightAlt()},
      {"\x1B[1;3A", Event::ArrowUpAlt()},
      {"\x1B[1;3B", Event::ArrowDownAlt()},
      {"\x7F", Event::Backspace()},
      {"\x1B\x7F", Event::BackspaceAlt()},
      {"\x1B\x08", Event::BackspaceAlt()},
      {"\x1B[127;5u", Event::BackspaceCtrl()},
      {"\x1B[8;5u", Event::BackspaceCtrl()},
      {"\x1B[127;3u", Event::BackspaceAlt()},
      {"\x1B[8;3u", Event::BackspaceAlt()},
      {"\x1B[3~", Event::Delete()},
      {"\x1B[3;5~", Event::DeleteCtrl()},
      {"\x1B[3;3~", Event::DeleteAlt()},
      {"\x1B", Event::Escape()},
      {"\n", Event::Return()},
      {"\r", Event::Return()},
      {"\t", Event::Tab()},
      {"\x1B[Z", Event::TabReverse()},
      {"\x1BOP", Event::F1()},
      {"\x1BOQ", Event::F2()},
      {"\x1BOR", Event::F3()},
      {"\x1BOS", Event::F4()},
      {"\x1B[15~", Event::F5()},
      {"\x1B[17~", Event::F6()},
      {"\x1B[18~", Event::F7()},
      {"\x1B[19~", Event::F8()},
      {"\x1B[20~", Event::F9()},
      {"\x1B[21~", Event::F10()},
      {"\x1B[23~", Event::F11()},
      {"\x1B[24~", Event::F12()},
      {"\x1B[2~", Event::Insert()},
      {"\x1B[H", Event::Home()},
      {"\x1B[F", Event::End()},
      {"\x1B[5~", Event::PageUp()},
      {"\x1B[6~", Event::PageDown()},
      {"\x1BOA", Event::ArrowUp()},
      {"\x1BOB", Event::ArrowDown()},
      {"\x1BOC", Event::ArrowRight()},
      {"\x1BOD", Event::ArrowLeft()},
      {"\x1BOH", Event::Home()},
      {"\x1BOF", Event::End()},
      // rxvt: Ctrl+arrows, Home and End, and Alt as an Escape before a key.
      {"\x1BOa", Event::ArrowUpCtrl()},
      {"\x1BOb", Event::ArrowDownCtrl()},
      {"\x1BOc", Event::ArrowRightCtrl()},
      {"\x1BOd", Event::ArrowLeftCtrl()},
      {"\x1B[7~", Event::Home()},
      {"\x1B[8~", Event::End()},
      // tmux, screen and the Linux console.
      {"\x1B[1~", Event::Home()},
      {"\x1B[4~", Event::End()},
      {"\x1B\x1B[A", Event::ArrowUpAlt()},
      {"\x1B\x1B[B", Event::ArrowDownAlt()},
      {"\x1B\x1B[C", Event::ArrowRightAlt()},
      {"\x1B\x1B[D", Event::ArrowLeftAlt()},
      {"\x1B\x1B", Event::Escape()},
      {"\x1B[[A", Event::F1()},
      {"\x1B[[B", Event::F2()},
      {"\x1B[[C", Event::F3()},
      {"\x1B[[D", Event::F4()},
      {"\x1B[[E", Event::F5()},
      {"\x1B[11~", Event::F1()},
      {"\x1B[12~", Event::F2()},
      {"\x1B[13~", Event::F3()},
      {"\x1B[14~", Event::F4()},
      {"\x1BOt", Event::F5()},
      {"\x1BOu", Event::F6()},
      {"\x1BOv", Event::F7()},
      {"\x1BOl", Event::F8()},
      {"\x1BOw", Event::F9()},
      {"\x1BOx", Event::F10()},
      {"\x1B[M", Event::F1()},
      {"\x1B[N", Event::F2()},
      {"\x1B[O", Event::F3()},
      {"\x1B[P", Event::F4()},
      {"\x1B[Q", Event::F5()},
      {"\x1B[R", Event::F6()},
      {"\x1B[S", Event::F7()},
      {"\x1B[T", Event::F8()},
      {"\x1B[U", Event::F9()},
      {"\x1B[V", Event::F10()},
      {"\x1B[W", Event::F11()},
      {"\x1B[X", Event::F12()},
      {"\x01", Event::CtrlA()},
      {"\x02", Event::CtrlB()},
      {"\x03", Event::CtrlC()},
      {"\x04", Event::CtrlD()},
      {"\x05", Event::CtrlE()},
      {"\x06", Event::CtrlF()},
      {"\x07", Event::CtrlG()},
      {"\x08", Event::Backspace()},
      {"\x09", Event::Tab()},
      {"\x0A", Event::CtrlJ()},
      {"\x0B", Event::CtrlK()},
      {"\x0C", Event::CtrlL()},
      {"\x0D", Event::Return()},
      {"\x0E", Event::CtrlN()},
      {"\x0F", Event::CtrlO()},
      {"\x10", Event::CtrlP()},
      {"\x11", Event::CtrlQ()},
      {"\x12", Event::CtrlR()},
      {"\x13", Event::CtrlS()},
      {"\x14", Event::CtrlT()},
      {"\x15", Event::CtrlU()},
      {"\x16", Event::CtrlV()},
      {"\x17", Event::CtrlW()},
      {"\x18", Event::CtrlX()},
      {"\x19", Event::CtrlY()},
      {"\x1A", Event::CtrlZ()},
      {"\x1B"
       "a",
       Event::AltA()},
      {"\x1B"
       "b",
       Event::AltB()},
      {"\x1B"
       "c",
       Event::AltC()},
      {"\x1B"
       "d",
       Event::AltD()},
      {"\x1B"
       "e",
       Event::AltE()},
      {"\x1B"
       "f",
       Event::AltF()},
      {"\x1B"
       "g",
       Event::AltG()},
      {"\x1B"
       "h",
       Event::AltH()},
      {"\x1B"
       "i",
       Event::AltI()},
      {"\x1B"
       "j",
       Event::AltJ()},
      {"\x1B"
       "k",
       Event::AltK()},
      {"\x1B"
       "l",
       Event::AltL()},
      {"\x1B"
       "m",
       Event::AltM()},
      {"\x1B"
       "n",
       Event::AltN()},
      {"\x1B"
       "o",
       Event::AltO()},
      {"\x1B"
       "p",
       Event::AltP()},
      {"\x1B"
       "q",
       Event::AltQ()},
      {"\x1B"
       "r",
       Event::AltR()},
      {"\x1B"
       "s",
       Event::AltS()},
      {"\x1B"
       "t",
       Event::AltT()},
      {"\x1B"
       "u",
       Event::AltU()},
      {"\x1B"
       "v",
       Event::AltV()},
      {"\x1B"
       "w",
       Event::AltW()},
      {"\x1B"
       "x",
       Event::AltX()},
      {"\x1B"
       "y",
       Event::AltY()},
      {"\x1B"
       "z",
       Event::AltZ()},
      {std::string({8}), Event::Backspace()},
  };
  if (auto it = mapping.find(std::string(sequence)); it != mapping.end()) {
    return it->second;
  }
  return std::nullopt;
}

void TerminalInputParser::Timeout(int time) {
  timeout_ += time;
  if (timeout_ < 50) {
    return;
  }
  timeout_ = 0;
  if (pending_.empty()) {
    return;
  }
  if (auto event = ToEvent(pending_)) {
    Send(std::move(*event));
  } else {
    Send(DROP);
  }
}
void TerminalInputParser::Add(char c) {
  if (in_bracketed_paste_) {
    paste_buffer_ += c;
    constexpr std::string_view kPasteEnd = "\x1B[201~";
    if (paste_buffer_.size() >= kPasteEnd.size() &&
        std::string_view(paste_buffer_)
                .substr(paste_buffer_.size() - kPasteEnd.size()) == kPasteEnd) {
      EmitPastedText(std::string_view(paste_buffer_)
                         .substr(0, paste_buffer_.size() - kPasteEnd.size()));
      paste_buffer_.clear();
      in_bracketed_paste_ = false;
    }
    return;
  }
  pending_ += c;
  timeout_ = 0;
  position_ = -1;
  Send(Parse());
}
void TerminalInputParser::EmitPastedText(std::string_view text) {
  size_t i = 0;
  while (i < text.size()) {
    if (text[i] == '\n' || text[i] == '\r') {
      if (text[i] == '\r' && i + 1 < text.size() && text[i + 1] == '\n') {
        ++i;
      }
      // A plain '\n' character event, not Event::Return(): the latter goes
      // through the "user pressed Enter" handler, which carries over the
      // current line's indentation. Pasted text already has its own
      // indentation, so that would double it on every line.
      KeyboardData newline_kb;
      newline_kb.codepoint = '\n';
      newline_kb.from_paste = true;
      events_.emplace_back(newline_kb);
      ++i;
      continue;
    }
    size_t end = i;
    uint32_t codepoint = 0;
    if (EatCodePoint(text, i, &end, &codepoint)) {
      KeyboardData kb;
      kb.codepoint = codepoint;
      kb.from_paste = true;
      events_.emplace_back(kb);
      i = end;
    } else {
      ++i;
    }
  }
}
unsigned char TerminalInputParser::Current() {
  return pending_[position_];
}
bool TerminalInputParser::Eat() {
  position_++;
  return position_ < static_cast<int>(pending_.size());
}
void TerminalInputParser::Send(TerminalInputParser::Output output) {
  if (auto* event = std::get_if<Event>(&output)) {
    events_.push_back(std::move(*event));
    pending_.clear();
    return;
  }
  if (auto* type = std::get_if<Type>(&output)) {
    if (*type == DROP) {
      pending_.clear();
    }
    return;
  }
}
TerminalInputParser::Output TerminalInputParser::Parse() {
  if (!Eat()) {
    return UNCOMPLETED;
  }
  if (Current() == '\x1B') {
    return ParseESC();
  }
  if (Current() < 32 || Current() == 127) {
    if (auto event = ToEvent(pending_)) {
      return *event;
    } else {
      return DROP;
    }
  }
  return ParseUTF8();
}
TerminalInputParser::Output TerminalInputParser::ParseUTF8() {
  auto head = Current();
  unsigned char selector = 0b1000'0000;
  unsigned char mask = selector;
  unsigned int first_zero = 8;
  for (unsigned int i = 0; i < 8; ++i) {
    mask |= selector;
    if (!(head & selector)) {
      first_zero = i;
      break;
    }
    selector >>= 1U;
  }
  auto value = uint32_t(head & ~mask);
  if (first_zero == 1 || first_zero >= 5) {
    return DROP;
  }
  for (unsigned int i = 2; i <= first_zero; ++i) {
    if (!Eat()) {
      return UNCOMPLETED;
    }
    head = Current();
    if ((head & 0b1100'0000) != 0b1000'0000) {
      return DROP;
    }
    value <<= 6;
    value += head & 0b0011'1111;
  }
  int extra_byte = 0;
  if (value <= 0x7F) {
    extra_byte = 0;
  } else if (value <= 0x7FF) {
    extra_byte = 1;
  } else if (value <= 0xFFFF) {
    extra_byte = 2;
  } else if (value <= 0x10FFFF) {
    extra_byte = 3;
  } else {
    return DROP;
  }
  if (extra_byte != position_) {
    return DROP;
  }
  return Event::Keyboard::From(value);
}
TerminalInputParser::Output TerminalInputParser::ParseESC() {
  if (!Eat()) {
    return UNCOMPLETED;
  }
  switch (Current()) {
    case 'P':
      return ParseDCS();
    case ']':
      return ParseOSC();
    case '[':
      return ParseCSI();
    case '\x1B': {
      // A third Escape in a row: no Alt+key starts with two, so the first is
      // an Escape of its own. Taking it now keeps a run of Escapes from
      // piling up, each one a level of recursion deeper than the last.
      if (position_ + 1 < static_cast<int>(pending_.size()) &&
          pending_[position_ + 1] == '\x1B') {
        events_.push_back(Event::Escape());
        pending_.erase(0, 1);
        position_ = -1;
        return Parse();
      }
      // rxvt sends Alt+key as an Escape before the key's own sequence. screen
      // holds an Escape back until the next key, and sends both at once: when
      // the two make no Alt+key, they are an Escape, then that key.
      Output output = ParseESC();
      if (auto* type = std::get_if<Type>(&output); type && *type == DROP) {
        events_.push_back(Event::Escape());
        pending_.erase(0, 1);
        position_ = -1;
        return Parse();
      }
      return output;
    }
    case 'O': {
      if (!Eat()) {
        return UNCOMPLETED;
      }
      if (auto event = ToEvent(pending_)) {
        return *event;
      } else {
        return DROP;
      }
    }
    default:
      if (auto event = ToEvent(pending_)) {
        return *event;
      } else {
        return Event::Keyboard::From((char)Current());
      }
  }
}
// Operating System Command: ESC ] ... terminated by BEL or by ST (ESC \).
//
namespace {
// One channel of an X11 color spec: 1 to 4 hex digits, scaled to 0..255.
std::optional<uint8_t> ParseColorChannel(std::string_view hex) {
  if (hex.empty() || hex.size() > 4) {
    return std::nullopt;
  }
  unsigned int value = 0;
  auto [ptr, ec] = std::from_chars(hex.data(), hex.data() + hex.size(), value,
                                   /*base=*/16);
  if (ec != std::errc() || ptr != hex.data() + hex.size()) {
    return std::nullopt;
  }
  const unsigned int max = (1U << (4 * hex.size())) - 1;
  return static_cast<uint8_t>((value * 255 + max / 2) / max);
}

// The color in the body of an OSC 10 or OSC 11 reply, the terminal's
// foreground or background color: `<code>;rgb:RRRR/GGGG/BBBB`.
std::optional<Color> ParseColorReply(std::string_view body,
                                     std::string_view code) {
  if (!body.starts_with(code) ||
      !body.substr(code.size()).starts_with(";rgb:")) {
    return std::nullopt;
  }
  body.remove_prefix(code.size() + 5);
  const size_t first = body.find('/');
  const size_t second = body.find('/', first + 1);
  if (first == std::string_view::npos || second == std::string_view::npos) {
    return std::nullopt;
  }
  auto r = ParseColorChannel(body.substr(0, first));
  auto g = ParseColorChannel(body.substr(first + 1, second - first - 1));
  auto b = ParseColorChannel(body.substr(second + 1));
  if (!r || !g || !b) {
    return std::nullopt;
  }
  return Color::RGB(*r, *g, *b);
}
}  // namespace

// These are replies to queries made of the terminal -- its background color,
// the clipboard, the window title. They have to be recognised whatever they
// answer: without this they fell through to the "some escape sequence we do
// not know" path and were handed to the application one byte at a time, so a
// terminal answering a question typed its answer into whatever had focus.
// The background color is kept for the Screen, which asks for it.
TerminalInputParser::Output TerminalInputParser::ParseOSC() {
  while (true) {
    if (!Eat()) {
      return UNCOMPLETED;
    }
    size_t terminator = 0;
    if (Current() == '\x07') {  // BEL
      terminator = 1;
    } else if (Current() == '\x1B') {
      if (!Eat()) {
        return UNCOMPLETED;
      }
      if (Current() == '\\') {  // ST
        terminator = 2;
      }
    }
    if (terminator != 0) {
      // `pending_` holds the whole sequence, from its "\x1B]".
      const std::string_view sequence(pending_);
      const std::string_view body =
          sequence.substr(2, sequence.size() - 2 - terminator);
      if (auto color = ParseColorReply(body, "11")) {
        background_color_ = color;
      }
      if (auto color = ParseColorReply(body, "10")) {
        foreground_color_ = color;
      }
      return DROP;
    }
  }
}

TerminalInputParser::Output TerminalInputParser::ParseDCS() {
  while (true) {
    if (!Eat()) {
      return UNCOMPLETED;
    }
    if (Current() == '\x1B') {
      if (!Eat()) {
        return UNCOMPLETED;
      }
      if (Current() == '\\') {
        // A terminal's reply to a cursor shape query (DECRQSS). RTXUI
        // never asks, so one can only be stray.
        if (pending_.size() == 10 && pending_[2] == '1' && pending_[3] == '$' &&
            pending_[4] == 'r') {
          return DROP;
        }
        if (auto event = ToEvent(pending_)) {
          return *event;
        } else {
          return DROP;
        }
      }
    }
  }
}
TerminalInputParser::Output TerminalInputParser::ParseCSI() {
  bool altered = false;
  int argument = 0;
  std::vector<int> arguments;
  while (true) {
    if (!Eat()) {
      return UNCOMPLETED;
    }
    if (Current() == '<') {
      altered = true;
      continue;
    }
    if (Current() == '[') {
      if (auto event = ToEvent(pending_)) {
        return *event;
      } else {
        continue;
      }
    }
    if (Current() >= '0' && Current() <= '9') {
      // Saturating. These bytes arrive from outside the process -- a terminal
      // reply, or anything else with the tty open -- so a long enough run of
      // digits must not be allowed to overflow, which is undefined behaviour
      // rather than a wrapped parameter. A CSI parameter is a coordinate or a
      // count, so the cap is already far outside any terminal.
      constexpr int kMaxArgument = 1000000;
      if (argument < kMaxArgument) {
        argument = argument * 10 + (Current() - '0');
        argument = std::min(argument, kMaxArgument);
      }
      continue;
    }
    if (Current() == ';') {
      arguments.push_back(argument);
      argument = 0;
      continue;
    }
    if (Current() >= '@' && Current() <= '~' && Current() != '<' &&
        Current() != '[') {
      arguments.push_back(argument);
      switch (Current()) {
        case 'M':
          if (position_ == 2) {  // `ESC[M`, with no parameters.
            return ParseX10Mouse();
          }
          return ParseMouse(altered, true, std::move(arguments));
        case 'm':
          return ParseMouse(altered, false, std::move(arguments));
        case 'R':
          return ParseCursorPosition(std::move(arguments));
        case 'y':
          if (pending_.starts_with("\x1B[?") &&
              pending_[pending_.size() - 2] == '$') {
            return ParseModeReport(std::move(arguments));
          }
          [[fallthrough]];
        default:
          if (pending_ == "\x1B[200~") {
            in_bracketed_paste_ = true;
            return DROP;
          }
          if (auto event = ToEvent(pending_)) {
            return *event;
          } else {
            return DROP;
          }
      }
    }
    if (Current() == '\x1B') {
      if (auto event = ToEvent(pending_)) {
        return *event;
      } else {
        return DROP;
      }
    }
  }
}
TerminalInputParser::Output TerminalInputParser::ParseMouse(
    bool /*altered*/,
    bool pressed,
    std::vector<int> arguments) {
  if (arguments.size() != 3) {
    if (auto event = ToEvent(pending_)) {
      return *event;
    } else {
      return DROP;
    }
  }
  MouseData mouse;
  int button = arguments[0];
  // urxvt encodes "no button" as -1 rather than 3, so a motion with no button
  // held, before any press, arrives as 32 - 1 instead of 32 + 3.
  if (button == 31) {
    button = 35;
  }
  mouse.motion = (button & 32) ? Event::Mouse::Moved
                               : (pressed ? Event::Mouse::Pressed
                                          : Event::Mouse::Released);
  mouse.button =
      (button & 64) ? Event::Mouse::Button(Event::Mouse::WheelUp + (button & 3))
                    : Event::Mouse::Button(button & 3);
  // urxvt reports a motion after the wheel turned as a drag of the wheel,
  // which nothing can hold.
  if ((button & 64) && mouse.motion == Event::Mouse::Moved) {
    mouse.button = Event::Mouse::None;
  }
  if (button < 64 && (button & 3) != 3) {
    const int bit = 1 << (button & 3);
    if (mouse.motion == Event::Mouse::Pressed) {
      released_buttons_ &= ~bit;
    } else if (mouse.motion == Event::Mouse::Released) {
      released_buttons_ |= bit;
    } else if (released_buttons_ & bit) {
      mouse.button = Event::Mouse::None;
    }
  }
  mouse.modifier.shift = (button & 4) != 0;
  mouse.modifier.meta = (button & 8) != 0;
  mouse.modifier.ctrl = (button & 16) != 0;
  mouse.x = arguments[1];
  mouse.y = arguments[2];
  return Event(mouse);
}
// A terminal that does not know the SGR encoding (1006) reports the mouse as
// `ESC[M` and three bytes: the button, the column and the row, each plus 32.
// A release names no button: it is the last one pressed. Some consoles send
// `ESC[M` alone for F1, which Timeout() still reads.
TerminalInputParser::Output TerminalInputParser::ParseX10Mouse() {
  int bytes[3] = {};
  for (int& byte : bytes) {
    if (!Eat()) {
      return UNCOMPLETED;
    }
    byte = Current() - 32;
  }
  int button = bytes[0];
  const bool pressed = (button & 3) != 3 || (button & (32 | 64)) != 0;
  if (pressed && (button & 32) == 0 && (button & 64) == 0) {
    x10_button_ = button & 3;
  }
  if (!pressed) {
    button = (button & ~3) | x10_button_;
  }
  return ParseMouse(false, pressed, {button, bytes[1], bytes[2]});
}
// `ESC[row;colR` is a terminal's reply to a cursor position query, which an
// inline screen makes to find where it is drawn. Not an Event: it answers the
// library, not the application.
TerminalInputParser::Output TerminalInputParser::ParseCursorPosition(
    std::vector<int> arguments) {
  if (arguments.size() != 2) {
    if (auto event = ToEvent(pending_)) {
      return *event;
    }
    return DROP;
  }
  cursor_positions_.emplace_back(std::max(arguments[1], 1) - 1,
                                 std::max(arguments[0], 1) - 1);
  return DROP;
}
// `ESC[?mode;value$y` is a terminal's reply to a DECRQM query: 1 or 3 when
// the mode is set, 2 or 4 when reset, 0 when the terminal does not know it.
// Not an Event either.
TerminalInputParser::Output TerminalInputParser::ParseModeReport(
    std::vector<int> arguments) {
  if (arguments.size() == 2 && arguments[0] == 1016) {
    pixel_mouse_ = arguments[1] == 1 || arguments[1] == 3;
  }
  return DROP;
}
void TerminalInputParser::AddEvent(Event event) {
  events_.push_back(std::move(event));
}
std::optional<Event> TerminalInputParser::GetEvent() {
  if (events_.empty()) {
    return std::nullopt;
  }
  Event event = events_.front();
  events_.erase(events_.begin());
  return event;
}

}  // namespace rtxui
