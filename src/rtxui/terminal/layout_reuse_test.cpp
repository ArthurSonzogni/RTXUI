// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// A frame whose only changes are animated colors and opacities keeps the last
// layout. Each test here plays such animations with RTXUI_VERIFY_LAYOUT_REUSE
// set, which lays out again anyway and aborts if that draws anything else.
#include <stdlib.h>

#include <catch2/catch_test_macros.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "rtxui/color.hpp"
#include "rtxui/component.hpp"
#include "rtxui/headless.hpp"
#include "rtxui/layout/layout.hpp"
#include "rtxui/refcounted.hpp"
#include "rtxui/task.hpp"

namespace rtxui {
namespace {

class Scene : public Component<Scene> {
 public:
  bool lit = false;
  std::string state() const { return lit ? "lit" : ""; }
  std::string source;
  std::string_view view;
  explicit Scene(std::string text) : source(std::move(text)), view(source) {
    Bind(lit);
    Bind(state);
  }
};

constexpr int kWidth = 40;
constexpr int kHeight = 12;

// Every cell's colors, as drawn.
std::vector<Color> Colors(const TimelineScreen& screen) {
  std::vector<Color> colors;
  for (int y = 0; y < kHeight; ++y) {
    for (int x = 0; x < kWidth; ++x) {
      colors.push_back(screen.BackgroundAt(x, y));
      colors.push_back(screen.ForegroundAt(x, y));
    }
  }
  return colors;
}

// Flips `lit`, then plays 300ms of what follows, once to count the layouts
// that ran meanwhile, which it returns, and once more to verify the frames
// that ran none: RTXUI_VERIFY_LAYOUT_REUSE lays out again, so it would count
// too.
int LayoutsWhileAnimating(const std::string& source) {
  int layouts = 0;
  for (const bool verify : {false, true}) {
    if (verify) {
      setenv("RTXUI_VERIFY_LAYOUT_REUSE", "1", /*overwrite=*/1);
    } else {
      unsetenv("RTXUI_VERIFY_LAYOUT_REUSE");
    }
    auto app = Ref<Scene>::New(source);
    TimelineScreen screen(app, kWidth, kHeight);
    screen.Advance(0);
    PostTask([&] { app->lit = true; });
    screen.Advance(0);  // The task's frame lays out.
    const std::vector<Color> before = Colors(screen);
    ResetLayoutRunCount();
    screen.Advance(100);
    CHECK(Colors(screen) != before);  // Something is animating.
    screen.Record(200, 16);
    if (!verify) {
      layouts = LayoutRunCount();
    }
  }
  return layouts;
}

TEST_CASE("A color transition keeps the layout", "[screen][layout-reuse]") {
  CHECK(LayoutsWhileAnimating(R"html(
    <div class="card {state}">
      plain <span class="mark">marked</span> <b>inherits</b>
      <p>block</p>
    </div>
    <style>
      .card {
        border: rounded;
        background-color: #202020;
        color: #a0a0a0;
        border-color: #303030;
        transition: background-color 200ms, color 200ms,
                    border-color 200ms;
      }
      .card.lit {
        background-color: #d0d0d0;
        color: #101010;
        border-color: #f0f0f0;
      }
      .mark { background-color: #404000; }
      .lit .mark {
        background-color: #ffff00;
        transition: background-color 200ms;
      }
    </style>
  )html") == 0);
}

TEST_CASE("An inherited color transition reaches projected content",
          "[screen][layout-reuse]") {
  CHECK(LayoutsWhileAnimating(R"html(
    <div class="box {state}">
      <button>projected</button>
      <details open="true"><summary>title</summary>body</details>
    </div>
    <style>
      .box { color: #808080; transition: color 200ms; }
      .box.lit { color: #ff0000; }
    </style>
  )html") == 0);
}

TEST_CASE("An opacity animation keeps the layout", "[screen][layout-reuse]") {
  CHECK(LayoutsWhileAnimating(R"html(
    <div class="row">
      <div class="item {state}">one</div>
      <div class="item">two</div>
    </div>
    <style>
      @keyframes pulse { from { opacity: 1; } to { opacity: 0.2; } }
      .row { display: flex; background-color: #102030; }
      .item { padding: 0 1; background-color: #405060; }
      .item.lit { animation: pulse 100ms infinite alternate; }
    </style>
  )html") == 0);
}

TEST_CASE("A table row's transition keeps the layout",
          "[screen][layout-reuse]") {
  CHECK(LayoutsWhileAnimating(R"html(
    <table class="{state}">
      <tr class="row"><td>a</td><td>b</td></tr>
      <tr><td>c</td><td>d</td></tr>
    </table>
    <style>
      .row {
        background-color: #000000;
        color: #ffffff;
        opacity: 1;
        transition: background-color 200ms, color 200ms, opacity 200ms;
      }
      .lit .row {
        background-color: #00ff00;
        color: #000000;
        opacity: 0.5;
      }
    </style>
  )html") == 0);
}

TEST_CASE("Generated content follows an inherited color transition",
          "[screen][layout-reuse]") {
  CHECK(LayoutsWhileAnimating(R"html(
    <div class="tag {state}">label</div>
    <style>
      .tag { color: #0000ff; transition: color 200ms; }
      .tag.lit { color: #ffff00; }
      .tag::before { content: "> "; }
      .tag::after { content: " <"; background-color: #330033; }
    </style>
  )html") == 0);
}

TEST_CASE("A translate animation keeps the layout", "[screen][layout-reuse]") {
  CHECK(LayoutsWhileAnimating(R"html(
    <div class="panel">
      <div class="badge {state}">new</div>
      <div class="note">stays</div>
    </div>
    <style>
      @keyframes slide { from { translate: 20 1; } }
      .panel { border: rounded; background-color: #101010; }
      .badge { width: 5; background-color: #ff00ff; }
      .badge.lit { animation: slide 250ms ease-out; }
    </style>
  )html") == 0);
}

TEST_CASE("A transition of a size lays out each frame",
          "[screen][layout-reuse]") {
  CHECK(LayoutsWhileAnimating(R"html(
    <div class="bar {state}">grow</div>
    <style>
      .bar { width: 4; background-color: #ff0000; transition: width 200ms; }
      .bar.lit { width: 20; }
    </style>
  )html") > 0);
}

// A page with something of each kind for the tests below to change, which
// check that a layout reusing the last one's fragments notices each change.
class Page : public Component<Page> {
 public:
  std::vector<std::string> items = {"one", "two", "three", "four", "five"};
  std::string deep = "deep";
  bool wide = false;
  bool marked = false;
  bool starred = false;
  std::string width() const { return wide ? "wide" : ""; }
  std::string mark() const { return marked ? "marked" : ""; }
  std::string stars() const { return starred ? "starred" : ""; }

  std::string_view view = R"html(
    <div class="row {stars}">
      <div class="card {width}">card</div>
      <div class="card"><div><div><span>{deep}</span></div></div></div>
      <div class="card {mark}">mark</div>
    </div>
    <div class="list">
      <for each="{items}" as="item"><div class="item">{item}</div></for>
    </div>
    <style>
      .row { display: flex; }
      .card { width: 8; border: rounded; background-color: #202060; }
      .card.wide { width: 14; }
      .card.marked::before { content: "> "; }
      .starred .card::after { content: "*"; }
      .list { height: 5; overflow-y: scroll; border: rounded; }
    </style>
  )html";

  Page() {
    Bind(items);
    Bind(deep);
    Bind(wide);
    Bind(marked);
    Bind(starred);
    Bind(width);
    Bind(mark);
    Bind(stars);
  }
};

// Every cell's colors and the text, as drawn.
std::pair<std::string, std::vector<Color>> Drawn(const TimelineScreen& screen) {
  return {screen.Text(), Colors(screen)};
}

// Draws a page, then `change`s it and draws again, reusing what the first
// layout can offer. Checks that this draws what a page that started out
// changed draws.
void CheckChange(void (*change)(Page&)) {
  unsetenv("RTXUI_VERIFY_LAYOUT_REUSE");
  auto expected_app = Ref<Page>::New();
  change(*expected_app);
  TimelineScreen expected_screen(expected_app, kWidth, kHeight);
  expected_screen.Advance(0);
  const auto expected = Drawn(expected_screen);

  auto app = Ref<Page>::New();
  TimelineScreen screen(app, kWidth, kHeight);
  screen.Advance(0);
  CHECK(Drawn(screen) != expected);
  PostTask([&] { change(*app); });
  ResetLayoutRunCount();
  screen.Advance(0);
  CHECK(LayoutReuseCount() > 0);
  CHECK(Drawn(screen) == expected);
}

TEST_CASE("A layout lays out again only what changed",
          "[screen][layout-reuse]") {
  unsetenv("RTXUI_VERIFY_LAYOUT_REUSE");
  auto app = Ref<Page>::New();
  TimelineScreen screen(app, kWidth, kHeight);
  screen.Advance(0);
  ResetLayoutRunCount();
  PostTask([&] { app->deep = "changed"; });
  screen.Advance(0);
  const int runs = LayoutRunCount();
  CHECK(LayoutReuseCount() > 0);

  // Laying out a page with nothing to reuse runs every box, and more.
  auto fresh_app = Ref<Page>::New();
  ResetLayoutRunCount();
  TimelineScreen fresh(fresh_app, kWidth, kHeight);
  fresh.Advance(0);
  CHECK(runs * 2 < LayoutRunCount());
}

TEST_CASE("A reused layout follows a text change deep in the tree",
          "[screen][layout-reuse]") {
  CheckChange([](Page& page) { page.deep = "changed text"; });
}

TEST_CASE("A reused layout follows a style change", "[screen][layout-reuse]") {
  CheckChange([](Page& page) { page.wide = true; });
}

TEST_CASE("A reused layout follows generated content",
          "[screen][layout-reuse]") {
  CheckChange([](Page& page) { page.marked = true; });
  // Through an ancestor's class: nothing about the cards themselves changes.
  CheckChange([](Page& page) { page.starred = true; });
}

TEST_CASE("A reused layout follows an added and a removed child",
          "[screen][layout-reuse]") {
  CheckChange(
      [](Page& page) { page.items.insert(page.items.begin(), "zero"); });
  CheckChange([](Page& page) { page.items.erase(page.items.begin()); });
}

// The grid fields of a style live outside the part compared byte for byte.
// The area changes through a class above the grid, so that nothing else about
// the grid or the item does.
class Areas : public Component<Areas> {
 public:
  bool swapped = false;
  std::string side() const { return swapped ? "swapped" : ""; }
  std::string_view view = R"html(
    <div class="{side}">
      <div class="grid"><div class="item">item</div></div>
      <div><div>unchanged</div></div>
    </div>
    <style>
      .grid { display: grid; grid-template-areas: "a b"; width: 20; }
      .item { grid-area: a; }
      .swapped .item { grid-area: b; }
    </style>
  )html";
  Areas() {
    Bind(swapped);
    Bind(side);
  }
};

TEST_CASE("A reused layout follows a change of grid area",
          "[screen][layout-reuse]") {
  unsetenv("RTXUI_VERIFY_LAYOUT_REUSE");
  auto app = Ref<Areas>::New();
  TimelineScreen screen(app, kWidth, kHeight);
  screen.Advance(0);
  REQUIRE(screen.Text().starts_with("item"));
  PostTask([&] { app->swapped = true; });
  ResetLayoutRunCount();
  screen.Advance(0);
  CHECK(LayoutReuseCount() > 0);
  CHECK(screen.Text().starts_with("          item"));
}

TEST_CASE("A reused layout follows a scroll", "[screen][layout-reuse]") {
  unsetenv("RTXUI_VERIFY_LAYOUT_REUSE");
  auto app = Ref<Page>::New();
  TimelineScreen screen(app, kWidth, kHeight);
  screen.Advance(0);
  const std::string before = screen.Text();
  REQUIRE(before.find("one") != std::string::npos);
  screen.Input("\x1b[<65;3;6M");  // Wheel down over the list.
  screen.Advance(0);
  CHECK(screen.Text() != before);
  CHECK(screen.Text().find("four") != std::string::npos);
}

TEST_CASE("A reused layout follows a resize", "[screen][layout-reuse]") {
  unsetenv("RTXUI_VERIFY_LAYOUT_REUSE");
  auto app = Ref<Page>::New();
  TimelineScreen screen(app, kWidth, kHeight);
  screen.Advance(0);
  screen.Resize(kWidth / 2, kHeight);
  screen.Advance(0);

  auto fresh_app = Ref<Page>::New();
  TimelineScreen fresh(fresh_app, kWidth / 2, kHeight);
  fresh.Advance(0);
  CHECK(screen.Text() == fresh.Text());
}

}  // namespace
}  // namespace rtxui
