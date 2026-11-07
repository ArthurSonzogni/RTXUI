// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/canvas.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "rtxui/color.hpp"
#include "rtxui/component.hpp"
#include "rtxui/diagnostic.hpp"
#include "rtxui/event.hpp"
#include "rtxui/headless.hpp"
#include "rtxui/refcounted.hpp"

namespace rtxui {
namespace {

// The rows of a screen's text, without their trailing spaces.
std::vector<std::string> Rows(const std::string& text) {
  std::vector<std::string> rows;
  size_t start = 0;
  while (start <= text.size()) {
    size_t end = text.find('\n', start);
    if (end == std::string::npos) {
      end = text.size();
    }
    std::string row = text.substr(start, end - start);
    row.erase(row.find_last_not_of(' ') + 1);
    rows.push_back(row);
    start = end + 1;
  }
  return rows;
}

// SGR mouse reports, with 0-based cells.
std::string Press(int x, int y) {
  return "\x1b[<0;" + std::to_string(x + 1) + ";" + std::to_string(y + 1) + "M";
}
std::string Drag(int x, int y) {
  return "\x1b[<32;" + std::to_string(x + 1) + ";" + std::to_string(y + 1) +
         "M";
}
std::string Release(int x, int y) {
  return "\x1b[<0;" + std::to_string(x + 1) + ";" + std::to_string(y + 1) + "m";
}
std::string Hover(int x, int y) {
  return "\x1b[<35;" + std::to_string(x + 1) + ";" + std::to_string(y + 1) +
         "M";
}

class Sized : public Component<Sized> {
 public:
  int draws = 0;
  int drawn_width = 0;
  int drawn_height = 0;
  std::string_view view = R"(
    <canvas width="6" height="2" ondraw="Draw"/>
  )";
  Sized() { Bind(Draw); }
  void Draw(Canvas& c) {
    ++draws;
    drawn_width = c.width();
    drawn_height = c.height();
    c.Text(0, 0, "abcdefgh");
    c.Set(5, 1, "z");
  }
};

TEST_CASE("canvas draws at the size of its attributes", "[component][canvas]") {
  auto app = Ref<Sized>::New();
  HeadlessScreen screen(app, 20, 4);
  CHECK(app->drawn_width == 6);
  CHECK(app->drawn_height == 2);
  const auto rows = Rows(screen.Text());
  CHECK(rows[0] == "abcdef");  // Clipped to its width.
  CHECK(rows[1] == "     z");
}

class Styled : public Component<Styled> {
 public:
  int drawn_width = 0;
  int drawn_height = 0;
  std::string_view view = R"(
    <canvas class="c" ondraw="Draw"/>
    <style>
      .c { display: block; width: 100%; height: 3; padding: 0 1;
           border: rounded; }
    </style>
  )";
  Styled() { Bind(Draw); }
  void Draw(Canvas& c) {
    drawn_width = c.width();
    drawn_height = c.height();
    c.Set(0, 0, "x");
  }
};

TEST_CASE("canvas draws in its content box, as CSS sizes it",
          "[component][canvas]") {
  auto app = Ref<Styled>::New();
  HeadlessScreen screen(app, 20, 5);
  // 20 wide less two borders and two padding cells; 3 tall less two borders.
  CHECK(app->drawn_width == 16);
  CHECK(app->drawn_height == 1);
  CHECK(Rows(screen.Text())[1] == "│ x" + std::string(16, ' ') + "│");

  screen.Resize(12, 5);
  CHECK(app->drawn_width == 8);
}

class Counting : public Component<Counting> {
 public:
  int draws = 0;
  int value = 0;
  std::string_view view = R"(
    <canvas width="3" height="1" ondraw="Draw"/>
    <button onclick="Other">other</button>
  )";
  Counting() {
    Bind(value);
    Bind(Draw);
    Bind(Other);
  }
  void Draw(Canvas& c) {
    ++draws;
    c.Text(0, 0, std::to_string(value));
  }
  void Other() {}
};

TEST_CASE("canvas draws again only when its owner's state changes",
          "[component][canvas]") {
  auto app = Ref<Counting>::New();
  HeadlessScreen screen(app, 20, 3);
  const int first = app->draws;
  CHECK(first >= 1);

  screen.Input(Hover(10, 0));
  screen.Input(Hover(11, 0));
  CHECK(app->draws == first);

  app->value = 42;
  screen.Input(Hover(12, 0));  // Any event digests the state.
  CHECK(app->draws == first + 1);
  CHECK(Rows(screen.Text())[0].starts_with("42"));
}

class Painter : public Component<Painter> {
 public:
  std::vector<std::string> events;
  std::string_view view = R"(
    <div style="padding: 1 2;">
      <canvas width="5" height="2" ondraw="Draw" onpointer="OnPointer"/>
    </div>
  )";
  Painter() {
    Bind(Draw);
    Bind(OnPointer);
  }
  void Draw(Canvas&) {}
  void OnPointer(const PointerEvent& e) {
    const char* motion = e.mouse.motion() == Event::Mouse::Pressed ? "press"
                         : e.mouse.motion() == Event::Mouse::Released
                             ? "release"
                             : "move";
    events.push_back(std::string(motion) + " " + std::to_string(e.x) + "," +
                     std::to_string(e.y) + " " + std::to_string(e.pixel_x) +
                     "," + std::to_string(e.pixel_y));
  }
};

