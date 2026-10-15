// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/slider/slider.hpp"

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>

#include "catch2/catch_message.hpp"
#include "rtxui/component.hpp"
#include "rtxui/component/component_internal.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/event.hpp"
#include "rtxui/refcounted.hpp"
#include "rtxui/screen.hpp"
#include "rtxui/terminal/screen_internal.hpp"
#include "rtxui/terminal/terminal_device.hpp"
#include "rtxui/testing/component_as.hpp"

using namespace rtxui;

namespace {

TEST_CASE("Slider squashed layout regression", "[component][slider][layout]") {
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  device->TriggerResize(40, 5);

  class SliderTest : public rtxui::Component<SliderTest> {
   public:
    std::string_view view = R"(
      <div style="display: flex; width: 5;">
        <slider id="slider" width="20" value="50" />
      </div>
    )";
  };

  auto component = rtxui::Ref<SliderTest>::New();
  rtxui::Screen screen = rtxui::ScreenInternals::Make(component, device);
  screen.Draw();

  std::string output = device->GetOutput();

  // Count track characters '─'. We expect 19 (10 + 9).
  int track_count = 0;
  size_t pos = 0;
  while ((pos = output.find("─", pos)) != std::string::npos) {
    track_count++;
    pos += 3;  // '─' is 3 bytes in UTF-8
  }

  if (track_count <
      15) {  // Allow some slack but ensure it's not squashed to ~3
    FAIL("Slider squashed. Track count=" << track_count << "\nOutput:\n"
                                         << output);
  }

  CHECK(track_count >= 15);
  CHECK(output.find("●") != std::string::npos);
}

class SliderTestComponent : public rtxui::Component<SliderTestComponent> {
 public:
  int my_val = 50;
  bool onchange_called = false;

  SliderTestComponent() {
    Bind(my_val);
    Import("OnSliderChange", [this]() { onchange_called = true; });
  }
  std::string_view view = R"(
    <slider value="{my_val}" min="0" max="100" step="10" width="11" onchange="OnSliderChange" />
  )";
};

TEST_CASE("Slider Component Basic Interactions", "[component][slider]") {
  auto container = rtxui::Ref<SliderTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* slider_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("slider");
  REQUIRE(slider_el != nullptr);
  auto* slider_ptr = ComponentAs<rtxui::slider>(slider_el);
  REQUIRE(slider_ptr != nullptr);

  // Initial state (value = 50)
  CHECK(slider_ptr->value == 50);
  CHECK(container->my_val == 50);
  CHECK(container->onchange_called == false);

  // Focus and trigger arrow keys
  slider_el->set_focused(true);
  slider_ptr->OnEvent(Event::ArrowRight());
  container->Digest();

  // Value should increase by step (10)
  CHECK(slider_ptr->value == 60);
  CHECK(container->my_val == 60);
  CHECK(container->onchange_called == true);

  // Press ArrowLeft
  container->onchange_called = false;
  slider_ptr->OnEvent(Event::ArrowLeft());
  container->Digest();

  CHECK(slider_ptr->value == 50);
  CHECK(container->my_val == 50);
  CHECK(container->onchange_called == true);
}

class SliderStepZeroTestComponent
    : public rtxui::Component<SliderStepZeroTestComponent> {
 public:
  int my_val = 50;
  SliderStepZeroTestComponent() { Bind(my_val); }
  std::string_view view = R"(
    <slider value="{my_val}" min="0" max="100" step="0" width="11" />
  )";
};

TEST_CASE("Slider with step=0 doesn't crash on mouse drag",
          "[component][slider][regression]") {
  // Regression: step is bindable and freely settable via the step=""
  // attribute with no validation. Snapping the dragged value to the
  // nearest step did `(raw_val - min) % step` unconditionally, so step=0
  // crashed the whole app with SIGFPE (integer modulo by zero) on the
  // first mouse-drag interaction.
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  auto container = rtxui::Ref<SliderStepZeroTestComponent>::New();
  rtxui::Screen screen = rtxui::ScreenInternals::Make(container, device);
  screen.Draw();

  auto* slider_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("slider");
  REQUIRE(slider_el != nullptr);
  int abs_x = slider_el->absolute_x();
  int abs_y = slider_el->absolute_y();

  Event::Mouse mouse;
  mouse.button = Event::Mouse::Button::Left;
  mouse.motion = Event::Mouse::Motion::Pressed;
  mouse.x = abs_x + 2;
  mouse.y = abs_y + 1;
  screen.Dispatch(Event(mouse));

  CHECK(container->my_val == 0);

  // Keyboard arrows go through the same clamp (delta = ±step); with a
  // sane step=1 floor they should still move the value.
  Event::Keyboard right_arrow;
  right_arrow.motion = Event::Keyboard::Motion::Pressed;
  right_arrow.special = Event::Keyboard::Special::ArrowRight;
  screen.Dispatch(Event(right_arrow));
  CHECK(container->my_val == 1);
}

