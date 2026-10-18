// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/tabs/tabs.hpp"

#include <catch2/catch_test_macros.hpp>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "rtxui/component.hpp"
#include "rtxui/component/component_internal.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/event.hpp"
#include "rtxui/layout/style.hpp"
#include "rtxui/paint/texture.hpp"
#include "rtxui/refcounted.hpp"
#include "rtxui/screen.hpp"
#include "rtxui/terminal/event_internal.hpp"
#include "rtxui/testing/component_as.hpp"
#include "rtxui/testing/render.hpp"

using namespace rtxui;

namespace {

// --- Tabs ---
class TabsTestComponent : public rtxui::Component<TabsTestComponent> {
 public:
  std::string active_tab = "tab1";

  TabsTestComponent() { Bind(active_tab); }
  std::string_view view = R"(
    <tabs value="{active_tab}">
      <tab-pane label="Tab One" name="tab1">Content One</tab-pane>
      <tab-pane label="Tab Two" name="tab2">Content Two</tab-pane>
    </tabs>
  )";
};

// The header buttons the <tabs> template renders, in pane order. They used to
// be Elements the component built by hand into a named slot; they are a <for>
// over a bound collection now, so reach them by class rather than by slot.
std::vector<rtxui::Element*> TabHeaderButtons(rtxui::Element* root) {
  std::vector<rtxui::Element*> buttons;
  if (root) {
    root->Visit([&](rtxui::Element& element) {
      for (const std::string& name : element.classes()) {
        if (name == "tab-header-btn") {
          buttons.push_back(&element);
        }
      }
    });
  }
  return buttons;
}

}  // namespace

TEST_CASE("Tabs and TabPane Components", "[component][tabs]") {
  auto container = rtxui::Ref<TabsTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* tabs_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("tabs");
  REQUIRE(tabs_el != nullptr);
  auto* tabs_ptr = ComponentAs<rtxui::tabs>(tabs_el);
  REQUIRE(tabs_ptr != nullptr);

  // Initial tab: tab1
  CHECK(tabs_ptr->value == "tab1");

  // Verify headers slot
  auto headers = TabHeaderButtons(rtxui::ComponentInternals::Root(*tabs_ptr));
  REQUIRE(headers.size() == 2);

  auto* tab1_header = headers[0];
  auto* tab2_header = headers[1];
  CHECK(tab1_header->classes()[1] == "active-tab");

  // Find pane elements initially
  Element* pane1_el = nullptr;
  Element* pane2_el = nullptr;
  auto find_panes = [&]() {
    pane1_el = nullptr;
    pane2_el = nullptr;
    rtxui::ComponentInternals::Root(*container)->Visit([&](Element& el) {
      if (el.tag() == "tab-pane" || el.tag() == "tab_pane") {
        if (el.Attributes().count("name")) {
          auto name = el.Attributes().at("name");
          if (name == "tab1") {
            pane1_el = &el;
          }
          if (name == "tab2") {
            pane2_el = &el;
          }
        }
      }
    });
  };
  find_panes();
  REQUIRE(pane1_el != nullptr);
  REQUIRE(pane2_el != nullptr);

  // Verify initial classes and display styles (tab1 active, tab2 inactive)
  CHECK(pane1_el->classes()[0] == "active");
  CHECK(pane1_el->style.display_none == false);
  CHECK(pane2_el->classes()[0] == "inactive");
  CHECK(pane2_el->style.display_none == true);

  // Select tab2 via index
  tabs_ptr->SelectTab("1");
  container->Digest();
  screen.Draw();

  CHECK(tabs_ptr->value == "tab2");
  CHECK(container->active_tab == "tab2");

  // Re-find pane elements after re-render/Digest
  find_panes();
  REQUIRE(pane1_el != nullptr);
  REQUIRE(pane2_el != nullptr);

  // Verify switched classes and display styles (tab1 inactive, tab2 active)
  CHECK(pane1_el->classes()[0] == "inactive");
  CHECK(pane1_el->style.display_none == true);
  CHECK(pane2_el->classes()[0] == "active");
  CHECK(pane2_el->style.display_none == false);
}

