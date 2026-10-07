// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

#include "rtxui/component.hpp"
#include "rtxui/diagnostic.hpp"
#include "rtxui/headless.hpp"
#include "rtxui/layout/style.hpp"
#include "rtxui/style/apply_style.hpp"

namespace rtxui {
namespace {

class Outlined : public Component<Outlined> {
 public:
  std::string_view view = R"html(
    <div class="row">
      <span>ab</span><span class="box">cd</span><span>ef</span>
    </div>
    <style>
      .row { padding: 1 2; }
      .box { outline: solid; }
    </style>
  )html";
};

TEST_CASE("An outline is drawn around a box without moving anything",
          "[paint][outline]") {
  // The outline lands on the padding around the row and over the
  // neighbouring text, even the text painted after it. Nothing moves, as it
  // would for a border.
  CHECK(RenderToString(Ref<Outlined>::New(), 10, 3) ==
        "   ┌──┐\n"
        "  a│cd│f\n"
        "   └──┘\n");
}

class Offset : public Component<Offset> {
 public:
  std::string_view view = R"html(
    <div class="frame">
      <div class="box">x</div>
    </div>
    <style>
      .frame { padding: 2 4; }
      .box { width: 3; height: 1; outline: double; outline-offset: 1; }
    </style>
  )html";
};

TEST_CASE("outline-offset moves the outline away from the box",
          "[paint][outline]") {
  CHECK(RenderToString(Ref<Offset>::New(), 12, 5) ==
        "  ╔═════╗\n"
        "  ║     ║\n"
        "  ║ x   ║\n"
        "  ║     ║\n"
        "  ╚═════╝\n");
}

TEST_CASE("outline shorthand and longhands parse", "[css][outline]") {
  ComputedStyle style;
  ApplyStyle(style, {"outline", "rgb(1, 2, 3) round"});
  CHECK(style.outline_style == BorderStyle::Round);
  CHECK(style.outline_color == Color::RGB(1, 2, 3));

  ApplyStyle(style, {"outline", "1"});
  CHECK(style.outline_style == BorderStyle::Solid);
  CHECK_FALSE(style.outline_color.has_value());

  ApplyStyle(style, {"outline", "0"});
  CHECK(style.outline_style == BorderStyle::None);

  ApplyStyle(style, {"outline-style", "heavy"});
  ApplyStyle(style, {"outline-color", "red"});
  ApplyStyle(style, {"outline-offset", "-1"});
  CHECK(style.outline_style == BorderStyle::Heavy);
  CHECK(style.outline_color.has_value());
  CHECK(style.outline_offset == -1);

  std::vector<std::string> messages;
  SetDiagnosticHandler([&](const Diagnostic& diagnostic) {
    messages.push_back(diagnostic.message);
  });
  ApplyStyle(style, {"outline", "solid wobbly"});
  ApplyStyle(style, {"outline-offset", "far"});
  SetDiagnosticHandler(nullptr);
  CHECK(messages.size() == 2);
  CHECK(style.outline_offset == -1);
}

}  // namespace
}  // namespace rtxui
