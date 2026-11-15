// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/terminal/key_spec.hpp"

#include <catch2/catch_test_macros.hpp>
#include <string_view>

#include "rtxui/event.hpp"

namespace rtxui {
namespace {

bool Matches(std::string_view spec, const Event& event) {
  auto parsed = KeySpec::Parse(spec);
  REQUIRE(parsed);
  return parsed->Matches(event.get<Event::Keyboard>());
}

TEST_CASE("KeySpec matches characters", "[key_spec]") {
  CHECK(Matches("j", Event::j()));
  CHECK_FALSE(Matches("j", Event::J()));
  CHECK(Matches("G", Event::G()));
  CHECK(Matches("shift+g", Event::G()));
  CHECK_FALSE(Matches("g", Event::G()));
  CHECK(Matches("?", Event(Event::Keyboard::From('?'))));
  CHECK(Matches("+", Event(Event::Keyboard::From('+'))));
  CHECK(Matches("é", Event(Event::Keyboard::From("é"))));
  CHECK(Matches("Space", Event(Event::Keyboard::From(' '))));
  CHECK_FALSE(Matches("j", Event::ArrowDown()));
}

TEST_CASE("KeySpec matches modifiers exactly", "[key_spec]") {
  CHECK(Matches("ctrl+d", Event::CtrlD()));
  CHECK(Matches("ctrl+D", Event::CtrlD()));
  CHECK_FALSE(Matches("d", Event::CtrlD()));
  CHECK_FALSE(Matches("ctrl+d", Event::d()));
  CHECK(Matches("alt+x", Event::AltX()));
  CHECK(Matches("ctrl+alt+x", Event::CtrlAltX()));
  CHECK_FALSE(Matches("ctrl+x", Event::CtrlAltX()));
  CHECK_FALSE(Matches("ctrl++", Event(Event::Keyboard::From('+'))));
}

TEST_CASE("KeySpec matches special keys", "[key_spec]") {
  CHECK(Matches("ArrowDown", Event::ArrowDown()));
  CHECK_FALSE(Matches("ArrowDown", Event::ArrowDownCtrl()));
  CHECK(Matches("ctrl+ArrowDown", Event::ArrowDownCtrl()));
  CHECK(Matches("Return", Event::Return()));
  CHECK(Matches("Enter", Event::Return()));
  CHECK(Matches("Escape", Event::Escape()));
  CHECK(Matches("shift+Tab", Event::TabReverse()));
  CHECK_FALSE(Matches("Tab", Event::TabReverse()));
  CHECK(Matches("F5", Event::F5()));
}

TEST_CASE("KeySpec rejects what names no key", "[key_spec]") {
  CHECK_FALSE(KeySpec::Parse("Arow"));
  CHECK_FALSE(KeySpec::Parse("jk"));
  CHECK_FALSE(KeySpec::Parse(""));
  CHECK_FALSE(KeySpec::Parse("control+d"));
  CHECK_FALSE(KeySpec::Parse("ArrowDown+ctrl"));
}

}  // namespace
}  // namespace rtxui
