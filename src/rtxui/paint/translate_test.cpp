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

class Moved : public Component<Moved> {
 public:
  int clicks = 0;
  void Click() { ++clicks; }
  Moved() {
    Bind(clicks);
    Bind(Click);
  }
  std::string_view view = R"html(
    <div class="box" onclick="Click">AB</div>
    <div>CD</div>
    <style>
      .box { width: 2; translate: 3 1; }
    </style>
  )html";
};

TEST_CASE("translate moves a box without moving anything else",
          "[paint][translate]") {
  // "CD" stays where layout put it, on the second row, under the box's own
  // place.
  CHECK(RenderToString(Ref<Moved>::New(), 6, 3) ==
        "\n"
        "CD AB\n"
        "\n");
}

TEST_CASE("A translated box is clicked where it is drawn",
          "[paint][translate]") {
  auto app = Ref<Moved>::New();
  HeadlessScreen screen(app, 6, 3);
  screen.Click(0, 0);  // Where layout put it.
  CHECK(app->clicks == 0);
  screen.Click(4, 1);  // Where it is drawn.
  CHECK(app->clicks == 1);
}

class Halfway : public Component<Halfway> {
 public:
  std::string_view view = R"html(
    <div class="box">ABCD</div>
    <style>
      .box { width: 4; translate: 50% 0; }
    </style>
  )html";
};

TEST_CASE("A translate percentage is of the box's own size",
          "[paint][translate]") {
  CHECK(RenderToString(Ref<Halfway>::New(), 10, 1) == "  ABCD\n");
}

TEST_CASE("translate parses lengths, percentages and calc()",
          "[css][translate]") {
  ComputedStyle style;
  ApplyStyle(style, {"translate", "calc(100% + 2) -1"});
  CHECK(style.translate_x == Length::MakeCalc(2, 100));
  CHECK(style.translate_y == Length::Cells(-1));

  ApplyStyle(style, {"translate", "50%"});
  CHECK(style.translate_x == Length::Pct(50));
  CHECK(style.translate_y == Length::Cells(0));

  ApplyStyle(style, {"translate", "none"});
  CHECK(style.translate_x == Length::Cells(0));

  std::vector<std::string> messages;
  SetDiagnosticHandler([&](const Diagnostic& diagnostic) {
    messages.push_back(diagnostic.message);
  });
  ApplyStyle(style, {"translate", "auto"});
  ApplyStyle(style, {"translate", "left 2"});
  ApplyStyle(style, {"translate", "1 2 3"});
  SetDiagnosticHandler(nullptr);
  CHECK(messages.size() == 3);
  CHECK(style.translate_x == Length::Cells(0));
}

}  // namespace
}  // namespace rtxui
