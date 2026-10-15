// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.

#include <catch2/catch_test_macros.hpp>
#include <string_view>

#include "rtxui/component.hpp"
#include "rtxui/component/component_internal.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/event.hpp"
#include "rtxui/refcounted.hpp"
#include "rtxui/screen.hpp"

using namespace rtxui;

namespace {

// --- Label ---
class LabelTestComponent : public rtxui::Component<LabelTestComponent> {
 public:
  bool cb1_checked = false;
  bool cb2_checked = false;

  LabelTestComponent() {
    Bind(cb1_checked);
    Bind(cb2_checked);
  }
  std::string_view view = R"(
    <div>
      <!-- Explicit association via for -->
      <label id="lbl1" for="cb1">Explicit Label</label>
      <checkbox id="cb1" checked="{cb1_checked}">CB1</checkbox>

      <!-- Implicit association via nesting -->
      <label id="lbl2">
        Implicit Label
        <checkbox id="cb2" checked="{cb2_checked}">CB2</checkbox>
      </label>
    </div>
  )";
};

}  // namespace

TEST_CASE("Label Component Interaction", "[component][label]") {
  auto container = rtxui::Ref<LabelTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* lbl1_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("#lbl1");
  auto* cb1_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("#cb1");
  auto* lbl2_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("#lbl2");
  auto* cb2_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("#cb2");

  REQUIRE(lbl1_el != nullptr);
  REQUIRE(cb1_el != nullptr);
  REQUIRE(lbl2_el != nullptr);
  REQUIRE(cb2_el != nullptr);

  auto* lbl1_comp = const_cast<rtxui::ComponentBase*>(lbl1_el->component());
  auto* lbl2_comp = const_cast<rtxui::ComponentBase*>(lbl2_el->component());
  REQUIRE(lbl1_comp != nullptr);
  REQUIRE(lbl2_comp != nullptr);

  // Initial state: unchecked
  CHECK(container->cb1_checked == false);
  CHECK(container->cb2_checked == false);

  // 1. Click on Explicit Label (lbl1)
  Event::Mouse mouse1;
  mouse1.button = Event::Mouse::Button::Left;
  mouse1.motion = Event::Mouse::Motion::Pressed;
  mouse1.x = lbl1_el->absolute_x() + 1;
  mouse1.y = lbl1_el->absolute_y() + 1;
  Event click1(mouse1);

  CHECK(lbl1_comp->OnEvent(click1) == true);
  container->Digest();
  screen.Draw();

  // Verify cb1 toggled, cb2 unchanged, cb1 focused!
  CHECK(container->cb1_checked == true);
  CHECK(container->cb2_checked == false);
  CHECK(cb1_el->focused() == true);

  // 2. Click on Implicit Label (lbl2)
  Event::Mouse mouse2;
  mouse2.button = Event::Mouse::Button::Left;
  mouse2.motion = Event::Mouse::Motion::Pressed;
  mouse2.x = lbl2_el->absolute_x() + 1;
  mouse2.y = lbl2_el->absolute_y() + 1;
  Event click2(mouse2);

  CHECK(lbl2_comp->OnEvent(click2) == true);
  container->Digest();
  screen.Draw();

  // Verify cb2 toggled, cb2 focused!
  CHECK(container->cb1_checked == true);
  CHECK(container->cb2_checked == true);
  CHECK(cb2_el->focused() == true);
}

namespace {

class DisabledLabelTestComponent
    : public rtxui::Component<DisabledLabelTestComponent> {
 public:
  bool cb_checked = false;
  DisabledLabelTestComponent() { Bind(cb_checked); }
  std::string_view view = R"(
    <label id="lbl" for="cb">Label</label>
    <checkbox id="cb" checked="{cb_checked}" disabled="true">CB</checkbox>
  )";
};

}  // namespace

TEST_CASE("Clicking a label doesn't check or focus its disabled target",
          "[component][label][disabled][regression]") {
  // label::OnEvent doesn't itself check target_el->disabled() before
  // focusing and triggering it - it relies on the target's own OnEvent
  // (which no-ops while disabled, see the disabled=\"\" work earlier this
  // session) and its own `parent->Digest()` call at the end (which
  // cascades into the target's Digest(), re-running SyncDisabled and
  // dropping the focus it had just set) to self-correct within the same
  // synchronous call. Pin that down explicitly so a future refactor that
  // drops the seemingly-redundant Digest() call doesn't quietly leave a
  // disabled checkbox focused.
  auto container = rtxui::Ref<DisabledLabelTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* lbl_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("#lbl");
  auto* cb_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("#cb");
  auto* lbl_comp = const_cast<rtxui::ComponentBase*>(lbl_el->component());

  Event::Mouse mouse;
  mouse.button = Event::Mouse::Button::Left;
  mouse.motion = Event::Mouse::Motion::Pressed;
  mouse.x = lbl_el->absolute_x() + 1;
  mouse.y = lbl_el->absolute_y() + 1;
  lbl_comp->OnEvent(Event(mouse));

  CHECK_FALSE(container->cb_checked);
  CHECK_FALSE(cb_el->focused());
}
