// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/details/details.hpp"

#include <catch2/catch_test_macros.hpp>
#include <string>
#include <string_view>

#include "rtxui/component.hpp"
#include "rtxui/component/component_internal.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/dom/text_element.hpp"
#include "rtxui/event.hpp"
#include "rtxui/refcounted.hpp"
#include "rtxui/screen.hpp"
#include "rtxui/testing/component_as.hpp"

using namespace rtxui;

namespace {

class DetailsTestComponent : public rtxui::Component<DetailsTestComponent> {
 public:
  bool open = false;

  DetailsTestComponent() { Bind(open); }
  std::string_view view = R"(
    <details open="{open}">
      <summary>My Title</summary>
      <p>Secret content</p>
    </details>
  )";
};

class DetailsDefaultTestComponent
    : public rtxui::Component<DetailsDefaultTestComponent> {
 public:
  std::string_view view = R"(
    <details>
      <p>Secret content</p>
    </details>
  )";
};

}  // namespace

TEST_CASE("Details and Summary Components", "[component][details]") {
  auto container = rtxui::Ref<DetailsTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* details_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("details");
  REQUIRE(details_el != nullptr);
  auto* details_ptr = ComponentAs<rtxui::details>(details_el);
  REQUIRE(details_ptr != nullptr);

  // 1. Initial State (open is false)
  CHECK(details_ptr->open == false);
  CHECK(details_ptr->arrow_char == "▶");
  CHECK(details_ptr->content_class == "closed");

  // 2. Toggle via program API
  details_ptr->Toggle();
  container->Digest();
  screen.Draw();

  CHECK(details_ptr->open == true);
  CHECK(details_ptr->arrow_char == "▼");
  CHECK(details_ptr->content_class == "open");
  CHECK(container->open == true);

  // 3. Toggle back via program API
  details_ptr->Toggle();
  container->Digest();
  screen.Draw();

  CHECK(details_ptr->open == false);
  CHECK(container->open == false);

  // 4. Toggle via Keyboard Event on focused summary-line
  auto* summary_line_el = rtxui::ComponentInternals::Root(*details_ptr)
                              ->QuerySelector(".summary-line");
  REQUIRE(summary_line_el != nullptr);

  // Reset focus
  rtxui::ComponentInternals::Root(*details_ptr)->Visit([](rtxui::Element& el) {
    el.set_focused(false);
  });
  summary_line_el->set_focused(true);

  // Dispatch return key event
  Event return_event = Event::Keyboard({
      Event::Keyboard::Motion::Pressed,
      Event::Keyboard::Special::Return,
  });
  CHECK(details_ptr->OnEvent(return_event) == true);
  container->Digest();
  screen.Draw();

  CHECK(details_ptr->open == true);
  CHECK(container->open == true);

  // Dispatch space key event
  Event space_event = Event::Keyboard::From(' ');
  CHECK(details_ptr->OnEvent(space_event) == true);
  container->Digest();
  screen.Draw();

  CHECK(details_ptr->open == false);
  CHECK(container->open == false);

  // 5. Toggle via Mouse Click on summary-line
  Event::Mouse mouse_click;
  mouse_click.button = Event::Mouse::Button::Left;
  mouse_click.motion = Event::Mouse::Motion::Pressed;
  mouse_click.x = summary_line_el->absolute_x() + 1;
  mouse_click.y = summary_line_el->absolute_y() + 1;
  Event click_event(mouse_click);

  screen.Dispatch(click_event);
  container->Digest();
  screen.Draw();

  CHECK(details_ptr->open == true);
  CHECK(container->open == true);
}

TEST_CASE("Details Component Default Summary", "[component][details]") {
  auto container = rtxui::Ref<DetailsDefaultTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* details_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("details");
  REQUIRE(details_el != nullptr);
  auto* details_ptr = ComponentAs<rtxui::details>(details_el);
  REQUIRE(details_ptr != nullptr);

  // With no <summary> projected, the summary line falls back to the label
  // "Details". It used to be an imperatively-added TextElement parked inside
  // the summary slot; it is now declared in the template next to the slot, so
  // assert what the user sees rather than where the node lives.
  auto* summary_line = rtxui::ComponentInternals::Root(*container)
                           ->QuerySelector(".summary-line");
  REQUIRE(summary_line != nullptr);
  std::string label;
  summary_line->Visit([&](rtxui::Element& e) {
    if (auto* text = dynamic_cast<rtxui::TextElement*>(&e)) {
      label += text->text();
    }
  });
  CHECK(label.find("Details") != std::string::npos);
}

TEST_CASE(
    "Details Component Exposes Part Attributes For External "
    "::part() Styling",
    "[component][details][part]") {
  auto container = rtxui::Ref<DetailsTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* details_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("details");
  REQUIRE(details_el != nullptr);

  auto* container_el = details_el->QuerySelector(".details-container");
  REQUIRE(container_el != nullptr);
  const std::string* container_part = container_el->GetAttribute("part");
  REQUIRE(container_part != nullptr);
  CHECK(*container_part == "details-container");

  auto* summary_line_el = details_el->QuerySelector(".summary-line");
  REQUIRE(summary_line_el != nullptr);
  const std::string* summary_line_part = summary_line_el->GetAttribute("part");
  REQUIRE(summary_line_part != nullptr);
  CHECK(*summary_line_part == "summary-line");

  auto* arrow_el = details_el->QuerySelector(".arrow");
  REQUIRE(arrow_el != nullptr);
  const std::string* arrow_part = arrow_el->GetAttribute("part");
  REQUIRE(arrow_part != nullptr);
  CHECK(*arrow_part == "arrow");

  auto* content_el = details_el->QuerySelector(".details-content");
  REQUIRE(content_el != nullptr);
  const std::string* content_part = content_el->GetAttribute("part");
  REQUIRE(content_part != nullptr);
  CHECK(*content_part == "details-content");
}
