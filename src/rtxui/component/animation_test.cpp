// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "rtxui/color.hpp"
#include "rtxui/component.hpp"
#include "rtxui/diagnostic.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/headless.hpp"
#include "rtxui/screen.hpp"
#include "rtxui/terminal/terminal_device.hpp"

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
    <Child id="host"/>
    <style>
      @keyframes shared {
        from { opacity: 0; }
        to { opacity: 1; }
      }
      #host { animation: shared 1s linear; }
    </style>
  )html";
  Parent() { Import<Child>(); }
};

TEST_CASE("@keyframes are scoped to the component declaring them",
          "[animation]") {
  FakeClock clock;
  DiagnosticRecorder diagnostics;
  auto app = Ref<Parent>::New();
  app->Mount();

  // The parent's own rule, on the <Child> tag it wrote, finds its keyframes.
  Element* host = app->Root()->QuerySelector("#host");
  REQUIRE(host != nullptr);
  clock.Advance(host, 500);
  CHECK(host->style.opacity == Catch::Approx(0.5f));

  // The child's rule does not see the parent's keyframes.
  Element* inner = app->Root()->QuerySelector("#inner");
  REQUIRE(inner != nullptr);
  inner->TickTransitions(g_now);
  CHECK(inner->style.opacity == Catch::Approx(1.0f));
  CHECK(diagnostics.Has("animation 'shared' has no matching @keyframes"));
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

class Slide : public Component<Slide> {
 public:
  std::string_view view = R"html(
    <div id="box">Hi</div>
    <style>
      @keyframes slide { from { right: -10; } }
      #box { position: fixed; top: 0; right: 2; animation: slide 1s linear; }
    </style>
  )html";
};

TEST_CASE("An animation moves an element by its offsets", "[animation]") {
  FakeClock clock;
  auto app = Ref<Slide>::New();
  app->Mount();
  Element* box = app->Root()->QuerySelector("#box");
  REQUIRE(box != nullptr);

  CHECK(box->style.right == Length::Cells(-10));
  clock.Advance(box, 500);
  CHECK(box->style.right == Length::Cells(-4));
  // `to` is left out, so it ends on the element's own offset.
  clock.Advance(box, 600);
  CHECK(box->style.right == Length::Cells(2));
}

TEST_CASE("An element sliding in from past the edge of the screen is clipped",
          "[animation]") {
  FakeClock clock;
  auto app = Ref<Slide>::New();
  auto device = std::make_shared<MockTerminalDevice>();
  device->TriggerResize(10, 1);
  Screen screen(app, device);
  screen.Draw();
  CHECK(screen.Text() == "\n");  // `right: -10` puts it past the edge.

  g_now += 900;  // right: 0.8, drawn as 0.
  screen.Step();
  CHECK(screen.Text() == "        Hi\n");

  g_now += 200;
  screen.Step();
  CHECK(screen.Text() == "      Hi\n");
}

class Inset : public Component<Inset> {
 public:
  std::string_view view = R"html(
    <div id="box">Hi</div>
    <style>
      @keyframes drop { from { inset: 0; } to { inset: 4; } }
      #box { position: fixed; animation: drop 1s linear forwards; }
    </style>
  )html";
};

TEST_CASE("inset animates the four offsets", "[animation]") {
  FakeClock clock;
  auto app = Ref<Inset>::New();
  app->Mount();
  Element* box = app->Root()->QuerySelector("#box");
  REQUIRE(box != nullptr);

  clock.Advance(box, 500);
  CHECK(box->style.top == Length::Cells(2));
  CHECK(box->style.right == Length::Cells(2));
  CHECK(box->style.bottom == Length::Cells(2));
  CHECK(box->style.left == Length::Cells(2));
}

class Shift : public Component<Shift> {
 public:
  std::string box_class;
  Shift() { Bind(box_class); }
  std::string_view view = R"html(
    <div id="box" class="{box_class}">Hi</div>
    <style>
      #box { position: fixed; left: 0; transition: left 1s linear; }
      #box.moved { left: 10; }
    </style>
  )html";
};

TEST_CASE("A transition moves an element by its offsets", "[animation]") {
  FakeClock clock;
  auto app = Ref<Shift>::New();
  app->Mount();
  Element* box = app->Root()->QuerySelector("#box");
  REQUIRE(box != nullptr);

  app->box_class = "moved";
  app->Digest();
  CHECK(box->style.left == Length::Cells(0));
  clock.Advance(box, 500);
  CHECK(box->style.left == Length::Cells(5));
  clock.Advance(box, 600);
  CHECK(box->style.left == Length::Cells(10));
}

class Ending : public Component<Ending> {
 public:
  int ended = 0;
  std::string last_arg;
  void Ended() { ++ended; }
  void EndedWith(std::string arg) { last_arg = std::move(arg); }
  Ending() {
    Bind(Ended);
    Bind(EndedWith);
  }
  std::string_view view = R"html(
    <div id="once" onanimationend="Ended">Hi</div>
    <div id="named" onanimationend="EndedWith(pop)">Hi</div>
    <div id="forever" onanimationend="Ended">Hi</div>
    <style>
      @keyframes pop { from { opacity: 0; } }
      #once, #named { animation: pop 100ms; }
      #forever { animation: pop 100ms infinite; }
    </style>
  )html";
};

TEST_CASE("onanimationend runs once an animation finishes", "[animation]") {
  FakeClock clock;
  auto app = Ref<Ending>::New();
  auto device = std::make_shared<MockTerminalDevice>();
  device->TriggerResize(10, 3);
  Screen screen(app, device);
  screen.Draw();

  g_now += 50;
  screen.Step();
  CHECK(app->ended == 0);
  CHECK(app->last_arg.empty());

  g_now += 100;
  screen.Step();
  CHECK(app->ended == 1);  // #once, not #forever.
  CHECK(app->last_arg == "pop");

  g_now += 1000;
  screen.Step();
  CHECK(app->ended == 1);
}

