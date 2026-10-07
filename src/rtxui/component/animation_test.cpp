// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "rtxui/color.hpp"
#include "rtxui/component.hpp"
#include "rtxui/diagnostic.hpp"
#include "rtxui/dom/element.hpp"

namespace rtxui {
namespace {

// A clock the test moves by hand, so that animations are sampled at exact
// times.
double g_now = 1000.0;
double FakeNow() {
  return g_now;
}

class FakeClock {
 public:
  FakeClock() {
    g_now = 1000.0;
    time::SetCustomClock(&FakeNow);
  }
  ~FakeClock() { time::SetCustomClock(nullptr); }
  FakeClock(const FakeClock&) = delete;
  FakeClock& operator=(const FakeClock&) = delete;

  // Moves time forward by `ms` and ticks `element` there.
  void Advance(Element* element, double ms) {
    g_now += ms;
    element->TickTransitions(g_now);
  }
};

class DiagnosticRecorder {
 public:
  DiagnosticRecorder() {
    SetDiagnosticHandler(
        [this](const Diagnostic& d) { messages.push_back(d.message); });
  }
  ~DiagnosticRecorder() { SetDiagnosticHandler(nullptr); }
  DiagnosticRecorder(const DiagnosticRecorder&) = delete;
  DiagnosticRecorder& operator=(const DiagnosticRecorder&) = delete;

  bool Has(std::string_view fragment) const {
    for (const auto& message : messages) {
      if (message.find(fragment) != std::string::npos) {
        return true;
      }
    }
    return false;
  }

  std::vector<std::string> messages;
};

uint8_t Red(const std::optional<Color>& color) {
  REQUIRE(color.has_value());
  return color->r;
}

class Fade : public Component<Fade> {
 public:
  std::string_view view = R"html(
    <div id="box">Hi</div>
    <style>
      @keyframes fade {
        from { background-color: rgb(0, 0, 0); }
        to { background-color: rgb(200, 0, 0); }
      }
      #box { animation: fade 1s linear; }
    </style>
  )html";
};

TEST_CASE("An animation interpolates between its keyframes over time",
          "[animation]") {
  FakeClock clock;
  auto app = Ref<Fade>::New();
  app->Mount();
  Element* box = app->Root()->QuerySelector("#box");
  REQUIRE(box != nullptr);

  CHECK(Red(box->style.background_color) == 0);
  CHECK(box->HasPlayingAnimations());

  clock.Advance(box, 250);
  CHECK(Red(box->style.background_color) == 50);
  clock.Advance(box, 250);
  CHECK(Red(box->style.background_color) == 100);

  // Done, with no fill: the property goes back to its value without the
  // animation, which here is no background at all.
  clock.Advance(box, 600);
  CHECK_FALSE(box->style.background_color.has_value());
  CHECK_FALSE(box->HasPlayingAnimations());
  CHECK_FALSE(box->IsAnimating());
}

class Pulse : public Component<Pulse> {
 public:
  std::string_view view = R"html(
    <div id="box">Hi</div>
    <style>
      @keyframes pulse {
        from { opacity: 0; }
        to { opacity: 1; }
      }
      #box { animation: pulse 100ms linear infinite alternate; }
    </style>
  )html";
};

TEST_CASE("An infinite alternating animation goes back and forth",
          "[animation]") {
  FakeClock clock;
  auto app = Ref<Pulse>::New();
  app->Mount();
  Element* box = app->Root()->QuerySelector("#box");
  REQUIRE(box != nullptr);

  clock.Advance(box, 25);
  CHECK(box->style.opacity == Catch::Approx(0.25f));
  // The second iteration runs backwards.
  clock.Advance(box, 100);
  CHECK(box->style.opacity == Catch::Approx(0.75f));
  // The eleventh, 1025ms in, forwards again: and so on forever.
  clock.Advance(box, 900);
  CHECK(box->style.opacity == Catch::Approx(0.25f));
  CHECK(box->HasPlayingAnimations());
  // Never settles, so a headless run does not wait for it.
  CHECK_FALSE(box->HasPlayingAnimations(/*include_infinite=*/false));
  CHECK_FALSE(box->IsAnimating(/*include_infinite=*/false));
}

