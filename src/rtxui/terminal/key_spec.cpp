// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/terminal/key_spec.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <utility>

#include "rtxui/base/string.hpp"
#include "rtxui/event.hpp"

namespace rtxui {

namespace {

using Special = Event::Keyboard::Special;

constexpr auto kSpecialNames =
    std::to_array<std::pair<std::string_view, Special>>({
        {"ArrowLeft", Special::ArrowLeft},
        {"ArrowRight", Special::ArrowRight},
        {"ArrowUp", Special::ArrowUp},
        {"ArrowDown", Special::ArrowDown},
        {"Backspace", Special::Backspace},
        {"Delete", Special::Delete},
        {"Escape", Special::Escape},
        {"Return", Special::Return},
        {"Enter", Special::Return},
        {"Tab", Special::Tab},
        {"TabReverse", Special::TabReverse},
        {"Insert", Special::Insert},
        {"Home", Special::Home},
        {"End", Special::End},
        {"PageUp", Special::PageUp},
        {"PageDown", Special::PageDown},
        {"F1", Special::F1},
        {"F2", Special::F2},
        {"F3", Special::F3},
        {"F4", Special::F4},
        {"F5", Special::F5},
        {"F6", Special::F6},
        {"F7", Special::F7},
        {"F8", Special::F8},
        {"F9", Special::F9},
        {"F10", Special::F10},
        {"F11", Special::F11},
        {"F12", Special::F12},
    });

std::uint32_t Lower(std::uint32_t c) {
  return c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c;
}

}  // namespace

std::optional<KeySpec> KeySpec::Parse(std::string_view text) {
  KeySpec spec;
  // Modifiers come first, each followed by '+'. The key itself may be '+'.
  while (true) {
    const size_t plus = text.find('+');
    if (plus == std::string_view::npos || plus + 1 == text.size()) {
      break;
    }
    const std::string_view modifier = text.substr(0, plus);
    if (modifier == "ctrl") {
      spec.ctrl = true;
    } else if (modifier == "alt") {
      spec.alt = true;
    } else if (modifier == "meta") {
      spec.meta = true;
    } else if (modifier == "shift") {
      spec.shift = true;
    } else {
      return std::nullopt;
    }
    text.remove_prefix(plus + 1);
  }

  if (text == "Space") {
    spec.codepoint = ' ';
  } else {
    for (const auto& [name, special] : kSpecialNames) {
      if (name == text) {
        spec.special = special;
      }
    }
  }
  if (spec.special == Special::Tab && spec.shift) {
    // Terminals send Shift+Tab as its own key.
    spec.special = Special::TabReverse;
    spec.shift = false;
  }
  if (spec.special == Special::None && spec.codepoint == 0) {
    size_t end = 0;
    std::uint32_t codepoint = 0;
    if (!EatCodePoint(text, 0, &end, &codepoint) || end != text.size()) {
      return std::nullopt;
    }
    spec.codepoint = codepoint;
  }
  if (spec.codepoint != 0 && spec.shift) {
    // A character carries its Shift: shift+g is G.
    if (spec.codepoint >= 'a' && spec.codepoint <= 'z') {
      spec.codepoint = spec.codepoint - 'a' + 'A';
    }
    spec.shift = false;
  }
  return spec;
}

bool KeySpec::Matches(const Event::Keyboard& key) const {
  const Event::Modifier modifier = key.modifier();
  if (modifier.alt() != alt || modifier.ctrl() != ctrl ||
      modifier.meta() != meta) {
    return false;
  }
  if (special != Special::None) {
    return key.special() == special && modifier.shift() == shift;
  }
  if (key.special() != Special::None) {
    return false;
  }
  // Ctrl+D arrives as 'd' with Ctrl, whichever case the template wrote.
  if (ctrl || alt || meta) {
    return Lower(key.codepoint()) == Lower(codepoint);
  }
  return key.codepoint() == codepoint;
}

}  // namespace rtxui
