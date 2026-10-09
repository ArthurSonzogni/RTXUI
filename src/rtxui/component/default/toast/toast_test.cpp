// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/toast/toast.hpp"

#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

#include "rtxui/component.hpp"
#include "rtxui/diagnostic.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/headless.hpp"
#include "rtxui/task.hpp"

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
  TimelineScreen timeline(app, 30, 6);
  CHECK_FALSE(ToastShown(*app));

  PostTask([&] { app->saved = true; });
  timeline.Advance(0);
  CHECK(ToastShown(*app));

  // Still there just before its 40ms are up...
  timeline.Advance(39);
  CHECK(ToastShown(*app));
  CHECK(app->saved);

  // ...closing once they are, and the application's flag follows at once...
  timeline.Advance(1);
  CHECK_FALSE(app->saved);
  CHECK(ToastShown(*app));

  // ...gone once it has slid out.
  timeline.Advance(250);
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

TEST_CASE("A toast slides in from the side it sits on", "[component][toast]") {
  {
    TimelineScreen right(Ref<Placed>::New(), 20, 4);
    CHECK(right.Text() == "\n\n\n\n");  // Out of sight at first.
    // Partway: cut by the edge of the screen. It travels its own width and
    // the 2 cells it sits from the edge, so it shows from the first frames.
    CHECK(right.Advance(60) ==
          "              ▊▔▔▔▔▔\n"
          "              ▊ Save\n"
          "              ▊▁▁▁▁▁\n"
          "\n");
    CHECK(right.Advance(340) ==
          "         ▊▔▔▔▔▔▔▔▎\n"
          "         ▊ Saved ▎\n"
          "         ▊▁▁▁▁▁▁▁▎\n"
          "\n");
  }

  auto app = Ref<Placed>::New();
  app->placement = "bottom-left";
  TimelineScreen left(app, 20, 4);
  CHECK(left.Advance(60) ==
        "▔▔▔▔▔▎\n"
        "aved ▎\n"
        "▁▁▁▁▁▎\n"
        "\n");
}

TEST_CASE("A closing toast slides out before it hides", "[component][toast]") {
  auto app = Ref<Placed>::New();
  TimelineScreen screen(app, 20, 4);
  screen.Advance(400);
  const Element* box = app->Root()->QuerySelector(".toast");
  REQUIRE(box != nullptr);

  PostTask([&] { app->shown = false; });
  screen.Advance(0);
  CHECK(*box->GetAttribute("part") == "toast closing");
  CHECK(screen.Advance(160) ==
        "              ▊▔▔▔▔▔\n"
        "              ▊ Save\n"
        "              ▊▁▁▁▁▁\n"
        "\n");
  CHECK(screen.Advance(60) == "\n\n\n\n");
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
  auto app = Ref<Faded>::New();
  TimelineScreen screen(app, 20, 4);
  const std::string open = screen.Advance(400);
  const Element* box = app->Root()->QuerySelector(".toast");
  REQUIRE(box != nullptr);

  PostTask([&] { app->shown = false; });
  // Fading, in place: no slide.
  screen.Advance(0);
  CHECK(screen.Advance(150) == open);
  CHECK(box->style.opacity < 1.0f);
  CHECK(screen.Advance(200) == "\n\n\n\n");
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
  std::vector<std::string> messages;
  SetDiagnosticHandler(
      [&](const Diagnostic& d) { messages.push_back(d.message); });
  auto app = Ref<Restyled2>::New();
  TimelineScreen screen(app, 20, 4);
  screen.Advance(400);
  PostTask([&] { app->shown = false; });
  screen.Advance(0);
  screen.Advance(150);
  SetDiagnosticHandler(nullptr);
  CHECK(messages.empty());
  const Element* box = app->Root()->QuerySelector(".toast");
  REQUIRE(box != nullptr);
  CHECK(box->style.opacity < 1.0f);
}

}  // namespace
}  // namespace rtxui
