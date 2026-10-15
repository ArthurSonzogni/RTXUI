// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/tooltip/tooltip.hpp"

#include <catch2/catch_test_macros.hpp>
#include <string>
#include <string_view>

#include "rtxui/component.hpp"
#include "rtxui/component/component_internal.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/layout/style.hpp"
#include "rtxui/refcounted.hpp"
#include "rtxui/screen.hpp"
#include "rtxui/testing/component_as.hpp"

using namespace rtxui;

namespace {

// --- Tooltip ---
class TooltipTestComponent : public rtxui::Component<TooltipTestComponent> {
 public:
  std::string tooltip_text = "Helpful Info";
  std::string tooltip_place = "top";

  TooltipTestComponent() {
    Bind(tooltip_text);
    Bind(tooltip_place);
  }
  std::string_view view = R"(
    <div>
      <tooltip id="tt" content="{tooltip_text}" placement="{tooltip_place}">
        <div id="trigger">Hover Me</div>
      </tooltip>
    </div>
  )";
};

}  // namespace

TEST_CASE("Tooltip Component Interaction", "[component][tooltip]") {
  auto container = rtxui::Ref<TooltipTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* tt_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("#tt");
  auto* trigger_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("#trigger");
  REQUIRE(tt_el != nullptr);
  REQUIRE(trigger_el != nullptr);

  auto* tt_ptr = ComponentAs<rtxui::tooltip>(tt_el);
  REQUIRE(tt_ptr != nullptr);

  // Initial state: not hovered, class is hidden
  CHECK(tt_ptr->tooltip_class == "hidden");

  // Simulated hover
  tt_el->set_hovered(true);
  container->Digest();
  screen.Draw();

  // Verify class is now visible
  CHECK(tt_ptr->tooltip_class == "visible");

  // Hover removed
  tt_el->set_hovered(false);
  container->Digest();
  screen.Draw();

  // Verify class is hidden again
  CHECK(tt_ptr->tooltip_class == "hidden");
}

TEST_CASE(
    "Tooltip Component Exposes Part Attributes For External "
    "::part() Styling",
    "[component][tooltip][part]") {
  auto container = rtxui::Ref<TooltipTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* tt_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("#tt");
  REQUIRE(tt_el != nullptr);

  auto* container_el = tt_el->QuerySelector(".tooltip-container");
  REQUIRE(container_el != nullptr);
  const std::string* container_part = container_el->GetAttribute("part");
  REQUIRE(container_part != nullptr);
  CHECK(*container_part == "tooltip-container");

  auto* trigger_el = tt_el->QuerySelector(".tooltip-trigger");
  REQUIRE(trigger_el != nullptr);
  const std::string* trigger_part = trigger_el->GetAttribute("part");
  REQUIRE(trigger_part != nullptr);
  CHECK(*trigger_part == "tooltip-trigger");

  auto* popup_el = tt_el->QuerySelector(".tooltip-popup");
  REQUIRE(popup_el != nullptr);
  const std::string* popup_part = popup_el->GetAttribute("part");
  REQUIRE(popup_part != nullptr);
  CHECK(*popup_part == "tooltip-popup");
}