class QuickSlide : public Component<QuickSlide> {
 public:
  std::string_view view = R"html(
    <div id="box">Hi</div>
    <style>
      @keyframes slide { from { right: -10; } }
      #box { position: fixed; top: 0; right: 2; animation: slide 50ms; }
    </style>
  )html";
};

TEST_CASE("A headless screen shows the frame an animation ends on",
          "[animation]") {
  HeadlessScreen screen(Ref<QuickSlide>::New(), 10, 1);
  CHECK(screen.Text() == "      Hi\n");
}

class Hidden : public Component<Hidden> {
 public:
  std::string box_class = "hidden";
  Hidden() { Bind(box_class); }
  std::string_view view = R"html(
    <div id="box" class="{box_class}">Hi</div>
    <style>
      @keyframes fade { from { opacity: 0; } }
      #box { animation: fade 1s linear; }
      .hidden { display: none; }
    </style>
  )html";
};

TEST_CASE("An animation starts when its element is displayed", "[animation]") {
  FakeClock clock;
  auto app = Ref<Hidden>::New();
  app->Mount();
  Element* box = app->Root()->QuerySelector("#box");
  REQUIRE(box != nullptr);

  // Hidden for a while: nothing plays.
  CHECK_FALSE(box->HasPlayingAnimations());
  clock.Advance(box, 2000);

  app->box_class = "";
  app->Digest();
  CHECK(box->style.opacity == Catch::Approx(0.0f));
  clock.Advance(box, 500);
  CHECK(box->style.opacity == Catch::Approx(0.5f));

  // Hiding it stops it; showing it again starts it over.
  app->box_class = "hidden";
  app->Digest();
  CHECK_FALSE(box->HasPlayingAnimations());
  app->box_class = "";
  app->Digest();
  CHECK(box->style.opacity == Catch::Approx(0.0f));
}

class MixedUnits : public Component<MixedUnits> {
 public:
  std::string_view view = R"html(
    <div id="box">Hi</div>
    <style>
      @keyframes move { from { right: 50%; } to { right: 10; } }
      #box { position: fixed; animation: move 1s linear; }
    </style>
  )html";
};

TEST_CASE("A length animates between cells and percents", "[animation]") {
  FakeClock clock;
  auto app = Ref<MixedUnits>::New();
  app->Mount();
  Element* box = app->Root()->QuerySelector("#box");
  REQUIRE(box != nullptr);

  CHECK(box->style.right.Resolve(20) == 10);  // 50% of 20.
  clock.Advance(box, 500);
  // Halfway: 25% + 5, not a jump at the end.
  CHECK(box->style.right == Length::MakeCalc(5, 25));
  CHECK(box->style.right.Resolve(20) == 10);
  clock.Advance(box, 250);
  CHECK(box->style.right.Resolve(40) == 12);  // 12.5% of 40, + 7.5.
}

class SlideOwnWidth : public Component<SlideOwnWidth> {
 public:
  std::string_view view = R"html(
    <div id="box">ABCD</div>
    <style>
      @keyframes in { from { translate: calc(100% + 2); } }
      #box { width: 4; animation: in 1s linear; }
    </style>
  )html";
};

TEST_CASE("translate animates by the box's own size", "[animation]") {
  FakeClock clock;
  auto app = Ref<SlideOwnWidth>::New();
  auto device = std::make_shared<MockTerminalDevice>();
  device->TriggerResize(10, 1);
  Screen screen(app, device);
  screen.Draw();
  CHECK(screen.Text() == "      ABCD\n");  // 4 + 2 to the right.
  g_now += 500;
  screen.Step();
  CHECK(screen.Text() == "   ABCD\n");
  g_now += 600;
  screen.Step();
  CHECK(screen.Text() == "ABCD\n");
}

class SlideOnClass : public Component<SlideOnClass> {
 public:
  std::string box_class;
  SlideOnClass() { Bind(box_class); }
  std::string_view view = R"html(
    <div id="box" class="{box_class}">Hi</div>
    <style>
      #box { transition: translate 1s linear; }
      #box.away { translate: 0 100%; }
    </style>
  )html";
};

TEST_CASE("A transition moves an element by its translate", "[animation]") {
  FakeClock clock;
  auto app = Ref<SlideOnClass>::New();
  app->Mount();
  Element* box = app->Root()->QuerySelector("#box");
  REQUIRE(box != nullptr);

  app->box_class = "away";
  app->Digest();
  clock.Advance(box, 500);
  CHECK(box->style.translate_y == Length::MakeCalc(0, 50));
  clock.Advance(box, 600);
  CHECK(box->style.translate_y == Length::Pct(100));
}

class Short : public Component<Short> {
 public:
  std::string_view view = R"html(
    <div id="box">Hi</div>
    <style>
      @keyframes fade { from { opacity: 0; } }
      #box { animation: fade 200ms linear 100ms; }
    </style>
  )html";
};

TEST_CASE("An animation is sampled exactly at its delay and its end",
          "[animation]") {
  FakeClock clock;
  auto app = Ref<Short>::New();
  app->Mount();
  Element* box = app->Root()->QuerySelector("#box");
  REQUIRE(box != nullptr);

  // 0.1f and 0.2f seconds are not exact: read as they are, the animation
  // started late and ended late.
  clock.Advance(box, 200);
  CHECK(box->style.opacity == Catch::Approx(0.5f));
  clock.Advance(box, 100);
  CHECK_FALSE(box->HasPlayingAnimations());
}

}  // namespace
}  // namespace rtxui
