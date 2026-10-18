// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/dialog/dialog.hpp"

#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>

#include "rtxui/component.hpp"
#include "rtxui/component/component_internal.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/event.hpp"
#include "rtxui/refcounted.hpp"
#include "rtxui/screen.hpp"
#include "rtxui/terminal/event_internal.hpp"
#include "rtxui/terminal/screen_internal.hpp"
#include "rtxui/terminal/terminal_device.hpp"
#include "rtxui/testing/component_as.hpp"

using namespace rtxui;

namespace {

// --- Dialog ---
class DialogTestComponent : public rtxui::Component<DialogTestComponent> {
 public:
  bool dialog_open = false;

  DialogTestComponent() { Bind(dialog_open); }
  std::string_view view = R"(
    <dialog open="{dialog_open}" title="Confirm Dialog">
      Body Content
      <button id="dlg-btn">Confirm</button>
    </dialog>
  )";
};

}  // namespace

TEST_CASE("Dialog Component Overlay", "[component][dialog]") {
  auto container = rtxui::Ref<DialogTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* dialog_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("dialog");
  REQUIRE(dialog_el != nullptr);
  auto* dialog_ptr = ComponentAs<rtxui::dialog>(dialog_el);
  REQUIRE(dialog_ptr != nullptr);

  // Initial State: closed
  CHECK(dialog_ptr->open == false);
  CHECK(dialog_ptr->overlay_class == "closed");

  // Open dialog
  container->dialog_open = true;
  container->Digest();
  screen.Draw();

  CHECK(dialog_ptr->open == true);
  CHECK(dialog_ptr->overlay_class == "open");

  // Verify the nested button is present and queryable inside dialog slot
  auto* btn_el = dialog_el->QuerySelector("#dlg-btn");
  REQUIRE(btn_el != nullptr);
  CHECK(btn_el->tag() == "button");

  // Escape cancels the open dialog (and must not fall through to the
  // screen's Escape-to-quit handling).
  screen.Dispatch(Event::Escape());
  CHECK(dialog_ptr->open == false);
  CHECK(dialog_ptr->overlay_class == "closed");
}

namespace {

class DialogClickTestComponent
    : public rtxui::Component<DialogClickTestComponent> {
 public:
  bool dialog_open = true;
  int confirmed = 0;

  void Confirm() { confirmed++; }

  DialogClickTestComponent() {
    Bind(dialog_open);
    Bind(Confirm);
  }
  // The wrapper matters: it offsets the dialog's parent away from the
  // viewport origin, which is what made the overlay's contents hit-test in
  // the wrong place.
  std::string_view view = R"(
    <div class="main">
      <div>filler</div>
      <dialog open="{dialog_open}" title="Confirm">
        <button id="ok" onclick="Confirm">Confirm</button>
      </dialog>
    </div>
    <style>
      .main { padding: 1; width: 40; height: 6; }
    </style>
  )";
};

}  // namespace

TEST_CASE("Dialog: a button inside an open dialog is clickable",
          "[component][dialog]") {
  // Regression: the overlay is `position: fixed`, so hit testing has to reach
  // it through a separate path. Clicks on controls inside the dialog were
  // landing nowhere, leaving the dialog unusable once opened.
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  device->TriggerResize(60, 20);

  auto container = rtxui::Ref<DialogClickTestComponent>::New();
  rtxui::Screen screen = rtxui::ScreenInternals::Make(container, device);
  container->Digest();
  screen.Draw();

  auto* btn_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("#ok");
  REQUIRE(btn_el != nullptr);
  // The button must actually have been laid out somewhere on screen.
  REQUIRE(btn_el->layout_width() > 0);

  MouseData mouse;
  mouse.button = Event::Mouse::Button::Left;
  mouse.motion = Event::Mouse::Motion::Pressed;
  // Dispatch coordinates are 1-based; aim at the middle of the button.
  mouse.x = btn_el->absolute_x() + btn_el->layout_width() / 2 + 1;
  mouse.y = btn_el->absolute_y() + 1;

  screen.Dispatch(Event(mouse));
  mouse.motion = Event::Mouse::Motion::Released;
  screen.Dispatch(Event(mouse));

  CHECK(container->confirmed == 1);
}