class Fill : public Component<Fill> {
 public:
  std::string_view view = R"html(
    <div id="forwards">A</div>
    <div id="backwards">B</div>
    <style>
      @keyframes grow {
        from { width: 2; }
        to { width: 10; }
      }
      #forwards { width: 5; animation: grow 100ms linear forwards; }
      #backwards { width: 5; animation: grow 100ms linear 100ms backwards; }
    </style>
  )html";
};

TEST_CASE("animation-fill-mode holds the keyframes outside the animation",
          "[animation]") {
  FakeClock clock;
  auto app = Ref<Fill>::New();
  app->Mount();
  Element* forwards = app->Root()->QuerySelector("#forwards");
  Element* backwards = app->Root()->QuerySelector("#backwards");
  REQUIRE(forwards != nullptr);
  REQUIRE(backwards != nullptr);

  // Waiting out its delay, a backwards fill already shows the first keyframe.
  CHECK(backwards->style.width.value == Catch::Approx(2.0f));
  CHECK(forwards->style.width.value == Catch::Approx(2.0f));

  clock.Advance(forwards, 50);
  backwards->TickTransitions(g_now);
  CHECK(forwards->style.width.value == Catch::Approx(6.0f));
  CHECK(backwards->style.width.value == Catch::Approx(2.0f));

  clock.Advance(forwards, 500);
  backwards->TickTransitions(g_now);
  // Forwards keeps the last keyframe; backwards returns to its own width.
  CHECK(forwards->style.width.value == Catch::Approx(10.0f));
  CHECK(backwards->style.width.value == Catch::Approx(5.0f));
}

class Steps : public Component<Steps> {
 public:
  std::string_view view = R"html(
    <div id="box">Hi</div>
    <style>
      @keyframes blink {
        0% { color: rgb(0, 0, 0); }
        50% { color: rgb(100, 0, 0); animation-timing-function: step-end; }
        100% { color: rgb(200, 0, 0); }
      }
      #box { animation: blink 1s linear; }
    </style>
  )html";
};

TEST_CASE("A keyframe's own timing function eases the way to the next one",
          "[animation]") {
  FakeClock clock;
  auto app = Ref<Steps>::New();
  app->Mount();
  Element* box = app->Root()->QuerySelector("#box");
  REQUIRE(box != nullptr);

  // The first half is linear, the animation's own timing.
  clock.Advance(box, 250);
  CHECK(Red(box->style.foreground_color) == 50);
  // The second eases with step-end: it holds 50%'s value until the end.
  clock.Advance(box, 500);
  CHECK(Red(box->style.foreground_color) == 100);
}

class Toggle : public Component<Toggle> {
 public:
  bool spinning = true;
  int counter = 0;
  std::string box_class() const { return spinning ? "spin" : ""; }
  std::string_view view = R"html(
    <div id="box" class="{box_class}">{counter}</div>
    <style>
      @keyframes spin {
        from { opacity: 0; }
        to { opacity: 1; }
      }
      .spin { animation: spin 100ms linear infinite; }
    </style>
  )html";
  Toggle() {
    Bind(spinning);
    Bind(counter);
    Bind(box_class);
  }
};

TEST_CASE("Re-rendering carries an animation on instead of restarting it",
          "[animation]") {
  FakeClock clock;
  auto app = Ref<Toggle>::New();
  app->Mount();
  Element* box = app->Root()->QuerySelector("#box");
  REQUIRE(box != nullptr);
  clock.Advance(box, 40);
  CHECK(box->style.opacity == Catch::Approx(0.4f));

  app->counter = 1;
  app->Digest();
  box = app->Root()->QuerySelector("#box");
  REQUIRE(box != nullptr);
  clock.Advance(box, 20);
  CHECK(box->style.opacity == Catch::Approx(0.6f));
}

TEST_CASE("No longer declaring an animation stops it", "[animation]") {
  FakeClock clock;
  auto app = Ref<Toggle>::New();
  app->Mount();
  Element* box = app->Root()->QuerySelector("#box");
  REQUIRE(box != nullptr);
  clock.Advance(box, 40);
  CHECK(box->style.opacity == Catch::Approx(0.4f));

  app->spinning = false;
  app->Digest();
  box = app->Root()->QuerySelector("#box");
  REQUIRE(box != nullptr);
  CHECK(box->style.opacity == Catch::Approx(1.0f));
  CHECK(box->running_animations.empty());
  CHECK_FALSE(box->IsAnimating());
}

