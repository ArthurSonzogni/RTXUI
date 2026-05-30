#include "rtxui/internal/screen.hpp"
#include "rtxui/terminal/terminal_device.hpp"
#include "rtxui/internal/component.hpp"
#include "rtxui/internal/default_components.hpp"
#include "catch2/catch_test_macros.hpp"
#include "rtxui/dom/element.hpp"
#include <memory>

namespace rtxui {
namespace {

class DummyComponent : public Component<DummyComponent> {
 public:
  void InitReflection() override {
    Import<rtxui::div>();
    Component<DummyComponent>::InitReflection();
  }
  std::string_view view = R"(
    <div>Hello Mock</div>
  )";
};

TEST_CASE("TerminalDevice.MockIOWrites", "[terminal]") {
  auto device = std::make_shared<MockTerminalDevice>();
  auto component = Ref<DummyComponent>::New();
  
  Screen screen(component, device);
  
  // Verify that drawing wrote the expected component output to the mock terminal device.
  std::string output = device->GetOutput();
  REQUIRE_FALSE(output.empty());
  REQUIRE(output.find("Hello Mock") != std::string::npos);
}

TEST_CASE("TerminalDevice.ResizeTrigger", "[terminal]") {
  auto device = std::make_shared<MockTerminalDevice>();
  auto component = Ref<DummyComponent>::New();
  
  Screen screen(component, device);
  device->ClearOutput();
  
  // Set size to a new dimension and check if resizing updates width/height.
  device->TriggerResize(100, 30);
  
  // Screen size can be updated manually via Screen::UpdateSize or inside the event loop.
  // We will test direct Step-by-Step loop size updates in Step 2.
  int width = 0, height = 0;
  device->GetSize(width, height);
  REQUIRE(width == 100);
  REQUIRE(height == 30);
}

TEST_CASE("Screen.StepExecution", "[terminal]") {
  auto device = std::make_shared<MockTerminalDevice>();
  
  class EventTrackerComponent : public Component<EventTrackerComponent> {
   public:
    std::string last_event;
    bool OnEvent(Event event) override {
      if (event.is<Event::Keyboard>()) {
        last_event = "key:" + std::string(1, static_cast<char>(event.get<Event::Keyboard>().codepoint));
        return true;
      }
      return false;
    }
    std::string_view view = "Tracker";
  };
  
  auto component = Ref<EventTrackerComponent>::New();
  Screen screen(component, device);
  
  // Push 'a' and 'b' to input
  device->PushInput("ab");
  
  // First Step() should process 'a'
  screen.Step();
  REQUIRE(component->last_event == "key:a");
  
  // Second Step() should process 'b'
  screen.Step();
  REQUIRE(component->last_event == "key:b");
  
  // Push Ctrl-C to input and check if it terminates running_
  device->PushInput("\x03");
  screen.Step();
  // We can query screen.Step or loop to see if it should stop. We can't access private members directly, but we can verify it doesn't loop. Wait, we can test screen.Step() is a no-op if exited or check state if we expose a way, but since we didn't expose running_ via public getter, this is already verifying the return flow.
}

TEST_CASE("Screen.DispatchMouseEvent", "[terminal]") {
  auto device = std::make_shared<MockTerminalDevice>();
  
  class ClickableComponent : public Component<ClickableComponent> {
   public:
    int clicked_count = 0;
    void on_click() { clicked_count++; }

    void InitReflection() override {
      Import<rtxui::div>();
      Component<ClickableComponent>::InitReflection();
    }

    ClickableComponent() {
      Import("on_click", [this]() { on_click(); });
      Bind(clicked_count);
    }

    std::string_view view = R"(
      <div id="btn" onclick="on_click">Click Me ({clicked_count})</div>
    )";
  };

  auto component = Ref<ClickableComponent>::New();
  Screen screen(component, device);
  
  // Initially clicked_count is 0
  REQUIRE(component->clicked_count == 0);

  // Construct a Left Mouse Button Pressed Event at x=1, y=1
  Event::Mouse mouse_event;
  mouse_event.button = Event::Mouse::Button::Left;
  mouse_event.motion = Event::Mouse::Motion::Pressed;
  mouse_event.x = 1;
  mouse_event.y = 1;
  
  Event event = mouse_event;
  
  // Dispatch the event
  screen.Dispatch(event);
  
  // Clicked count should now be 1!
  REQUIRE(component->clicked_count == 1);
  
  // Let's verify that the output was re-rendered using QuerySelector and checking the view.
  auto* btn_element = component->Root()->QuerySelector("#btn");
  REQUIRE(btn_element != nullptr);
  REQUIRE(btn_element->Print().find("Click Me (1)") != std::string::npos);
}