// Regression test: `.tabs-headers { border-bottom: solid; }` never actually
// rendered a divider - `border-bottom` only parses a numeric width, so
// "solid" silently failed to parse and left both the width and the border
// style unset. Now fixed to `border-bottom: 1; border-style: solid;`.
TEST_CASE("Tabs header divider renders as a line under the headers",
          "[component][tabs][border]") {
  auto container = rtxui::Ref<TabsTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto texture = PaintComponent(*container, 40, 10);

  auto* headers = rtxui::ComponentInternals::Root(*container)
                      ->QuerySelector(".tabs-headers");
  REQUIRE(headers != nullptr);
  // The bottom border grows the headers box by exactly 1 row: the header
  // button labels occupy row 0 (tabs is the top-level element here), so the
  // divider line must be on row 1.
  CHECK(headers->layout_height() == 2);
  bool found_divider_char = false;
  for (int x = 0; x < headers->layout_width(); ++x) {
    if (texture[x, 1].character == "─") {
      found_divider_char = true;
      break;
    }
  }
  CHECK(found_divider_char);
}

TEST_CASE("Tabs Component Interactions", "[component][tabs][interaction]") {
  auto container = rtxui::Ref<TabsTestComponent>::New();
  rtxui::Screen screen(container);
  screen.SetInline(false);  // Clicks at the cells it draws on.
  screen.Draw();

  auto* tabs_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("tabs");
  REQUIRE(tabs_el != nullptr);
  auto* tabs_ptr = ComponentAs<rtxui::tabs>(tabs_el);
  REQUIRE(tabs_ptr != nullptr);

  // Initial tab: tab1
  CHECK(tabs_ptr->value == "tab1");

  auto headers = TabHeaderButtons(rtxui::ComponentInternals::Root(*tabs_ptr));
  REQUIRE(headers.size() == 2);

  auto* tab2_header = headers[1];
  REQUIRE(tab2_header != nullptr);

  // Click on the second tab header (tab2_header)
  MouseData mouse_click;
  mouse_click.button = Event::Mouse::Button::Left;
  mouse_click.motion = Event::Mouse::Motion::Pressed;
  mouse_click.x = tab2_header->absolute_x() + 1;
  mouse_click.y = tab2_header->absolute_y() + 1;
  Event click_event(mouse_click);

  screen.Dispatch(click_event);
  container->Digest();
  screen.Draw();

  // Verify tab switched to tab2
  CHECK(tabs_ptr->value == "tab2");
  CHECK(container->active_tab == "tab2");

  // Verify keyboard interaction: focus the first tab header (tab1_header)
  auto* tab1_header = headers[0];
  REQUIRE(tab1_header != nullptr);

  rtxui::ComponentInternals::Root(*container)->Visit([](Element& el) {
    el.set_focused(false);
  });
  tab1_header->set_focused(true);

  // Press Return key
  Event return_event =
      KeyboardData{.special = Event::Keyboard::Special::Return};
  screen.Dispatch(return_event);
  container->Digest();
  screen.Draw();

  // Verify tab switched back to tab1
  CHECK(tabs_ptr->value == "tab1");
}

