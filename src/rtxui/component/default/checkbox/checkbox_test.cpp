// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/checkbox/checkbox.hpp"

#include <catch2/catch_test_macros.hpp>
#include <optional>
#include <string>
#include <string_view>

#include "rtxui/color.hpp"
#include "rtxui/component.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/event.hpp"
#include "rtxui/layout/style.hpp"
#include "rtxui/refcounted.hpp"
#include "rtxui/testing/component_as.hpp"

using namespace rtxui;

namespace {

class CheckboxTestComponent : public rtxui::Component<CheckboxTestComponent> {
 public:
  bool my_checked = false;
  bool onchange_called = false;

  CheckboxTestComponent() {
    Bind(my_checked);
    Import("ToggleEnabled", [this]() { onchange_called = true; });
  }
  std::string_view view = R"(
    <checkbox checked="{my_checked}" onchange="ToggleEnabled">Click Me</checkbox>
  )";
};

TEST_CASE("Checkbox Component Basic Interactions", "[component][checkbox]") {
  auto container = rtxui::Ref<CheckboxTestComponent>::New();
  container->Mount();

  auto* cb_el = container->Root()->QuerySelector("checkbox");
  REQUIRE(cb_el != nullptr);
  auto* cb_ptr = ComponentAs<rtxui::checkbox>(cb_el);
  REQUIRE(cb_ptr != nullptr);

  // Initial state
  CHECK(cb_ptr->checked == false);
  CHECK(container->my_checked == false);
  CHECK(container->onchange_called == false);

  // Focus and trigger space
  cb_el->set_focused(true);
  cb_ptr->OnEvent(Event::Keyboard::From(' '));  // Space
  container->Digest();

  CHECK(cb_ptr->checked == true);
  CHECK(container->my_checked == true);
  CHECK(container->onchange_called == true);

  // Press space again
  container->onchange_called = false;
  cb_ptr->OnEvent(Event::Keyboard::From(' '));
  container->Digest();

  CHECK(cb_ptr->checked == false);
  CHECK(container->my_checked == false);
  CHECK(container->onchange_called == true);
}

class CheckboxCheckedStyleComponent
    : public rtxui::Component<CheckboxCheckedStyleComponent> {
 public:
  bool my_checked = false;

  CheckboxCheckedStyleComponent() { Bind(my_checked); }
  std::string_view view = R"(
    <checkbox checked="{my_checked}">Option</checkbox>
    <style>
      checkbox {
        color: white;
      }
      checkbox:checked {
        color: green;
      }
    </style>
  )";
};

TEST_CASE("Checkbox Component :checked CSS Pseudo-Class",
          "[component][checkbox][checked]") {
  auto container = rtxui::Ref<CheckboxCheckedStyleComponent>::New();
  container->Mount();

  auto* cb_el = container->Root()->QuerySelector("checkbox");
  REQUIRE(cb_el != nullptr);
  auto* cb_ptr = ComponentAs<rtxui::checkbox>(cb_el);
  REQUIRE(cb_ptr != nullptr);

  // Initial state: unchecked -> white
  container->Digest();
  CHECK(cb_el->style.foreground_color == Color::RGB(255, 255, 255));

  // Check the checkbox -> green
  container->my_checked = true;
  container->Digest();
  CHECK(cb_el->style.foreground_color == Color::RGB(0, 255, 0));

  // Uncheck the checkbox -> white
  container->my_checked = false;
  container->Digest();
  CHECK(cb_el->style.foreground_color == Color::RGB(255, 255, 255));
}

TEST_CASE(
    "Checkbox Component Exposes Checkmark Part Attribute For External "
    "::part() Styling",
    "[component][checkbox][part]") {
  auto container = rtxui::Ref<CheckboxTestComponent>::New();
  container->Mount();

  auto* cb_el = container->Root()->QuerySelector("checkbox");
  REQUIRE(cb_el != nullptr);

  auto* checkmark_el = cb_el->QuerySelector(".checkmark");
  REQUIRE(checkmark_el != nullptr);
  const std::string* part = checkmark_el->GetAttribute("part");
  REQUIRE(part != nullptr);
  CHECK(*part == "checkmark");
}

}  // namespace