TEST_CASE("Tooltip Placement Styles", "[component][tooltip][alignment]") {
  auto container = rtxui::Ref<TooltipTestComponent>::New();
  rtxui::Screen screen(container);

  auto* tt_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("#tt");
  REQUIRE(tt_el != nullptr);

  tt_el->set_hovered(true);

  auto check_placement = [&](const std::string& place, auto verify_fn) {
    container->tooltip_place = place;
    container->Digest();
    screen.Draw();

    auto* popup = rtxui::ComponentInternals::Root(*container)
                      ->QuerySelector(".tooltip-popup");
    REQUIRE(popup != nullptr);
    verify_fn(popup->style);
  };

  SECTION("Top Placements") {
    check_placement("top", [](const rtxui::ComputedStyle& style) {
      CHECK(style.bottom.unit == rtxui::Unit::Percent);
      CHECK(style.bottom.value == 100.0f);
      CHECK(style.left.unit == rtxui::Unit::Cells);
      CHECK(style.left.value == 0.0f);
      CHECK(style.right.unit == rtxui::Unit::Cells);
      CHECK(style.right.value == 0.0f);
      CHECK(style.margin_left_auto == true);
      CHECK(style.margin_right_auto == true);
    });

    check_placement("top-start", [](const rtxui::ComputedStyle& style) {
      CHECK(style.bottom.unit == rtxui::Unit::Percent);
      CHECK(style.bottom.value == 100.0f);
      CHECK(style.left.unit == rtxui::Unit::Cells);
      CHECK(style.left.value == 0.0f);
      CHECK(style.right.unit == rtxui::Unit::Auto);
      CHECK(style.margin_left_auto == false);
      CHECK(style.margin_right_auto == true);
    });

    check_placement("top-end", [](const rtxui::ComputedStyle& style) {
      CHECK(style.bottom.unit == rtxui::Unit::Percent);
      CHECK(style.bottom.value == 100.0f);
      CHECK(style.left.unit == rtxui::Unit::Auto);
      CHECK(style.right.unit == rtxui::Unit::Cells);
      CHECK(style.right.value == 0.0f);
      CHECK(style.margin_left_auto == true);
      CHECK(style.margin_right_auto == false);
    });
  }

  SECTION("Bottom Placements") {
    check_placement("bottom", [](const rtxui::ComputedStyle& style) {
      CHECK(style.top.unit == rtxui::Unit::Percent);
      CHECK(style.top.value == 100.0f);
      CHECK(style.left.unit == rtxui::Unit::Cells);
      CHECK(style.left.value == 0.0f);
      CHECK(style.right.unit == rtxui::Unit::Cells);
      CHECK(style.right.value == 0.0f);
      CHECK(style.margin_left_auto == true);
      CHECK(style.margin_right_auto == true);
    });

    check_placement("bottom-start", [](const rtxui::ComputedStyle& style) {
      CHECK(style.top.unit == rtxui::Unit::Percent);
      CHECK(style.top.value == 100.0f);
      CHECK(style.left.unit == rtxui::Unit::Cells);
      CHECK(style.left.value == 0.0f);
      CHECK(style.right.unit == rtxui::Unit::Auto);
      CHECK(style.margin_left_auto == false);
      CHECK(style.margin_right_auto == true);
    });

    check_placement("bottom-end", [](const rtxui::ComputedStyle& style) {
      CHECK(style.top.unit == rtxui::Unit::Percent);
      CHECK(style.top.value == 100.0f);
      CHECK(style.left.unit == rtxui::Unit::Auto);
      CHECK(style.right.unit == rtxui::Unit::Cells);
      CHECK(style.right.value == 0.0f);
      CHECK(style.margin_left_auto == true);
      CHECK(style.margin_right_auto == false);
    });
  }

  SECTION("Left Placements") {
    check_placement("left", [](const rtxui::ComputedStyle& style) {
      CHECK(style.right.unit == rtxui::Unit::Percent);
      CHECK(style.right.value == 100.0f);
      CHECK(style.top.unit == rtxui::Unit::Cells);
      CHECK(style.top.value == 0.0f);
      CHECK(style.bottom.unit == rtxui::Unit::Cells);
      CHECK(style.bottom.value == 0.0f);
      CHECK(style.margin_top_auto == true);
      CHECK(style.margin_bottom_auto == true);
    });

    check_placement("left-start", [](const rtxui::ComputedStyle& style) {
      CHECK(style.right.unit == rtxui::Unit::Percent);
      CHECK(style.right.value == 100.0f);
      CHECK(style.top.unit == rtxui::Unit::Cells);
      CHECK(style.top.value == 0.0f);
      CHECK(style.bottom.unit == rtxui::Unit::Auto);
      CHECK(style.margin_top_auto == false);
      CHECK(style.margin_bottom_auto == true);
    });

    check_placement("left-end", [](const rtxui::ComputedStyle& style) {
      CHECK(style.right.unit == rtxui::Unit::Percent);
      CHECK(style.right.value == 100.0f);
      CHECK(style.bottom.unit == rtxui::Unit::Cells);
      CHECK(style.bottom.value == 0.0f);
      CHECK(style.top.unit == rtxui::Unit::Auto);
      CHECK(style.margin_top_auto == true);
      CHECK(style.margin_bottom_auto == false);
    });
  }

  SECTION("Right Placements") {
    check_placement("right", [](const rtxui::ComputedStyle& style) {
      CHECK(style.left.unit == rtxui::Unit::Percent);
      CHECK(style.left.value == 100.0f);
      CHECK(style.top.unit == rtxui::Unit::Cells);
      CHECK(style.top.value == 0.0f);
      CHECK(style.bottom.unit == rtxui::Unit::Cells);
      CHECK(style.bottom.value == 0.0f);
      CHECK(style.margin_top_auto == true);
      CHECK(style.margin_bottom_auto == true);
    });

    check_placement("right-start", [](const rtxui::ComputedStyle& style) {
      CHECK(style.left.unit == rtxui::Unit::Percent);
      CHECK(style.left.value == 100.0f);
      CHECK(style.top.unit == rtxui::Unit::Cells);
      CHECK(style.top.value == 0.0f);
      CHECK(style.bottom.unit == rtxui::Unit::Auto);
      CHECK(style.margin_top_auto == false);
      CHECK(style.margin_bottom_auto == true);
    });

    check_placement("right-end", [](const rtxui::ComputedStyle& style) {
      CHECK(style.left.unit == rtxui::Unit::Percent);
      CHECK(style.left.value == 100.0f);
      CHECK(style.bottom.unit == rtxui::Unit::Cells);
      CHECK(style.bottom.value == 0.0f);
      CHECK(style.top.unit == rtxui::Unit::Auto);
      CHECK(style.margin_top_auto == true);
      CHECK(style.margin_bottom_auto == false);
    });
  }
}