TEST_CASE("Tabs Component keyboard focus survives an intervening Digest",
          "[component][tabs][regression]") {
  // Regression: tabs::Digest() used to unconditionally destroy and
  // recreate every header button Element, even when only switching the
  // active value (not the pane list). If anything else triggered a
  // Digest() between focusing a header via keyboard and pressing
  // Enter/Space, the focused button was replaced by a fresh, unfocused
  // one, and the keypress silently did nothing.
  auto container = rtxui::Ref<TabsTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* tabs_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("tabs");
  auto* tabs_ptr = ComponentAs<rtxui::tabs>(tabs_el);
  auto headers = TabHeaderButtons(rtxui::ComponentInternals::Root(*tabs_ptr));
  REQUIRE(headers.size() == 2);
  auto* tab2_header = headers[1];

  rtxui::ComponentInternals::Root(*container)->Visit([](Element& el) {
    el.set_focused(false);
  });
  tab2_header->set_focused(true);

  // Something else triggers a Digest() before the keypress arrives (e.g. an
  // unrelated prop change elsewhere in the app).
  container->Digest();

  // Focus is what has to survive, not the Element identity. Reconciliation is
  // free to hand back a different object for the same button -- so re-read it
  // rather than holding the old pointer, which is what the component used to
  // do and how it lost focus in the first place.
  headers = TabHeaderButtons(rtxui::ComponentInternals::Root(*tabs_ptr));
  REQUIRE(headers.size() == 2);
  CHECK(headers[1]->focused());
  CHECK_FALSE(headers[0]->focused());

  Event return_event =
      KeyboardData{.special = Event::Keyboard::Special::Return};
  screen.Dispatch(return_event);
  container->Digest();
  screen.Draw();

  CHECK(tabs_ptr->value == "tab2");
}

TEST_CASE(
    "Tabs Component Exposes Part Attributes For External "
    "::part() Styling",
    "[component][tabs][part]") {
  auto container = rtxui::Ref<TabsTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* tabs_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("tabs");
  REQUIRE(tabs_el != nullptr);
  auto* tabs_ptr = ComponentAs<rtxui::tabs>(tabs_el);
  REQUIRE(tabs_ptr != nullptr);

  auto* container_el = tabs_el->QuerySelector(".tabs-container");
  REQUIRE(container_el != nullptr);
  const std::string* container_part = container_el->GetAttribute("part");
  REQUIRE(container_part != nullptr);
  CHECK(*container_part == "tabs-container");

  auto* headers_el = tabs_el->QuerySelector(".tabs-headers");
  REQUIRE(headers_el != nullptr);
  const std::string* headers_part = headers_el->GetAttribute("part");
  REQUIRE(headers_part != nullptr);
  CHECK(*headers_part == "tabs-headers");

  auto* content_el = tabs_el->QuerySelector(".tabs-content");
  REQUIRE(content_el != nullptr);
  const std::string* content_part = content_el->GetAttribute("part");
  REQUIRE(content_part != nullptr);
  CHECK(*content_part == "tabs-content");

  auto headers = TabHeaderButtons(rtxui::ComponentInternals::Root(*tabs_ptr));
  REQUIRE(headers.size() == 2);
  auto* tab1_header = headers[0];
  const std::string* tab1_header_part = tab1_header->GetAttribute("part");
  REQUIRE(tab1_header_part != nullptr);
  CHECK(*tab1_header_part == "tab-header-btn active-tab");
  auto* tab2_header = headers[1];
  const std::string* tab2_header_part = tab2_header->GetAttribute("part");
  REQUIRE(tab2_header_part != nullptr);
  CHECK(*tab2_header_part == "tab-header-btn");
}

namespace {

class TabsCrashPreventionTestComponent
    : public rtxui::Component<TabsCrashPreventionTestComponent> {
 public:
  std::string active_tab = "tab1";

  TabsCrashPreventionTestComponent() { Bind(active_tab); }
  std::string_view view = R"(
    <tabs value="{active_tab}">
      <tab-pane label="Tab One" name="tab1">Content One</tab-pane>
    </tabs>
  )";
};

}  // namespace

TEST_CASE("Tabs SelectTab Crash Prevention", "[component][tabs][bug]") {
  auto container = rtxui::Ref<TabsCrashPreventionTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* tabs_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("tabs");
  REQUIRE(tabs_el != nullptr);
  auto* tabs_ptr = ComponentAs<rtxui::tabs>(tabs_el);
  REQUIRE(tabs_ptr != nullptr);

  CHECK_NOTHROW(tabs_ptr->SelectTab(""));
  CHECK_NOTHROW(tabs_ptr->SelectTab("invalid"));
  CHECK_NOTHROW(tabs_ptr->SelectTab("999"));
}