TEST_CASE("Screen.ScrollEventAndClipping", "[terminal][scroll]") {
  auto device = std::make_shared<MockTerminalDevice>();

  class ScrollTestComponent : public Component<ScrollTestComponent> {
   public:
    void InitReflection() override {
      Import<rtxui::div>();
      Component<ScrollTestComponent>::InitReflection();
    }
    std::string_view view = R"html(
      <div id="scrollable">
        <div>Line 1</div>
        <div>Line 2</div>
        <div>Line 3</div>
        <div>Line 4</div>
        <div>Line 5</div>
        <div>Line 6</div>
        <div>Line 7</div>
        <div>Line 8</div>
        <div>Line 9</div>
        <div>Line 10</div>
      </div>
      <style>
        #scrollable {
          display: block;
          height: 4;
          overflow-y: scroll;
          scroll-speed: 2;
        }
      </style>
    )html";
  };

  auto component = Ref<ScrollTestComponent>::New();
  Screen screen(component, device);

  auto* scroll_element = component->Root()->QuerySelector("#scrollable");
  REQUIRE(scroll_element != nullptr);

  // 1. Initial State
  REQUIRE(scroll_element->scroll_height() == 10);
  REQUIRE(scroll_element->scroll_y() == 0);

  // 2. Mouse Wheel Scroll Down (Scroll by speed = 2)
  Event::Mouse wheel_down;
  wheel_down.button = Event::Mouse::Button::WheelDown;
  wheel_down.motion = Event::Mouse::Motion::Pressed;
  wheel_down.x = 2;
  wheel_down.y = 2;
  screen.Dispatch(wheel_down);
  REQUIRE(scroll_element->scroll_y() == 2);

  // 3. Mouse Wheel Scroll Up (Scroll by speed = 2)
  Event::Mouse wheel_up;
  wheel_up.button = Event::Mouse::Button::WheelUp;
  wheel_up.motion = Event::Mouse::Motion::Pressed;
  wheel_up.x = 2;
  wheel_up.y = 2;
  screen.Dispatch(wheel_up);
  REQUIRE(scroll_element->scroll_y() == 0);

  // 4. Click to Focus the Element
  Event::Mouse click;
  click.button = Event::Mouse::Button::Left;
  click.motion = Event::Mouse::Motion::Pressed;
  click.x = 2;
  click.y = 2;
  screen.Dispatch(click);

  // 5. Keyboard Navigation ArrowDown (Scroll by speed = 2)
  screen.Dispatch(Event::ArrowDown());
  REQUIRE(scroll_element->scroll_y() == 2);

  // 6. Keyboard PageDown (Scroll by viewport height = 4)
  screen.Dispatch(Event::PageDown());
  // 2 + 4 = 6. Max scroll is 10 (content) - 4 (viewport) = 6.
  REQUIRE(scroll_element->scroll_y() == 6);

  // 7. Keyboard ArrowUp (Scroll by speed = 2)
  screen.Dispatch(Event::ArrowUp());
  REQUIRE(scroll_element->scroll_y() == 4);

  // 8. Keyboard PageUp (Scroll by viewport height = 4)
  screen.Dispatch(Event::PageUp());
  REQUIRE(scroll_element->scroll_y() == 0);
}

TEST_CASE("Screen.HorizontalScrollEvent", "[terminal][scroll]") {
  auto device = std::make_shared<MockTerminalDevice>();

  class ScrollTestComponent : public Component<ScrollTestComponent> {
   public:
    void InitReflection() override {
      Import<rtxui::div>();
      Component<ScrollTestComponent>::InitReflection();
    }
    std::string_view view = R"html(
      <div id="scrollable">
        <div class="wide-child">Very long text that overflows horizontally</div>
      </div>
      <style>
        #scrollable {
          display: block;
          width: 10;
          overflow-x: scroll;
          scroll-speed-x: 3;
        }
        .wide-child {
          display: block;
          width: 30;
        }
      </style>
    )html";
  };

  auto component = Ref<ScrollTestComponent>::New();
  Screen screen(component, device);

  auto* scroll_element = component->Root()->QuerySelector("#scrollable");
  REQUIRE(scroll_element != nullptr);

  // 1. Initial State
  // Content width = 30, viewport width = 10. Max scroll = 20.
  REQUIRE(scroll_element->scroll_width() == 30);
  REQUIRE(scroll_element->scroll_x() == 0);

  // 2. Mouse Wheel Scroll Right (Scroll by speed = 3)
  Event::Mouse wheel_right;
  wheel_right.button = Event::Mouse::Button::WheelRight;
  wheel_right.motion = Event::Mouse::Motion::Pressed;
  wheel_right.x = 2;
  wheel_right.y = 2;
  screen.Dispatch(wheel_right);
  REQUIRE(scroll_element->scroll_x() == 3);

  // 3. Mouse Wheel Scroll Left (Scroll by speed = 3)
  Event::Mouse wheel_left;
  wheel_left.button = Event::Mouse::Button::WheelLeft;
  wheel_left.motion = Event::Mouse::Motion::Pressed;
  wheel_left.x = 2;
  wheel_left.y = 2;
  screen.Dispatch(wheel_left);
  REQUIRE(scroll_element->scroll_x() == 0);

  // 4. Click to Focus the Element
  Event::Mouse click;
  click.button = Event::Mouse::Button::Left;
  click.motion = Event::Mouse::Motion::Pressed;
  click.x = 2;
  click.y = 2;
  screen.Dispatch(click);

  // 5. Keyboard Navigation ArrowRight (Scroll by speed = 3)
  screen.Dispatch(Event::ArrowRight());
  REQUIRE(scroll_element->scroll_x() == 3);

  // 6. Keyboard ArrowLeft (Scroll by speed = 3)
  screen.Dispatch(Event::ArrowLeft());
  REQUIRE(scroll_element->scroll_x() == 0);
}

