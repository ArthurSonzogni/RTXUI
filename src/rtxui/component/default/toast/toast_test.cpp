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

  // ...gone after, and the application's flag follows.
  std::this_thread::sleep_for(std::chrono::milliseconds(60));
  screen.Step();
  CHECK_FALSE(ToastShown(*app));
  CHECK_FALSE(app->saved);
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

}  // namespace
}  // namespace rtxui
