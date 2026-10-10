// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// A frame whose only changes are animated colors and opacities keeps the last
// layout. Each test here plays such animations with RTXUI_VERIFY_LAYOUT_REUSE
// set, which lays out again anyway and aborts if that draws anything else.
#include <catch2/catch_test_macros.hpp>
#include <cstdlib>
#include <string>
#include <vector>

#include "rtxui/color.hpp"
#include "rtxui/component.hpp"
#include "rtxui/headless.hpp"
#include "rtxui/layout/layout.hpp"
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

}  // namespace
}  // namespace rtxui
