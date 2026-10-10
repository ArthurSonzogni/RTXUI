// Copyright 2020 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_TERMINAL_TERMINAL_INPUT_PARSER
#define RTXUI_TERMINAL_TERMINAL_INPUT_PARSER

#include <functional>
#include <optional>  // for std::optional
#include <string>    // for string
#include <utility>   // for exchange
#include <vector>    // for vector

#include "rtxui/color.hpp"  // for Color
#include "rtxui/event.hpp"  // for Event

namespace rtxui {

// Parse a sequence of |char| across |time|. Produces |Event|.
class TerminalInputParser {
 public:
  explicit TerminalInputParser() = default;
  void Timeout(int time);
  void Add(char c);
  std::optional<Event> GetEvent();
  bool HasPendingEvents() const { return !events_.empty(); }
  // The background color the terminal reported, in reply to an OSC 11
  // query, since the last call. Not an Event: it answers the library, not
  // the application.
  std::optional<Color> TakeBackgroundColor() {
    return std::exchange(background_color_, std::nullopt);
  }

 private:
  unsigned char Current();
  bool Eat();

  enum Type {
    UNCOMPLETED,
    DROP,
  };

  using Output = std::variant<Type, Event>;

  void Send(Output output);

  Output Parse();
  Output ParseUTF8();
  Output ParseESC();
  Output ParseDCS();
  Output ParseCSI();
  Output ParseOSC();
  Output ParseMouse(bool altered, bool pressed, std::vector<int> arguments);
  Output ParseCursorPosition(std::vector<int> arguments);

  void AddEvent(Event event);
  std::optional<Event> ToEvent(std::string_view);
  void EmitPastedText(std::string_view text);

  std::vector<Event> events_;
  int position_ = -1;
  int timeout_ = 0;
  std::string pending_;

  // Bracketed paste mode (enabled by Screen via "\x1b[?2004h") wraps pasted
  // content in "\x1b[200~" / "\x1b[201~" markers. While inside, incoming
  // bytes are literal paste content, not escape sequences to interpret --
  // they bypass the normal pending_/Parse() state machine entirely.
  bool in_bracketed_paste_ = false;
  std::string paste_buffer_;

  std::optional<Color> background_color_;
};

}  // namespace rtxui

#endif /* end of include guard: RTXUI_TERMINAL_TERMINAL_INPUT_PARSER */