TEST_CASE("Dialog Component Layout Centering", "[component][dialog][layout]") {
  auto container = rtxui::Ref<DialogTestComponent>::New();
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  device->TriggerResize(80, 24);
  rtxui::Screen screen = rtxui::ScreenInternals::Make(container, device);

  container->dialog_open = true;
  container->Digest();
  screen.Draw();

  auto* overlay_el = rtxui::ComponentInternals::Root(*container)
                         ->QuerySelector(".dialog-overlay");
  auto* box_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector(".dialog-box");
  REQUIRE(overlay_el != nullptr);
  REQUIRE(box_el != nullptr);

  CHECK(overlay_el->layout_width() == 80);
  CHECK(overlay_el->layout_height() == 24);

  int box_w = box_el->layout_width();
  int box_h = box_el->layout_height();
  REQUIRE(box_w > 0);
  REQUIRE(box_h > 0);

  int expected_x = (80 - box_w) / 2;
  int expected_y = (24 - box_h) / 2;

  CHECK(box_el->absolute_x() == expected_x);
  CHECK(box_el->absolute_y() == expected_y);
}

TEST_CASE(
    "Dialog Component Exposes Part Attributes For External "
    "::part() Styling",
    "[component][dialog][part]") {
  auto container = rtxui::Ref<DialogTestComponent>::New();
  rtxui::Screen screen(container);

  container->dialog_open = true;
  container->Digest();
  screen.Draw();

  auto* dialog_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("dialog");
  REQUIRE(dialog_el != nullptr);

  auto* overlay_el = dialog_el->QuerySelector(".dialog-overlay");
  REQUIRE(overlay_el != nullptr);
  const std::string* overlay_part = overlay_el->GetAttribute("part");
  REQUIRE(overlay_part != nullptr);
  CHECK(*overlay_part == "dialog-overlay");

  auto* box_el = dialog_el->QuerySelector(".dialog-box");
  REQUIRE(box_el != nullptr);
  const std::string* box_part = box_el->GetAttribute("part");
  REQUIRE(box_part != nullptr);
  CHECK(*box_part == "dialog-box");

  auto* header_el = dialog_el->QuerySelector(".dialog-header");
  REQUIRE(header_el != nullptr);
  const std::string* header_part = header_el->GetAttribute("part");
  REQUIRE(header_part != nullptr);
  CHECK(*header_part == "dialog-header");

  auto* title_el = dialog_el->QuerySelector(".dialog-title");
  REQUIRE(title_el != nullptr);
  const std::string* title_part = title_el->GetAttribute("part");
  REQUIRE(title_part != nullptr);
  CHECK(*title_part == "dialog-title");

  auto* body_el = dialog_el->QuerySelector(".dialog-body");
  REQUIRE(body_el != nullptr);
  const std::string* body_part = body_el->GetAttribute("part");
  REQUIRE(body_part != nullptr);
  CHECK(*body_part == "dialog-body");
}

namespace {

// A page with a list and a dialog. Its root, like any component's, is an
// inline box around them.
class DialogPageTestComponent
    : public rtxui::Component<DialogPageTestComponent> {
 public:
  std::string_view view = R"(
    <div class="list">one<br/>two<br/>three</div>
    <dialog open="true" title="Confirm"><p id="body">Sure?</p></dialog>
    <style>
      .list { display: flex; flex-direction: column; width: 20; }
    </style>
  )";
};

class DialogInPageTestComponent
    : public rtxui::Component<DialogInPageTestComponent> {
 public:
  DialogInPageTestComponent() { Import<DialogPageTestComponent>(); }
  std::string_view view = R"(
    <div class="app">
      <div class="side">side</div>
      <div class="main">
        <p>Title</p>
        <div class="page"><DialogPageTestComponent /></div>
      </div>
    </div>
    <style>
      .app { display: flex; width: 100%; height: 100%; }
      .side { width: 10; }
      .main { flex-grow: 1; padding: 1 2; }
    </style>
  )";
};

}  // namespace

TEST_CASE("Dialog: the overlay covers the screen from inside a component",
          "[component][dialog]") {
  // The overlay is `position: fixed`, so wherever the dialog sits it must be
  // placed against the screen. From a component nested in a page, it was
  // shifted by where the page started.
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  device->TriggerResize(60, 20);
  auto container = rtxui::Ref<DialogInPageTestComponent>::New();
  rtxui::Screen screen = rtxui::ScreenInternals::Make(container, device);
  container->Digest();
  screen.Draw();

  auto* overlay = rtxui::ComponentInternals::Root(*container)
                      ->QuerySelector(".dialog-overlay");
  REQUIRE(overlay != nullptr);
  CHECK(overlay->absolute_x() == 0);
  CHECK(overlay->absolute_y() == 0);
  CHECK(overlay->layout_width() == 60);
  CHECK(overlay->layout_height() == 20);
}