TEST_CASE("Slider Component Mouse Drag and Capture", "[component][slider]") {
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  auto container = rtxui::Ref<SliderTestComponent>::New();
  rtxui::Screen screen = rtxui::ScreenInternals::Make(container, device);
  screen.Draw();

  auto* slider_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("slider");
  REQUIRE(slider_el != nullptr);
  auto* slider_ptr = ComponentAs<rtxui::slider>(slider_el);
  REQUIRE(slider_ptr != nullptr);

  // width=11, min=0, max=100, step=10. Root() is an inline <span> whose
  // layout_width_ is 0; use the width attribute directly for coordinates.
  int abs_x = rtxui::ComponentInternals::Root(*slider_ptr)
                  ->absolute_x();  // 0 in mock terminal
  int abs_y = rtxui::ComponentInternals::Root(*slider_ptr)
                  ->absolute_y();                // 0 in mock terminal
  int track_w = std::max(2, slider_ptr->width);  // 11

  // Press at the leftmost column of the track (1-based mouse coords)
  {
    Event::Mouse mouse;
    mouse.button = Event::Mouse::Button::Left;
    mouse.motion = Event::Mouse::Motion::Pressed;
    mouse.x = abs_x + 2;  // 1-based: column 1 in 0-indexed (start of track)
    mouse.y = abs_y + 1;
    screen.Dispatch(Event(mouse));
  }

  // Value should be 0 (leftmost) and capture active
  CHECK(slider_ptr->value == 0);
  CHECK(rtxui::ComponentInternals::GetMouseCapturer() == slider_ptr);

  // Move to the centre of the track — y deliberately far off (capture ignores
  // it)
  {
    Event::Mouse mouse;
    mouse.button = Event::Mouse::Button::Left;
    mouse.motion = Event::Mouse::Motion::Moved;
    mouse.x = abs_x + (track_w / 2) + 2;  // middle column, 1-based
    mouse.y = abs_y + 50;
    screen.Dispatch(Event(mouse));
  }

  // Value should be 50 and capture still held
  CHECK(slider_ptr->value == 50);
  CHECK(rtxui::ComponentInternals::GetMouseCapturer() == slider_ptr);

  // Release at the rightmost column
  {
    Event::Mouse mouse;
    mouse.button = Event::Mouse::Button::Left;
    mouse.motion = Event::Mouse::Motion::Released;
    mouse.x = abs_x + track_w + 1;  // last column, 1-based
    mouse.y = abs_y + 1;
    screen.Dispatch(Event(mouse));
  }

  // Value should be 100 and capture released
  CHECK(slider_ptr->value == 100);
  CHECK(rtxui::ComponentInternals::GetMouseCapturer() == nullptr);
}

TEST_CASE(
    "Slider Component Exposes Track/Thumb Part Attributes For "
    "External ::part() Styling",
    "[component][slider][part]") {
  auto container = rtxui::Ref<SliderTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto* slider_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("slider");
  REQUIRE(slider_el != nullptr);

  auto* container_el = slider_el->QuerySelector(".slider-container");
  REQUIRE(container_el != nullptr);
  const std::string* container_part = container_el->GetAttribute("part");
  REQUIRE(container_part != nullptr);
  CHECK(*container_part == "slider-container");

  auto* track_left_el = slider_el->QuerySelector(".track-left");
  REQUIRE(track_left_el != nullptr);
  const std::string* track_left_part = track_left_el->GetAttribute("part");
  REQUIRE(track_left_part != nullptr);
  CHECK(*track_left_part == "track-left");

  auto* thumb_el = slider_el->QuerySelector(".thumb");
  REQUIRE(thumb_el != nullptr);
  const std::string* thumb_part = thumb_el->GetAttribute("part");
  REQUIRE(thumb_part != nullptr);
  CHECK(*thumb_part == "thumb");

  auto* track_right_el = slider_el->QuerySelector(".track-right");
  REQUIRE(track_right_el != nullptr);
  const std::string* track_right_part = track_right_el->GetAttribute("part");
  REQUIRE(track_right_part != nullptr);
  CHECK(*track_right_part == "track-right");
}

}  // namespace

