// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/toast/toast.hpp"

#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "rtxui/component.hpp"
#include "rtxui/diagnostic.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/headless.hpp"
#include "rtxui/screen.hpp"
#include "rtxui/task.hpp"
#include "rtxui/terminal/terminal_device.hpp"

namespace rtxui {
namespace {

class Saver : public Component<Saver> {
 public:
  bool saved = false;
  int duration = 40;
  Saver() {
    Bind(saved);
    Bind(duration);
  }
  std::string_view view = R"html(
    <div>
      <span>editor</span>
      <toast open="{saved}" duration="{duration}">Saved</toast>
    </div>
  )html";
};

bool ToastShown(Saver& app) {
  const Element* box = app.Root()->QuerySelector(".toast");
  REQUIRE(box != nullptr);
  return !box->style.display_none;
}

TEST_CASE("A toast shows, then closes itself", "[component][toast]") {
  auto app = Ref<Saver>::New();
  auto device = std::make_shared<MockTerminalDevice>();
  device->TriggerResize(30, 6);
  Screen screen(app, device);
  screen.Draw();
  CHECK_FALSE(ToastShown(*app));

  PostTask([&] { app->saved = true; });
  screen.Step();
  CHECK(ToastShown(*app));

  // Still there before its time is up...
  screen.Step();
  CHECK(ToastShown(*app));

  // ...closing after, and the application's flag follows at once...
  std::this_thread::sleep_for(std::chrono::milliseconds(60));
  screen.Step();
  CHECK_FALSE(app->saved);
  CHECK(ToastShown(*app));

  // ...gone once it has slid out.
  std::this_thread::sleep_for(std::chrono::milliseconds(250));
  screen.Step();
  CHECK_FALSE(ToastShown(*app));
}

TEST_CASE("A toast shows in a corner, over the rest", "[component][toast]") {
  auto app = Ref<Saver>::New();
  app->saved = true;
  app->duration = 0;  // Stays until closed.
  HeadlessScreen screen(app, 20, 5);
  CHECK(screen.Text() ==
        "editor\n"
        "         ▊▔▔▔▔▔▔▔▎\n"
        "         ▊ Saved ▎\n"
        "         ▊▁▁▁▁▁▁▁▎\n"
        "\n");
  // Clicking it closes it.
  screen.Click(12, 2);
  CHECK(screen.Text() == "editor\n\n\n\n\n");
  CHECK_FALSE(app->saved);
}

class Misplaced : public Component<Misplaced> {
 public:
  std::string_view view = R"html(
    <toast open="true" placement="middle">x</toast>
  )html";
};

TEST_CASE("An unknown toast placement is reported", "[component][toast]") {
  std::vector<std::string> messages;
  SetDiagnosticHandler(
      [&](const Diagnostic& d) { messages.push_back(d.message); });
  RenderToString(Ref<Misplaced>::New(), 10, 3);
  SetDiagnosticHandler(nullptr);
  REQUIRE_FALSE(messages.empty());
  CHECK(messages[0].find("placement=\"middle\"") != std::string::npos);
}

}  // namespace
}  // namespace rtxui

namespace rtxui {
namespace {

class Stacked : public Component<Stacked> {
 public:
  std::string_view view = R"html(
    <div class="stack">
      <toast open="true" duration="0">one</toast>
      <toast open="true" duration="0">two</toast>
    </div>
    <style>
      .stack { position: fixed; bottom: 0; right: 0; }
      toast::part(toast) { position: static; }
    </style>
  )html";
};

TEST_CASE("Toasts stack in a container of their own", "[component][toast]") {
  CHECK(RenderToString(Ref<Stacked>::New(), 12, 7) ==
        "\n"
        "     ▊▔▔▔▔▔▎\n"
        "     ▊ one ▎\n"
        "     ▊▁▁▁▁▁▎\n"
        "     ▊▔▔▔▔▔▎\n"
        "     ▊ two ▎\n"
        "     ▊▁▁▁▁▁▎\n");
}

class Restyled : public Component<Restyled> {
 public:
  std::string_view view = R"html(
    <toast open="true" duration="0">hi</toast>
    <style>
      toast::part(toast) { position: static; border: round; }
    </style>
  )html";
};

TEST_CASE("A toast's border can be restyled through its part",
          "[component][toast]") {
  CHECK(RenderToString(Ref<Restyled>::New(), 8, 3) ==
        "╭──────╮\n"
        "│ hi   │\n"
        "╰──────╯\n");
}

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
};

class Placed : public Component<Placed> {
 public:
  bool shown = true;
  std::string placement = "bottom-right";
  Placed() {
    Bind(shown);
    Bind(placement);
  }
  std::string_view view = R"html(
    <toast open="{shown}" duration="0" placement="{placement}">Saved</toast>
  )html";
};