class Pausable : public Component<Pausable> {
 public:
  bool paused = false;
  std::string box_class() const { return paused ? "paused" : ""; }
  std::string_view view = R"html(
    <div id="box" class="{box_class}">Hi</div>
    <style>
      @keyframes fade {
        from { opacity: 0; }
        to { opacity: 1; }
      }
      #box { animation: fade 1s linear; }
      #box.paused { animation-play-state: paused; }
    </style>
  )html";
  Pausable() {
    Bind(paused);
    Bind(box_class);
  }
};

TEST_CASE("animation-play-state: paused freezes an animation where it is",
          "[animation]") {
  FakeClock clock;
  auto app = Ref<Pausable>::New();
  app->Mount();
  Element* box = app->Root()->QuerySelector("#box");
  REQUIRE(box != nullptr);
  clock.Advance(box, 300);
  CHECK(box->style.opacity == Catch::Approx(0.3f));

  app->paused = true;
  app->Digest();
  box = app->Root()->QuerySelector("#box");
  REQUIRE(box != nullptr);
  clock.Advance(box, 5000);
  CHECK(box->style.opacity == Catch::Approx(0.3f));
  CHECK_FALSE(box->HasPlayingAnimations());

  // Resuming picks up from there, not from where the clock has got to.
  app->paused = false;
  app->Digest();
  box = app->Root()->QuerySelector("#box");
  REQUIRE(box != nullptr);
  clock.Advance(box, 100);
  CHECK(box->style.opacity == Catch::Approx(0.4f));
}

class Child : public Component<Child> {
 public:
  std::string_view view = R"html(
    <div id="inner">Child</div>
    <style>
      #inner { animation: shared 1s linear; }
    </style>
  )html";
};

class Parent : public Component<Parent> {
 public:
  std::string_view view = R"html(
    <Child/>
    <style>
      @keyframes shared {
        from { opacity: 0; }
        to { opacity: 1; }
      }
    </style>
  )html";
  Parent() { Import<Child>(); }
};

TEST_CASE("A component uses the @keyframes of a component around it",
          "[animation]") {
  FakeClock clock;
  DiagnosticRecorder diagnostics;
  auto app = Ref<Parent>::New();
  app->Mount();
  Element* inner = app->Root()->QuerySelector("#inner");
  REQUIRE(inner != nullptr);
  clock.Advance(inner, 500);
  CHECK(inner->style.opacity == Catch::Approx(0.5f));
  CHECK(diagnostics.messages.empty());
}

class Missing : public Component<Missing> {
 public:
  std::string_view view = R"html(
    <div id="box">Hi</div>
    <style>
      @keyframes odd {
        to { padding: 2; opacity: 0.5; }
      }
      #box { animation: nowhere 1s; }
      #box { animation: odd 1s; }
    </style>
  )html";
};

TEST_CASE("Animations that cannot run are reported", "[animation]") {
  FakeClock clock;
  DiagnosticRecorder diagnostics;
  auto app = Ref<Missing>::New();
  app->Mount();
  // The later rule wins, so `nowhere` is never looked up; `odd` is, and its
  // padding cannot be animated.
  CHECK_FALSE(diagnostics.Has("nowhere"));
  CHECK(diagnostics.Has("'padding' in @keyframes odd cannot be animated"));
}

class Unknown : public Component<Unknown> {
 public:
  std::string_view view = R"html(
    <div id="box">Hi</div>
    <style>
      #box { animation: nowhere 1s; }
    </style>
  )html";
};

TEST_CASE("An animation naming no @keyframes is reported", "[animation]") {
  FakeClock clock;
  DiagnosticRecorder diagnostics;
  auto app = Ref<Unknown>::New();
  app->Mount();
  CHECK(diagnostics.Has("animation 'nowhere' has no matching @keyframes"));
}

}  // namespace
}  // namespace rtxui