TEST_CASE("canvas reports the pointer in its own coordinates",
          "[component][canvas]") {
  auto app = Ref<Painter>::New();
  HeadlessScreen screen(app, 20, 6);

  // Outside: nothing.
  screen.Input(Hover(0, 0));
  CHECK(app->events.empty());

  // The canvas starts at column 2, row 1. Braille: 2×4 dots a cell, the
  // middle one without pixel reports.
  screen.Input(Hover(3, 2));
  screen.Input(Press(2, 1));
  // Held: dragged past its edge, it keeps reporting.
  screen.Input(Drag(10, 4));
  screen.Input(Release(10, 4));
  // Released: outside again, nothing.
  screen.Input(Hover(12, 4));
  CHECK(app->events == std::vector<std::string>{
                           "move 1,1 3,6",
                           "press 0,0 1,2",
                           "move 8,3 17,14",
                           "release 8,3 17,14",
                       });
}

class Pixels : public Component<Pixels> {
 public:
  Canvas::Pixels mode = Canvas::Pixels::Braille;
  std::string_view view = R"(
    <canvas width="4" height="1" ondraw="Draw"/>
  )";
  Pixels() { Bind(Draw); }
  void Draw(Canvas& c) {
    c.SetPixels(mode);
    CHECK(c.pixel_width() == 4 * (mode == Canvas::Pixels::HalfBlock ? 1 : 2));
    // A diagonal from the top left, and a dot in the last cell.
    c.Line(0, 0, 1, 1);
    c.Point(c.pixel_width() - 1, c.pixel_height() - 1);
  }
};

TEST_CASE("canvas draws dots in each pixel mode", "[component][canvas]") {
  auto app = Ref<Pixels>::New();
  SECTION("braille") {
    HeadlessScreen screen(app, 10, 2);
    CHECK(Rows(screen.Text())[0] == "⠑  ⢀");
  }
  SECTION("quadrant") {
    app->mode = Canvas::Pixels::Quadrant;
    HeadlessScreen screen(app, 10, 2);
    CHECK(Rows(screen.Text())[0] == "▚  ▗");
  }
  SECTION("half block") {
    app->mode = Canvas::Pixels::HalfBlock;
    HeadlessScreen screen(app, 10, 2);
    CHECK(Rows(screen.Text())[0] == "▀▄ ▄");
  }
}

class TwoColors : public Component<TwoColors> {
 public:
  std::string_view view = R"(
    <canvas width="1" height="1" ondraw="Draw"/>
  )";
  TwoColors() { Bind(Draw); }
  void Draw(Canvas& c) {
    c.SetPixels(Canvas::Pixels::HalfBlock);
    c.Point(0, 0, Color::RGB(255, 0, 0));
    c.Point(0, 1, Color::RGB(0, 0, 255));
  }
};

TEST_CASE("canvas half blocks take a color each", "[component][canvas]") {
  TimelineScreen screen(Ref<TwoColors>::New(), 4, 1);
  CHECK(screen.Text().starts_with("▀"));
  CHECK(screen.ForegroundAt(0, 0) == Color::RGB(255, 0, 0));
  CHECK(screen.BackgroundAt(0, 0) == Color::RGB(0, 0, 255));
}

class Wide : public Component<Wide> {
 public:
  std::string_view view = R"(
    <canvas width="5" height="1" ondraw="Draw"/>
  )";
  Wide() { Bind(Draw); }
  void Draw(Canvas& c) {
    c.Text(0, 0, "a界b");
    c.Set(4, 0, "界");  // Does not fit: dropped.
  }
};

TEST_CASE("canvas gives a wide glyph two cells", "[component][canvas]") {
  HeadlessScreen screen(Ref<Wide>::New(), 10, 1);
  CHECK(Rows(screen.Text())[0] == "a界b");
}

class Undrawn : public Component<Undrawn> {
 public:
  std::string_view view = R"(<canvas width="2" height="1"/>)";
};

class Misnamed : public Component<Misnamed> {
 public:
  std::string_view view = R"(<canvas width="2" height="1" ondraw="Nope"/>)";
};

TEST_CASE("canvas reports a missing draw function", "[component][canvas]") {
  std::vector<std::string> messages;
  SetDiagnosticHandler(
      [&](const Diagnostic& d) { messages.push_back(d.message()); });
  {
    HeadlessScreen undrawn(Ref<Undrawn>::New(), 10, 1);
    HeadlessScreen misnamed(Ref<Misnamed>::New(), 10, 1);
  }
  SetDiagnosticHandler(nullptr);
  CHECK(messages == std::vector<std::string>{
                        "<canvas> has no ondraw: nothing draws it",
                        "<canvas ondraw=\"Nope\">: no bound method takes an "
                        "rtxui::Canvas& by that name",
                    });
}

}  // namespace
}  // namespace rtxui