struct ToastScreen {
  explicit ToastScreen(Ref<ComponentBase> app)
      : device(std::make_shared<MockTerminalDevice>()) {
    device->TriggerResize(20, 4);
    screen = std::make_unique<Screen>(std::move(app), device);
    screen->Draw();
  }
  std::string At(double ms) {
    g_now += ms;
    screen->Step();
    return screen->Text();
  }
  std::shared_ptr<MockTerminalDevice> device;
  std::unique_ptr<Screen> screen;
};

TEST_CASE("A toast slides in from the side it sits on", "[component][toast]") {
  FakeClock clock;
  ToastScreen right(Ref<Placed>::New());
  CHECK(right.screen->Text() == "\n\n\n\n");  // Out of sight at first.
  // Partway: cut by the edge of the screen.
  CHECK(right.At(150) ==
        "               ▊▔▔▔▔\n"
        "               ▊ Sav\n"
        "               ▊▁▁▁▁\n"
        "\n");
  CHECK(right.At(250) ==
        "         ▊▔▔▔▔▔▔▔▎\n"
        "         ▊ Saved ▎\n"
        "         ▊▁▁▁▁▁▁▁▎\n"
        "\n");

  auto app = Ref<Placed>::New();
  app->placement = "bottom-left";
  ToastScreen left(app);
  CHECK(left.At(150) ==
        "▔▔▔▔▎\n"
        "ved ▎\n"
        "▁▁▁▁▎\n"
        "\n");
}

TEST_CASE("A closing toast slides out before it hides", "[component][toast]") {
  FakeClock clock;
  auto app = Ref<Placed>::New();
  ToastScreen screen(app);
  screen.At(400);
  const Element* box = app->Root()->QuerySelector(".toast");
  REQUIRE(box != nullptr);

  PostTask([&] { app->shown = false; });
  screen.At(0);
  CHECK(*box->GetAttribute("part") == "toast closing");
  CHECK(screen.At(100) ==
        "               ▊▔▔▔▔\n"
        "               ▊ Sav\n"
        "               ▊▁▁▁▁\n"
        "\n");
  CHECK(screen.At(150) == "\n\n\n\n");
  CHECK(box->style.display_none);
  CHECK(*box->GetAttribute("part") == "toast");
}

class Unanimated : public Component<Unanimated> {
 public:
  std::string_view view = R"html(
    <toast open="true" duration="0">hi</toast>
    <style>
      toast::part(toast) { animation: none; }
    </style>
  )html";
};

TEST_CASE("A toast without animations closes at once", "[component][toast]") {
  auto app = Ref<Unanimated>::New();
  HeadlessScreen screen(app, 10, 4);
  CHECK(screen.Text() ==
        "  ▊▔▔▔▔▎\n"
        "  ▊ hi ▎\n"
        "  ▊▁▁▁▁▎\n"
        "\n");
  screen.Click(4, 1);
  CHECK(screen.Text() == "\n\n\n\n");
}

class Faded : public Component<Faded> {
 public:
  bool shown = true;
  Faded() { Bind(shown); }
  std::string_view view = R"html(
    <toast open="{shown}" duration="0">hi</toast>
    <style>
      @keyframes fade-out { to { opacity: 0; } }
      toast::part(closing) { animation: fade-out 300ms forwards; }
    </style>
  )html";
};

TEST_CASE("A toast's closing animation can be replaced", "[component][toast]") {
  FakeClock clock;
  auto app = Ref<Faded>::New();
  ToastScreen screen(app);
  const std::string open = screen.At(400);
  const Element* box = app->Root()->QuerySelector(".toast");
  REQUIRE(box != nullptr);

  PostTask([&] { app->shown = false; });
  // Fading, in place: no slide.
  screen.At(0);
  CHECK(screen.At(150) == open);
  CHECK(box->style.opacity < 1.0f);
  CHECK(screen.At(200) == "\n\n\n\n");
}

class Restyled2 : public Component<Restyled2> {
 public:
  bool shown = true;
  Restyled2() { Bind(shown); }
  std::string_view view = R"html(
    <toast class="error" open="{shown}" duration="0">hi</toast>
    <style>
      @keyframes slide-in { from { right: -50; } }
      @keyframes fade-out { to { opacity: 0; } }
      .error::part(toast) { animation: slide-in 300ms; }
      .error::part(closing) { animation: fade-out 300ms forwards; }
    </style>
  )html";
};

TEST_CASE("A toast's opening and closing animations can both be replaced",
          "[component][toast]") {
  FakeClock clock;
  std::vector<std::string> messages;
  SetDiagnosticHandler(
      [&](const Diagnostic& d) { messages.push_back(d.message); });
  auto app = Ref<Restyled2>::New();
  ToastScreen screen(app);
  screen.At(400);
  PostTask([&] { app->shown = false; });
  screen.At(0);
  screen.At(150);
  SetDiagnosticHandler(nullptr);
  CHECK(messages.empty());
  const Element* box = app->Root()->QuerySelector(".toast");
  REQUIRE(box != nullptr);
  CHECK(box->style.opacity < 1.0f);
}

}  // namespace
}  // namespace rtxui
