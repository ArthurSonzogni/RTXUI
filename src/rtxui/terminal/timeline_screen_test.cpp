// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <string>
#include <vector>

#include "rtxui/component.hpp"
#include "rtxui/headless.hpp"
#include "rtxui/task.hpp"

namespace rtxui {
namespace {

class Later : public Component<Later> {
 public:
  std::string label = "waiting";
  Later() { Bind(label); }
  std::string_view view = R"html(<span>{label}</span>)html";
};

TEST_CASE("A timeline screen runs delayed tasks on its own clock",
          "[headless][timeline]") {
  auto app = Ref<Later>::New();
  TimelineScreen timeline(app, 10, 1);
  PostDelayedTask([&] { app->label = "done"; }, std::chrono::milliseconds(500));
  CHECK(timeline.Advance(499) == "waiting\n");
  CHECK(timeline.Advance(1) == "done\n");
  CHECK(timeline.Elapsed() == 500);
}

class Grow : public Component<Grow> {
 public:
  std::string_view view = R"html(
    <div class="bar">abcd</div>
    <style>
      @keyframes grow { from { width: 0; } to { width: 4; } }
      .bar { height: 1; overflow: hidden; animation: grow 64ms linear; }
    </style>
  )html";
};

TEST_CASE("A timeline screen records the frames an animation draws",
          "[headless][timeline]") {
  TimelineScreen timeline(Ref<Grow>::New(), 4, 1);
  // A cell more every 16ms, then nothing new.
  CHECK(timeline.Record(100) ==
        std::vector<std::string>{"\n", "a\n", "ab\n", "abc\n", "abcd\n"});
}

class Delayed : public Component<Delayed> {
 public:
  std::string bar_class;
  Delayed() { Bind(bar_class); }
  std::string_view view = R"html(
    <div class="bar {bar_class}">abcdefgh</div>
    <style>
      .bar {
        width: 0;
        height: 1;
        overflow: hidden;
        transition: width 80ms linear 100ms;
      }
      .bar.open { width: 8; }
    </style>
  )html";
};

TEST_CASE("A transition waits out its delay", "[headless][timeline]") {
  auto app = Ref<Delayed>::New();
  TimelineScreen timeline(app, 10, 1);
  PostTask([&] { app->bar_class = "open"; });
  timeline.Advance(0);
  CHECK(timeline.Advance(99) == "\n");      // Still waiting.
  CHECK(timeline.Advance(41) == "abcd\n");  // Halfway through.
  CHECK(timeline.Advance(40) == "abcdefgh\n");
}

class HiddenParent : public Component<HiddenParent> {
 public:
  std::string wrap_class = "hidden";
  HiddenParent() { Bind(wrap_class); }
  std::string_view view = R"html(
    <div class="{wrap_class}">
      <div class="bar">abcd</div>
    </div>
    <style>
      @keyframes grow { from { width: 0; } to { width: 4; } }
      .bar { height: 1; overflow: hidden; animation: grow 64ms linear; }
      .hidden { display: none; }
    </style>
  )html";
};

TEST_CASE("An animation under a hidden parent starts once it is shown",
          "[headless][timeline]") {
  auto app = Ref<HiddenParent>::New();
  TimelineScreen timeline(app, 4, 1);
  timeline.Advance(1000);  // Long enough to have played out unseen.
  PostTask([&] { app->wrap_class = ""; });
  timeline.Advance(0);
  // From the beginning, as it is shown.
  CHECK(timeline.Advance(32) == "ab\n");
  CHECK(timeline.Advance(32) == "abcd\n");
}

class Highlighted : public Component<Highlighted> {
 public:
  std::string_view view = R"html(
    <div class="row">ab</div>
    <style>
      .row { background-color: rgb(10, 20, 30); color: rgb(200, 100, 50); }
    </style>
  )html";
};

TEST_CASE("A timeline screen reads the colors drawn", "[headless][timeline]") {
  TimelineScreen screen(Ref<Highlighted>::New(), 4, 1);
  CHECK(screen.BackgroundAt(0, 0) == Color::RGB(10, 20, 30));
  CHECK(screen.ForegroundAt(1, 0) == Color::RGB(200, 100, 50));
  // Outside the screen: nothing drawn.
  CHECK(screen.BackgroundAt(9, 9) == Color());
}

}  // namespace
}  // namespace rtxui
