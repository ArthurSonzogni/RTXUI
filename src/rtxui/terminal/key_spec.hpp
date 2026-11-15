// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_TERMINAL_KEY_SPEC_HPP_
#define RTXUI_TERMINAL_KEY_SPEC_HPP_

#include <optional>
#include <string_view>

#include "rtxui/event.hpp"

namespace rtxui {

/// A key as a template names it in `onkey:<key>`: `j`, `G`, `ArrowDown`,
/// `ctrl+d`, `shift+Tab`, `Space`.
struct KeySpec {
  std::uint32_t codepoint = 0;
  Event::Keyboard::Special special = Event::Keyboard::None;
  bool alt = false;
  bool ctrl = false;
  bool meta = false;
  bool shift = false;

  /// The key `text` names, or nothing when it names none.
  static std::optional<KeySpec> Parse(std::string_view text);

  /// Whether pressing `key` is this key. A character is compared without
  /// Shift, which the character itself says: `G` is Shift+g.
  bool Matches(const Event::Keyboard& key) const;
};

}  // namespace rtxui

#endif  // RTXUI_TERMINAL_KEY_SPEC_HPP_