TEST_CASE("Vertical slider layout regression", "[component][slider][layout]") {
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  device->TriggerResize(10, 10);

  class VerticalSliderTest : public rtxui::Component<VerticalSliderTest> {
   public:
    std::string_view view = R"(
      <slider direction="vertical" width="5" value="50" min="0" max="100" />
    )";
  };

  auto component = rtxui::Ref<VerticalSliderTest>::New();
  rtxui::Screen screen = rtxui::ScreenInternals::Make(component, device);
  screen.Draw();

  std::string output = device->GetOutput();

  int rows_with_slider = 0;
  std::stringstream ss(output);
  std::string line;
  while (std::getline(ss, line)) {
    if (line.find("│") != std::string::npos ||
        line.find("●") != std::string::npos) {
      rows_with_slider++;
    }
  }

  if (rows_with_slider < 5) {
    FAIL("Vertical slider should span 5 rows. Output:\n" << output);
  }
  CHECK(rows_with_slider >= 5);
}

namespace {

class SliderDemoSquashTestComponent
    : public rtxui::Component<SliderDemoSquashTestComponent> {
 public:
  int slider_val = 50;

  SliderDemoSquashTestComponent() { Bind(slider_val); }

  std::string_view view = R"html(
    <div class="container">
      <div class="root">
        <div class="workspace">
          <div class="sidebar">Sidebar</div>
          <div class="main-scroll">
            <div class="card">
              <div class="row">
                <span>Slider:</span>
                <slider id="slider" value="{slider_val}" min="0" max="100" step="5" width="18" />
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
    <style>
      self {
        display: block;
        width: 100%;
        height: 100%;
      }
      .container {
        display: block;
        max-width: 120;
        width: 100%;
        margin-left: auto;
        margin-right: auto;
      }
      .root {
        display: flex;
        flex-direction: column;
        width: 100%;
      }
      .workspace {
        display: flex;
        flex-direction: row;
        width: 100%;
      }
      .sidebar {
        display: block;
        width: 16;
      }
      .main-scroll {
        flex-grow: 1;
        width: 0;
        display: block;
      }
      .card {
        display: block;
      }
      .row {
        display: flex;
        flex-direction: row;
      }
    </style>
  )html";
};

}  // namespace

TEST_CASE("Slider interaction layout squashing regression",
          "[component][slider][layout][regression]") {
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  device->TriggerResize(150, 20);

  auto container = rtxui::Ref<SliderDemoSquashTestComponent>::New();
  rtxui::Screen screen = rtxui::ScreenInternals::Make(container, device);
  screen.Draw();

  auto* slider_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("#slider");
  REQUIRE(slider_el != nullptr);
  auto* scroll_el = rtxui::ComponentInternals::Root(*container)
                        ->QuerySelector(".main-scroll");
  REQUIRE(scroll_el != nullptr);
  auto* container_el =
      rtxui::ComponentInternals::Root(*container)->QuerySelector(".container");
  REQUIRE(container_el != nullptr);

  int initial_container_width = container_el->layout_width();
  int initial_scroll_width = scroll_el->layout_width();

  // Click/Drag the slider
  Event::Mouse mouse;
  mouse.button = Event::Mouse::Button::Left;
  mouse.motion = Event::Mouse::Motion::Pressed;
  mouse.x = slider_el->absolute_x() + 5;
  mouse.y = slider_el->absolute_y() + 1;
  screen.Dispatch(Event(mouse));

  // Let's release the mouse too
  mouse.motion = Event::Mouse::Motion::Released;
  screen.Dispatch(Event(mouse));

  // Draw again to render final frame
  screen.Draw();

  int final_container_width = container_el->layout_width();
  int final_scroll_width = scroll_el->layout_width();

  CHECK(final_container_width == initial_container_width);
  CHECK(final_scroll_width == initial_scroll_width);
}