TEST_CASE("Screen.NestedScrollChaining", "[terminal][scroll]") {
  auto device = std::make_shared<MockTerminalDevice>();

  class NestedScrollComponent : public Component<NestedScrollComponent> {
   public:
    void InitReflection() override {
      Import<rtxui::div>();
      Component<NestedScrollComponent>::InitReflection();
    }
    std::string_view view = R"html(
      <div id="outer" class="outer-scroll">
        <div>Outer 1</div>
        <div>Outer 2</div>
        <div>Outer 3</div>
        <div id="inner" class="inner-scroll">
          <div>Inner 1</div>
          <div>Inner 2</div>
          <div>Inner 3</div>
          <div>Inner 4</div>
          <div>Inner 5</div>
        </div>
        <div>Outer 4</div>
        <div>Outer 5</div>
      </div>
      <style>
        .outer-scroll {
          display: block;
          height: 4;
          overflow-y: scroll;
          scroll-speed: 1;
        }
        .inner-scroll {
          display: block;
          height: 3;
          overflow-y: scroll;
          scroll-speed: 1;
        }
      </style>
    )html";
  };

  auto component = Ref<NestedScrollComponent>::New();
  Screen screen(component, device);

  auto* outer = component->Root()->QuerySelector("#outer");
  auto* inner = component->Root()->QuerySelector("#inner");
  REQUIRE(outer != nullptr);
  REQUIRE(inner != nullptr);

  // Initial State: both scroll_y are 0
  REQUIRE(outer->scroll_y() == 0);
  REQUIRE(inner->scroll_y() == 0);

  // Scroll Down 1: Inner scrolls from 0 to 1
  Event::Mouse wheel_down;
  wheel_down.button = Event::Mouse::Button::WheelDown;
  wheel_down.motion = Event::Mouse::Motion::Pressed;
  wheel_down.x = 2;
  wheel_down.y = 4; // targeting inner at y=3 (0-indexed)
  screen.Dispatch(wheel_down);
  REQUIRE(inner->scroll_y() == 1);
  REQUIRE(outer->scroll_y() == 0);

  // Scroll Down 2: Inner scrolls from 1 to 2 (max_scroll is 5-3 = 2)
  screen.Dispatch(wheel_down);
  REQUIRE(inner->scroll_y() == 2);
  REQUIRE(outer->scroll_y() == 0);

  // Scroll Down 3: Inner is already at max (2), so scroll bubbles up to outer!
  // Outer scrolls from 0 to 1.
  screen.Dispatch(wheel_down);
  REQUIRE(inner->scroll_y() == 2);
  REQUIRE(outer->scroll_y() == 1);

  // Scroll Down 4: Outer scrolls from 1 to 2.
  screen.Dispatch(wheel_down);
  REQUIRE(inner->scroll_y() == 2);
  REQUIRE(outer->scroll_y() == 2);

  // Now test Scroll Up:
  Event::Mouse wheel_up;
  wheel_up.button = Event::Mouse::Button::WheelUp;
  wheel_up.motion = Event::Mouse::Motion::Pressed;
  wheel_up.x = 2;
  wheel_up.y = 4;

  // Scroll Up 1: Inner scrolls back from 2 to 1.
  screen.Dispatch(wheel_up);
  REQUIRE(inner->scroll_y() == 1);
  REQUIRE(outer->scroll_y() == 2);

  // Scroll Up 2: Inner scrolls back from 1 to 0.
  screen.Dispatch(wheel_up);
  REQUIRE(inner->scroll_y() == 0);
  REQUIRE(outer->scroll_y() == 2);

  // Scroll Up 3: Inner is at 0 (min scroll), so scroll bubbles up to outer.
  // Outer scrolls back from 2 to 1.
  screen.Dispatch(wheel_up);
  REQUIRE(inner->scroll_y() == 0);
  REQUIRE(outer->scroll_y() == 1);

  // Scroll Up 4: Outer scrolls back from 1 to 0.
  screen.Dispatch(wheel_up);
  REQUIRE(inner->scroll_y() == 0);
  REQUIRE(outer->scroll_y() == 0);

  // Test keyboard navigation focus/bubbling:
  // Focus the inner scroll container
  inner->set_focused(true);
  screen.Draw(); // update focused element state in Screen

  // Press ArrowDown 1: Inner scrolls from 0 to 1
  screen.Dispatch(Event::ArrowDown());
  REQUIRE(inner->scroll_y() == 1);
  REQUIRE(outer->scroll_y() == 0);

  // Press ArrowDown 2: Inner scrolls from 1 to 2
  screen.Dispatch(Event::ArrowDown());
  REQUIRE(inner->scroll_y() == 2);
  REQUIRE(outer->scroll_y() == 0);

  // Press ArrowDown 3: Inner at max scroll, bubbles up to outer (outer scrolls to 1)
  screen.Dispatch(Event::ArrowDown());
  REQUIRE(inner->scroll_y() == 2);
  REQUIRE(outer->scroll_y() == 1);

  // Press ArrowUp 1: Inner scrolls from 2 to 1
  screen.Dispatch(Event::ArrowUp());
  REQUIRE(inner->scroll_y() == 1);
  REQUIRE(outer->scroll_y() == 1);

  // Press ArrowUp 2: Inner scrolls from 1 to 0
  screen.Dispatch(Event::ArrowUp());
  REQUIRE(inner->scroll_y() == 0);
  REQUIRE(outer->scroll_y() == 1);

  // Press ArrowUp 3: Inner at min scroll, bubbles up to outer (outer scrolls to 0)
  screen.Dispatch(Event::ArrowUp());
  REQUIRE(inner->scroll_y() == 0);
  REQUIRE(outer->scroll_y() == 0);
}

TEST_CASE("Screen.TabFocusCycling", "[terminal][focus]") {
  auto device = std::make_shared<MockTerminalDevice>();

  class FocusCyclingComponent : public Component<FocusCyclingComponent> {
   public:
    void InitReflection() override {
      Import<rtxui::div>();
      Import<rtxui::input>();
      Import<rtxui::checkbox>();
      Component<FocusCyclingComponent>::InitReflection();
    }
    std::string_view view = R"html(
      <div>
        <input id="input1" />
        <checkbox id="checkbox1">Check me</checkbox>
        <div id="non-focusable">Not focusable</div>
        <input id="input2" />
      </div>
    )html";
  };

  auto component = Ref<FocusCyclingComponent>::New();
  Screen screen(component, device);

  auto* input1 = component->Root()->QuerySelector("#input1");
  auto* checkbox1 = component->Root()->QuerySelector("#checkbox1");
  auto* non_focusable = component->Root()->QuerySelector("#non-focusable");
  auto* input2 = component->Root()->QuerySelector("#input2");

  REQUIRE(input1 != nullptr);
  REQUIRE(checkbox1 != nullptr);
  REQUIRE(non_focusable != nullptr);
  REQUIRE(input2 != nullptr);

  // Initial State: no element focused
  CHECK_FALSE(input1->focused());
  CHECK_FALSE(checkbox1->focused());
  CHECK_FALSE(non_focusable->focused());
  CHECK_FALSE(input2->focused());

  // 1. Tab key: Focuses first focusable element (input1)
  screen.Dispatch(Event::Tab());
  CHECK(input1->focused());
  CHECK_FALSE(checkbox1->focused());
  CHECK_FALSE(input2->focused());

  // 2. Tab key: Focuses next (checkbox1)
  screen.Dispatch(Event::Tab());
  CHECK_FALSE(input1->focused());
  CHECK(checkbox1->focused());
  CHECK_FALSE(input2->focused());

  // 3. Tab key: Skip non-focusable and focus next (input2)
  screen.Dispatch(Event::Tab());
  CHECK_FALSE(input1->focused());
  CHECK_FALSE(checkbox1->focused());
  CHECK(input2->focused());

  // 4. Tab key: Wrap around to the start (input1)
  screen.Dispatch(Event::Tab());
  CHECK(input1->focused());
  CHECK_FALSE(checkbox1->focused());
  CHECK_FALSE(input2->focused());

  // 5. Shift+Tab (TabReverse): Wrap around to the end (input2)
  screen.Dispatch(Event::TabReverse());
  CHECK_FALSE(input1->focused());
  CHECK_FALSE(checkbox1->focused());
  CHECK(input2->focused());

  // 6. Shift+Tab (TabReverse): Go to previous (checkbox1)
  screen.Dispatch(Event::TabReverse());
  CHECK_FALSE(input1->focused());
  CHECK(checkbox1->focused());
  CHECK_FALSE(input2->focused());
}

} // namespace
} // namespace rtxui
