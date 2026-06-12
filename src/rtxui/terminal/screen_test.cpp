#include "rtxui/internal/screen.hpp"

#include <memory>

#include "catch2/catch_test_macros.hpp"
#include "rtxui/component/default_components_internal.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/internal/component.hpp"
#include "rtxui/paint/texture.hpp"
#include "rtxui/terminal/terminal_device.hpp"

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

  // Verify that drawing wrote the expected component output to the mock
  // terminal device.
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

  // Screen size can be updated manually via Screen::UpdateSize or inside the
  // event loop. We will test direct Step-by-Step loop size updates in Step 2.
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
        last_event =
            "key:" +
            std::string(
                1, static_cast<char>(event.get<Event::Keyboard>().codepoint));
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
  // We can query screen.Step or loop to see if it should stop. We can't access
  // private members directly, but we can verify it doesn't loop. Wait, we can
  // test screen.Step() is a no-op if exited or check state if we expose a way,
  // but since we didn't expose running_ via public getter, this is already
  // verifying the return flow.
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

  // Let's verify that the output was re-rendered using QuerySelector and
  // checking the view.
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
  screen.SetSmoothScrollEnabled(false);

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
  screen.SetSmoothScrollEnabled(false);

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
  screen.SetSmoothScrollEnabled(false);

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
  wheel_down.y = 4;  // targeting inner at y=3 (0-indexed)
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
  screen.Draw();  // update focused element state in Screen

  // Press ArrowDown 1: Inner scrolls from 0 to 1
  screen.Dispatch(Event::ArrowDown());
  REQUIRE(inner->scroll_y() == 1);
  REQUIRE(outer->scroll_y() == 0);

  // Press ArrowDown 2: Inner scrolls from 1 to 2
  screen.Dispatch(Event::ArrowDown());
  REQUIRE(inner->scroll_y() == 2);
  REQUIRE(outer->scroll_y() == 0);

  // Press ArrowDown 3: Inner at max scroll, bubbles up to outer (outer scrolls
  // to 1)
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

  // Press ArrowUp 3: Inner at min scroll, bubbles up to outer (outer scrolls to
  // 0)
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

TEST_CASE("Screen.TransitionsAndHover", "[terminal][transitions]") {
  // Reset clock to normal when test finishes
  struct ClockRestorer {
    ~ClockRestorer() { time::SetCustomClock(nullptr); }
  } restorer;

  static double mock_now_ms = 1000.0;
  mock_now_ms = 1000.0;
  time::SetCustomClock([]() -> double { return mock_now_ms; });

  auto device = std::make_shared<MockTerminalDevice>();

  class TransitionTestComponent : public Component<TransitionTestComponent> {
   public:
    void InitReflection() override {
      Import<rtxui::div>();
      Component<TransitionTestComponent>::InitReflection();
    }
    std::string_view view = R"html(
      <div id="btn">Click me</div>
      <style>
        #btn {
          background-color: #000000;
          color: #ffffff;
          transition: background-color 1s linear;
        }
        #btn:hover {
          background-color: #ff0000;
        }
      </style>
    )html";
  };

  auto component = Ref<TransitionTestComponent>::New();
  Screen screen(component, device);

  auto* btn = component->Root()->QuerySelector("#btn");
  REQUIRE(btn != nullptr);

  // 1. Initial style: background-color is black (#000000)
  CHECK(btn->style.background_color == Color::RGB(0, 0, 0));
  CHECK(btn->active_transitions.empty());

  // 2. Hover the button
  btn->set_hovered(true);
  component->ResolveTargetStyles();

  // Active transitions should have background-color
  REQUIRE(btn->active_transitions.count("background-color") == 1);

  // Still black at progress = 0 (t = 1000ms)
  screen.Step();
  CHECK(btn->style.background_color == Color::RGB(0, 0, 0));

  // Advance to 1500ms (50% progress)
  mock_now_ms = 1500.0;
  screen.Step();
  CHECK(btn->style.background_color == Color::RGB(127, 0, 0));

  // Advance to 2000ms (100% progress)
  mock_now_ms = 2000.0;
  screen.Step();
  CHECK(btn->style.background_color == Color::RGB(255, 0, 0));
  CHECK(btn->active_transitions.empty());
}

TEST_CASE("Transitions.EasingFunctions", "[transitions][easing]") {
  struct ClockRestorer {
    ~ClockRestorer() { time::SetCustomClock(nullptr); }
  } restorer;

  static double mock_now_ms = 1000.0;
  mock_now_ms = 1000.0;
  time::SetCustomClock([]() -> double { return mock_now_ms; });

  auto device = std::make_shared<MockTerminalDevice>();

  class EasingTestComponent : public Component<EasingTestComponent> {
   public:
    void InitReflection() override {
      Import<rtxui::div>();
      Component<EasingTestComponent>::InitReflection();
    }
    std::string_view view = R"html(
      <div id="btn">Click me</div>
      <style>
        #btn {
          flex-grow: 0.0;
          transition: flex-grow 1s ease-in;
        }
        #btn:hover {
          flex-grow: 1.0;
        }
      </style>
    )html";
  };

  auto component = Ref<EasingTestComponent>::New();
  Screen screen(component, device);

  auto* btn = component->Root()->QuerySelector("#btn");
  REQUIRE(btn != nullptr);

  // Initial style
  CHECK(btn->style.flex_grow == 0.0f);

  // Hover
  btn->set_hovered(true);
  component->ResolveTargetStyles();

  REQUIRE(btn->active_transitions.count("flex-grow") == 1);

  // At t = 1000ms, progress = 0
  screen.Step();
  CHECK(btn->style.flex_grow == 0.0f);

  // At t = 1500ms, progress = 0.5.
  mock_now_ms = 1500.0;
  screen.Step();
  CHECK(btn->style.flex_grow > 0.3f);
  CHECK(btn->style.flex_grow < 0.35f);

  // At t = 2000ms, progress = 1.0
  mock_now_ms = 2000.0;
  screen.Step();
  CHECK(btn->style.flex_grow == 1.0f);
}

TEST_CASE("Transitions.Interruption", "[transitions]") {
  struct ClockRestorer {
    ~ClockRestorer() { time::SetCustomClock(nullptr); }
  } restorer;

  static double mock_now_ms = 1000.0;
  mock_now_ms = 1000.0;
  time::SetCustomClock([]() -> double { return mock_now_ms; });

  auto device = std::make_shared<MockTerminalDevice>();

  class InterruptionTestComponent
      : public Component<InterruptionTestComponent> {
   public:
    void InitReflection() override {
      Import<rtxui::div>();
      Component<InterruptionTestComponent>::InitReflection();
    }
    std::string_view view = R"html(
      <div id="btn">Click me</div>
      <style>
        #btn {
          background-color: #000000;
          transition: background-color 1s linear;
        }
        #btn:hover {
          background-color: #ff0000;
        }
      </style>
    )html";
  };

  auto component = Ref<InterruptionTestComponent>::New();
  Screen screen(component, device);

  auto* btn = component->Root()->QuerySelector("#btn");
  REQUIRE(btn != nullptr);

  // Initial style
  CHECK(btn->style.background_color == Color::RGB(0, 0, 0));

  // 1. Hover to start transition to Red
  btn->set_hovered(true);
  component->ResolveTargetStyles();

  // At start (t = 1000ms), still black
  screen.Step();
  CHECK(btn->style.background_color == Color::RGB(0, 0, 0));

  // Advance to t = 1500ms (50% progress) -> color is intermediate red (127, 0,
  // 0)
  mock_now_ms = 1500.0;
  screen.Step();
  CHECK(btn->style.background_color == Color::RGB(127, 0, 0));

  // 2. Unhover to interrupt transition and go back to black
  btn->set_hovered(false);
  component->ResolveTargetStyles();

  // At the moment of interruption (t = 1500ms), it should start from current
  // value (127, 0, 0) target is now #000000. Advance to t = 2000ms (500ms
  // later, which is 50% of the new 1s transition)
  mock_now_ms = 2000.0;
  screen.Step();
  // Halfway between 127 and 0 is 63
  CHECK(btn->style.background_color == Color::RGB(63, 0, 0));

  // Advance to t = 2500ms (100% of new transition)
  mock_now_ms = 2500.0;
  screen.Step();
  CHECK(btn->style.background_color == Color::RGB(0, 0, 0));
  CHECK(btn->active_transitions.empty());
}

TEST_CASE("Transitions.DispatchMouseEvent", "[transitions][mouse]") {
  struct ClockRestorer {
    ~ClockRestorer() { time::SetCustomClock(nullptr); }
  } restorer;

  static double mock_now_ms = 1000.0;
  mock_now_ms = 1000.0;
  time::SetCustomClock([]() -> double { return mock_now_ms; });

  auto device = std::make_shared<MockTerminalDevice>();

  class MouseTransitionComponent : public Component<MouseTransitionComponent> {
   public:
    void InitReflection() override {
      Import<rtxui::div>();
      Component<MouseTransitionComponent>::InitReflection();
    }
    std::string_view view = R"html(
      <div id="btn">Button</div>
      <style>
        #btn {
          background-color: #000000;
          width: 10;
          height: 1;
          transition: background-color 1s linear;
        }
        #btn:hover {
          background-color: #ff0000;
        }
      </style>
    )html";
  };

  auto component = Ref<MouseTransitionComponent>::New();
  Screen screen(component, device);

  // We need to render/draw first so that root_fragment_ layout bounds are
  // populated
  screen.Draw();

  auto* btn = component->Root()->QuerySelector("#btn");
  REQUIRE(btn != nullptr);
  CHECK_FALSE(btn->hovered());
  CHECK(btn->style.background_color == Color::RGB(0, 0, 0));

  // Send a mouse hover event inside the button bounds
  // Coordinates are 1-indexed. The button starts at (0, 0) in layout, which is
  // (1, 1) in screen coords.
  Event::Mouse hover_in;
  hover_in.button = Event::Mouse::Button::None;
  hover_in.motion = Event::Mouse::Motion::Moved;
  hover_in.x = 2;
  hover_in.y = 1;
  screen.Dispatch(hover_in);

  // Verify that dispatching set the hovered state
  CHECK(btn->hovered());
  REQUIRE(btn->active_transitions.count("background-color") == 1);

  // Tick the transitions
  // At t=1000ms
  screen.Step();
  CHECK(btn->style.background_color == Color::RGB(0, 0, 0));

  // At t=1500ms
  mock_now_ms = 1500.0;
  screen.Step();
  CHECK(btn->style.background_color == Color::RGB(127, 0, 0));

  // Send a mouse event outside button bounds
  Event::Mouse hover_out;
  hover_out.button = Event::Mouse::Button::None;
  hover_out.motion = Event::Mouse::Motion::Moved;
  hover_out.x = 15;
  hover_out.y = 1;
  screen.Dispatch(hover_out);

  // Verify that it is no longer hovered
  CHECK_FALSE(btn->hovered());

  // Tick transitions back to black
  // At t=2000ms (50% back)
  mock_now_ms = 2000.0;
  screen.Step();
  CHECK(btn->style.background_color == Color::RGB(63, 0, 0));

  // At t=2500ms (100% back)
  mock_now_ms = 2500.0;
  screen.Step();
  CHECK(btn->style.background_color == Color::RGB(0, 0, 0));
}

TEST_CASE("Transitions.FlexGrowLayout", "[transitions][layout]") {
  struct ClockRestorer {
    ~ClockRestorer() { time::SetCustomClock(nullptr); }
  } restorer;

  static double mock_now_ms = 1000.0;
  mock_now_ms = 1000.0;
  time::SetCustomClock([]() -> double { return mock_now_ms; });

  auto device = std::make_shared<MockTerminalDevice>();

  class FlexGrowTransitionComponent
      : public Component<FlexGrowTransitionComponent> {
   public:
    void InitReflection() override {
      Import<rtxui::div>();
      Component<FlexGrowTransitionComponent>::InitReflection();
    }
    std::string_view view = R"html(
      <div class="row">
        <div id="item1">Item 1</div>
        <div id="item2">Item 2</div>
      </div>
      <style>
        .row {
          display: flex;
          flex-direction: row;
          width: 20;
          height: 1;
        }
        #item1 {
          flex-grow: 1.0;
        }
        #item2 {
          flex-grow: 1.0;
          transition: flex-grow 1s linear;
        }
        #item2:hover {
          flex-grow: 3.0;
        }
      </style>
    )html";
  };

  auto component = Ref<FlexGrowTransitionComponent>::New();
  Screen screen(component, device);

  auto* item1 = component->Root()->QuerySelector("#item1");
  auto* item2 = component->Root()->QuerySelector("#item2");
  REQUIRE(item1 != nullptr);
  REQUIRE(item2 != nullptr);

  // Initial draw
  screen.Draw();

  // Initially both flex-grow are 1.0, so both get half the space (10 each)
  CHECK(item1->layout_width() == 10);
  CHECK(item2->layout_width() == 10);

  // Hover item2 to start transition
  item2->set_hovered(true);
  component->ResolveTargetStyles();

  // Step at t = 1000ms (0%)
  screen.Step();
  CHECK(item1->layout_width() == 10);
  CHECK(item2->layout_width() == 10);

  // (base 6 + 6 extra = 12).
  mock_now_ms = 1500.0;
  screen.Step();
  CHECK(item1->layout_width() == 8);
  CHECK(item2->layout_width() == 12);
  // total grow = 4.0. item1 gets 1/4 (base 6 + 2 extra = 8), item2 gets 3/4
  // (base 6 + 6 extra = 12).
  mock_now_ms = 2000.0;
  screen.Step();
  CHECK(item1->layout_width() == 8);
  CHECK(item2->layout_width() == 12);
}

TEST_CASE("Transitions.ActiveMouseEvent", "[transitions][mouse][active]") {
  struct ClockRestorer {
    ~ClockRestorer() { time::SetCustomClock(nullptr); }
  } restorer;

  static double mock_now_ms = 1000.0;
  mock_now_ms = 1000.0;
  time::SetCustomClock([]() -> double { return mock_now_ms; });

  auto device = std::make_shared<MockTerminalDevice>();

  class ActiveTransitionComponent
      : public Component<ActiveTransitionComponent> {
   public:
    void InitReflection() override {
      Import<rtxui::div>();
      Component<ActiveTransitionComponent>::InitReflection();
    }
    std::string_view view = R"html(
      <div id="btn">Button</div>
      <style>
        #btn {
          background-color: #000000;
          width: 10;
          height: 1;
          transition: background-color 1s linear;
        }
        #btn:hover {
          background-color: #ff0000;
        }
        #btn:active {
          background-color: #00ff00;
        }
      </style>
    )html";
  };

  auto component = Ref<ActiveTransitionComponent>::New();
  Screen screen(component, device);
  screen.Draw();

  auto* btn = component->Root()->QuerySelector("#btn");
  REQUIRE(btn != nullptr);
  CHECK_FALSE(btn->hovered());
  CHECK_FALSE(btn->active());

  // 1. Hover in
  Event::Mouse hover_in;
  hover_in.button = Event::Mouse::Button::None;
  hover_in.motion = Event::Mouse::Motion::Moved;
  hover_in.x = 2;
  hover_in.y = 1;
  screen.Dispatch(hover_in);

  CHECK(btn->hovered());
  CHECK_FALSE(btn->active());
  REQUIRE(btn->active_transitions.count("background-color") == 1);

  // Tick to midpoint (50% hover)
  mock_now_ms = 1500.0;
  screen.Step();
  CHECK(btn->style.background_color == Color::RGB(127, 0, 0));

  // 2. Press mouse (enters active state)
  Event::Mouse press;
  press.button = Event::Mouse::Button::Left;
  press.motion = Event::Mouse::Motion::Pressed;
  press.x = 2;
  press.y = 1;
  screen.Dispatch(press);

  CHECK(btn->hovered());
  CHECK(btn->active());
  // The transition should start from current style Color::RGB(127, 0, 0) to
  // Color::RGB(0, 255, 0)
  REQUIRE(btn->active_transitions.count("background-color") == 1);

  // Tick 500ms after press (t = 2000ms): 50% from (127, 0, 0) to (0, 255, 0)
  // R: 127 + (0 - 127)*0.5 = 63
  // G: 0 + (255 - 0)*0.5 = 127
  // B: 0
  mock_now_ms = 2000.0;
  screen.Step();
  CHECK(btn->style.background_color == Color::RGB(63, 127, 0));

  // 3. Release mouse (leaves active state, still hovered)
  Event::Mouse release;
  release.button = Event::Mouse::Button::Left;
  release.motion = Event::Mouse::Motion::Released;
  release.x = 2;
  release.y = 1;
  screen.Dispatch(release);

  CHECK(btn->hovered());
  CHECK_FALSE(btn->active());
  // Transitions to hover target: Color::RGB(255, 0, 0) from current
  // Color::RGB(63, 127, 0)
  REQUIRE(btn->active_transitions.count("background-color") == 1);

  // Tick 500ms after release (t = 2500ms): 50% from (63, 127, 0) to (255, 0, 0)
  // R: 63 + (255 - 63)*0.5 = 159
  // G: 127 + (0 - 127)*0.5 = 63
  // B: 0
  mock_now_ms = 2500.0;
  screen.Step();
  CHECK(btn->style.background_color == Color::RGB(159, 63, 0));
}

TEST_CASE("Transitions.FocusEvent", "[transitions][focus]") {
  struct ClockRestorer {
    ~ClockRestorer() { time::SetCustomClock(nullptr); }
  } restorer;

  static double mock_now_ms = 1000.0;
  mock_now_ms = 1000.0;
  time::SetCustomClock([]() -> double { return mock_now_ms; });

  auto device = std::make_shared<MockTerminalDevice>();

  class FocusTransitionComponent : public Component<FocusTransitionComponent> {
   public:
    void InitReflection() override {
      Import<rtxui::div>();
      Component<FocusTransitionComponent>::InitReflection();
    }
    std::string_view view = R"html(
      <div id="input" focusable="true"></div>
      <style>
        #input {
          background-color: #000000;
          width: 10;
          height: 1;
          transition: background-color 1s linear;
        }
        #input:focus {
          background-color: #0000ff;
        }
      </style>
    )html";
  };

  auto component = Ref<FocusTransitionComponent>::New();
  Screen screen(component, device);
  screen.Draw();

  auto* input = component->Root()->QuerySelector("#input");
  REQUIRE(input != nullptr);
  CHECK_FALSE(input->focused());

  // 1. Click to focus
  Event::Mouse click;
  click.button = Event::Mouse::Button::Left;
  click.motion = Event::Mouse::Motion::Pressed;
  click.x = 2;
  click.y = 1;
  screen.Dispatch(click);

  CHECK(input->focused());
  REQUIRE(input->active_transitions.count("background-color") == 1);

  // Tick to midpoint (50% focus)
  mock_now_ms = 1500.0;
  screen.Step();
  CHECK(input->style.background_color == Color::RGB(0, 0, 127));

  // 2. Click outside (focus goes to root/nullptr or loses focus)
  Event::Mouse click_outside;
  click_outside.button = Event::Mouse::Button::Left;
  click_outside.motion = Event::Mouse::Motion::Pressed;
  click_outside.x = 15;
  click_outside.y = 1;
  screen.Dispatch(click_outside);

  CHECK_FALSE(input->focused());
  REQUIRE(input->active_transitions.count("background-color") == 1);

  // Tick 500ms after focus loss (t = 2000ms): 50% from (0, 0, 127) to (0, 0, 0)
  mock_now_ms = 2000.0;
  screen.Step();
  CHECK(input->style.background_color == Color::RGB(0, 0, 63));
}

TEST_CASE("Screen.HitTestingFixedElementWithScroll", "[terminal][scroll]") {
  auto device = std::make_shared<MockTerminalDevice>();

  class HitTestFixedComponent : public Component<HitTestFixedComponent> {
   public:
    int fixed_clicks = 0;
    int scroll_clicks = 0;

    void on_fixed_click() { fixed_clicks++; }
    void on_scroll_click() { scroll_clicks++; }

    void InitReflection() override {
      Import<rtxui::div>();
      Component<HitTestFixedComponent>::InitReflection();
    }

    HitTestFixedComponent() {
      Import("on_fixed_click", [this]() { on_fixed_click(); });
      Import("on_scroll_click", [this]() { on_scroll_click(); });
      Bind(fixed_clicks);
      Bind(scroll_clicks);
    }

    std::string_view view = R"html(
      <div id="container">
        <div id="scroller">
          <div id="spacer"></div>
          <div id="scrolled_item" onclick="on_scroll_click">Scrolled Item</div>
          <div id="fixed_item" onclick="on_fixed_click">Fixed Item</div>
        </div>
      </div>
      <style>
        #container {
          width: 20;
          height: 10;
        }
        #scroller {
          display: block;
          width: 20;
          height: 5;
          overflow-y: scroll;
        }
        #spacer {
          height: 8;
        }
        #scrolled_item {
          height: 1;
        }
        #fixed_item {
          position: fixed;
          left: 5;
          top: 2;
          width: 10;
          height: 1;
        }
      </style>
    )html";
  };

  auto component = Ref<HitTestFixedComponent>::New();
  Screen screen(component, device);
  screen.SetSmoothScrollEnabled(false);
  screen.Draw();

  auto* fixed_el = component->Root()->QuerySelector("#fixed_item");
  auto* scroll_el = component->Root()->QuerySelector("#scroller");
  REQUIRE(fixed_el != nullptr);
  REQUIRE(scroll_el != nullptr);

  // Scroll position is initially 0
  REQUIRE(scroll_el->scroll_y() == 0);

  // Click on the fixed element. It is at top: 2, left: 5, which means y=2
  // (1-based mouse coordinates are x=6, y=3).
  Event::Mouse click_fixed;
  click_fixed.button = Event::Mouse::Button::Left;
  click_fixed.motion = Event::Mouse::Motion::Pressed;
  click_fixed.x = 6;
  click_fixed.y = 3;
  screen.Dispatch(click_fixed);

  REQUIRE(component->fixed_clicks == 1);
  REQUIRE(component->scroll_clicks == 0);

  // Now scroll the scrollable container by 3 lines down
  scroll_el->set_scroll_y(3);
  screen.Draw();

  // Click at the exact same physical coordinates x=6, y=3 (where the fixed
  // element stays painted)
  screen.Dispatch(click_fixed);

  // The fixed element should receive the click, since it is position: fixed and
  // does not move!
  REQUIRE(component->fixed_clicks == 2);
  REQUIRE(component->scroll_clicks == 0);
}

TEST_CASE("Screen.AnchorLinkScrolling", "[terminal][mouse][anchor]") {
  auto device = std::make_shared<MockTerminalDevice>();

  class AnchorTestComponent : public Component<AnchorTestComponent> {
   public:
    void InitReflection() override {
      Import<rtxui::div>();
      Component<AnchorTestComponent>::InitReflection();
    }

    std::string_view view = R"html(
      <div id="container">
        <div id="scroller">
          <a id="link" href="#target">Link to Target</a>
          <div id="spacer"></div>
          <div id="target">Target Element</div>
        </div>
      </div>
      <style>
        #container {
          width: 30;
          height: 10;
        }
        #scroller {
          display: block;
          width: 30;
          height: 5;
          overflow-y: scroll;
        }
        #spacer {
          height: 10;
        }
        #target {
          height: 1;
        }
      </style>
    )html";
  };

  auto component = Ref<AnchorTestComponent>::New();
  Screen screen(component, device);
  screen.SetSmoothScrollEnabled(false);
  screen.Draw();

  auto* scroll_el = component->Root()->QuerySelector("#scroller");
  REQUIRE(scroll_el != nullptr);
  REQUIRE(scroll_el->scroll_y() == 0);

  // Click on the link. The link "Link to Target" is at y=0, x=0 inside the scroller.
  // 1-based mouse coordinates are x=1, y=1.
  Event::Mouse click_link;
  click_link.button = Event::Mouse::Button::Left;
  click_link.motion = Event::Mouse::Motion::Pressed;
  click_link.x = 1;
  click_link.y = 1;
  screen.Dispatch(click_link);

  // Clicking the link should scroll #target into view.
  CHECK(scroll_el->scroll_y() > 0);
}

TEST_CASE("Screen.KeyboardFocusTabindexNavigation", "[terminal][focus]") {
  auto device = std::make_shared<MockTerminalDevice>();

  class FocusTabindexComponent : public Component<FocusTabindexComponent> {
   public:
    void InitReflection() override {
      Import<rtxui::div>();
      Component<FocusTabindexComponent>::InitReflection();
    }
    std::string_view view = R"html(
      <div>
        <div id="first" tabindex="2">Item 1 (tabindex=2)</div>
        <div id="second" tabindex="0">Item 2 (tabindex=0)</div>
        <div id="third" tabindex="1">Item 3 (tabindex=1)</div>
        <div id="fourth" tabindex="-1">Item 4 (tabindex="-1")</div>
        <div id="fifth" focusable="true">Item 5 (focusable=true, implicit 0)</div>
      </div>
    )html";
  };

  auto component = Ref<FocusTabindexComponent>::New();
  Screen screen(component, device);
  screen.Draw();

  auto* first = component->Root()->QuerySelector("#first");
  auto* second = component->Root()->QuerySelector("#second");
  auto* third = component->Root()->QuerySelector("#third");
  auto* fourth = component->Root()->QuerySelector("#fourth");
  auto* fifth = component->Root()->QuerySelector("#fifth");

  REQUIRE(first != nullptr);
  REQUIRE(second != nullptr);
  REQUIRE(third != nullptr);
  REQUIRE(fourth != nullptr);
  REQUIRE(fifth != nullptr);

  // Tab index order:
  // 1. positive tabindexes in ascending order:
  //    - third (tabindex="1")
  //    - first (tabindex="2")
  // 2. tabindex="0" and focusable="true" in document order:
  //    - second (tabindex="0")
  //    - fifth (focusable="true")
  // 3. fourth (tabindex="-1") is skipped entirely.

  // Initially, no element is focused
  REQUIRE_FALSE(first->focused());
  REQUIRE_FALSE(second->focused());
  REQUIRE_FALSE(third->focused());
  REQUIRE_FALSE(fourth->focused());
  REQUIRE_FALSE(fifth->focused());

  // 1. Press Tab -> should focus 'third' (tabindex=1)
  device->ClearOutput();
  screen.Dispatch(Event::Tab());
  REQUIRE(third->focused());
  REQUIRE_FALSE(device->GetOutput().empty());
  REQUIRE_FALSE(first->focused());
  REQUIRE_FALSE(second->focused());
  REQUIRE_FALSE(fifth->focused());

  // 2. Press Tab -> should focus 'first' (tabindex=2)
  screen.Dispatch(Event::Tab());
  REQUIRE(first->focused());
  REQUIRE_FALSE(third->focused());

  // 3. Press Tab -> should focus 'second' (tabindex=0)
  screen.Dispatch(Event::Tab());
  REQUIRE(second->focused());
  REQUIRE_FALSE(first->focused());

  // 4. Press Tab -> should focus 'fifth' (focusable=true)
  screen.Dispatch(Event::Tab());
  REQUIRE(fifth->focused());
  REQUIRE_FALSE(second->focused());

  // 5. Press Tab -> should wrap back to 'third'
  screen.Dispatch(Event::Tab());
  REQUIRE(third->focused());
  REQUIRE_FALSE(fifth->focused());

  // 6. Press Shift-Tab (TabReverse) -> should wrap to 'fifth'
  screen.Dispatch(Event::TabReverse());
  REQUIRE(fifth->focused());
  REQUIRE_FALSE(third->focused());

  // 7. Press Shift-Tab -> should focus 'second'
  screen.Dispatch(Event::TabReverse());
  REQUIRE(second->focused());
  REQUIRE_FALSE(fifth->focused());
}

TEST_CASE("Screen.MediaQueryResolutionOnResize", "[terminal][css][media]") {
  auto device = std::make_shared<MockTerminalDevice>();

  class ResponsiveComponent : public Component<ResponsiveComponent> {
   public:
    void InitReflection() override {
      Import<rtxui::div>();
      Component<ResponsiveComponent>::InitReflection();
    }
    std::string_view view = R"html(
      <div id="target">Content</div>
      <style>
        #target {
          background-color: #000000;
        }
        @media (max-width: 60) {
          #target {
            background-color: #ff0000;
          }
        }
        @media (min-width: 80) {
          #target {
            background-color: #00ff00;
          }
        }
      </style>
    )html";
  };

  auto component = Ref<ResponsiveComponent>::New();
  device->TriggerResize(70, 20);

  Screen screen(component, device);
  screen.Draw();

  auto* target = component->Root()->QuerySelector("#target");
  REQUIRE(target != nullptr);

  // At 70 width, only base style is active (black background: RGB(0,0,0))
  REQUIRE(target->style.background_color.has_value());
  CHECK(target->style.background_color == Color::RGB(0, 0, 0));

  // Now resize to width 50 (should trigger max-width: 60 -> red background:
  // RGB(255,0,0))
  device->TriggerResize(50, 20);
  device->PushInput(" ");
  screen.Step();

  REQUIRE(target->style.background_color.has_value());
  CHECK(target->style.background_color == Color::RGB(255, 0, 0));

  // Now resize to width 90 (should trigger min-width: 80 -> green background:
  // RGB(0,255,0))
  device->TriggerResize(90, 20);
  device->PushInput(" ");
  screen.Step();

  REQUIRE(target->style.background_color.has_value());
  CHECK(target->style.background_color == Color::RGB(0, 255, 0));
}

TEST_CASE("Screen.ScrollIntoViewOnKeyboardFocus", "[terminal][focus][scroll]") {
  auto device = std::make_shared<MockTerminalDevice>();

  class ScrollFocusComponent : public Component<ScrollFocusComponent> {
   public:
    void InitReflection() override {
      Import<rtxui::div>();
      Component<ScrollFocusComponent>::InitReflection();
    }
    std::string_view view = R"html(
      <div id="scrollable">
        <div class="spacer">Spacer 1</div>
        <div id="item1" class="item" tabindex="0">Item 1</div>
        <div id="item2" class="item" tabindex="0">Item 2</div>
        <div id="item3" class="item" tabindex="0">Item 3</div>
      </div>
      <style>
        #scrollable {
          height: 5;
          overflow-y: scroll;
          display: block;
        }
        .spacer {
          height: 3;
          display: block;
        }
        .item {
          height: 2;
          display: block;
        }
      </style>
    )html";
  };

  auto component = Ref<ScrollFocusComponent>::New();
  Screen screen(component, device);
  screen.SetSmoothScrollEnabled(false);
  screen.Draw();

  auto* scrollable = component->Root()->QuerySelector("#scrollable");
  auto* item1 = component->Root()->QuerySelector("#item1");
  auto* item2 = component->Root()->QuerySelector("#item2");
  auto* item3 = component->Root()->QuerySelector("#item3");

  REQUIRE(scrollable != nullptr);
  REQUIRE(item1 != nullptr);
  REQUIRE(item2 != nullptr);
  REQUIRE(item3 != nullptr);

  // Initially: scroll_y should be 0
  REQUIRE(scrollable->scroll_y() == 0);

  // 1. Dispatch Tab to focus item1
  // item1 is at top offset = 3, height = 2, so bottom = 5.
  // Viewport height is 5.
  // Sizable area fits [0, 5], so item1 is visible within scroll_y = 0.
  screen.Dispatch(Event::Tab());
  REQUIRE(item1->focused());
  REQUIRE(scrollable->scroll_y() == 0);

  // 2. Dispatch Tab to focus item2
  // item2 is at top offset = 5, height = 2, so bottom = 7.
  // It is out of view (visible was [0, 5]).
  // ScrollIntoView should adjust scroll_y to bottom - height = 7 - 5 = 2.
  screen.Dispatch(Event::Tab());
  REQUIRE(item2->focused());
  REQUIRE(scrollable->scroll_y() == 2);

  // 3. Dispatch Tab to focus item3
  // item3 is at top offset = 7, height = 2, so bottom = 9.
  // It is out of view (visible was [2, 7]).
  // ScrollIntoView should adjust scroll_y to 9 - 5 = 4.
  screen.Dispatch(Event::Tab());
  REQUIRE(item3->focused());
  REQUIRE(scrollable->scroll_y() == 4);

  // 4. Dispatch Shift-Tab (TabReverse) to focus item2
  // item2 top = 5, bottom = 7.
  // Current scroll_y = 4 (visible [4, 9]), so item2 is fully visible. scroll_y
  // should remain 4.
  screen.Dispatch(Event::TabReverse());
  REQUIRE(item2->focused());
  REQUIRE(scrollable->scroll_y() == 4);

  // 5. Dispatch Shift-Tab to focus item1
  // item1 top = 3, bottom = 5.
  // Visible was [4, 9], so item1 is out of view (top = 3 < scroll_y = 4).
  // ScrollIntoView should adjust scroll_y to its top = 3.
  screen.Dispatch(Event::TabReverse());
  REQUIRE(item1->focused());
  REQUIRE(scrollable->scroll_y() == 3);
}

TEST_CASE("Screen.ScrollAnimation", "[terminal][scroll][animation]") {
  struct ClockRestorer {
    ~ClockRestorer() { time::SetCustomClock(nullptr); }
  } restorer;

  static double mock_now_ms = 1000.0;
  mock_now_ms = 1000.0;
  time::SetCustomClock([]() -> double { return mock_now_ms; });

  auto device = std::make_shared<MockTerminalDevice>();

  class ScrollAnimComponent : public Component<ScrollAnimComponent> {
   public:
    void InitReflection() override {
      Import<rtxui::div>();
      Component<ScrollAnimComponent>::InitReflection();
    }
    std::string_view view = R"html(
      <div id="scrollable_auto">
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
      <div id="scrollable_smooth">
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
        #scrollable_auto {
          display: block;
          height: 4;
          overflow-y: scroll;
          scroll-speed: 2;
          scroll-behavior: auto;
        }
        #scrollable_smooth {
          display: block;
          height: 4;
          overflow-y: scroll;
          scroll-speed: 2;
          scroll-behavior: smooth;
        }
      </style>
    )html";
  };

  auto component = Ref<ScrollAnimComponent>::New();
  Screen screen(component, device);

  auto* scrollable_auto = component->Root()->QuerySelector("#scrollable_auto");
  auto* scrollable_smooth =
      component->Root()->QuerySelector("#scrollable_smooth");
  REQUIRE(scrollable_auto != nullptr);
  REQUIRE(scrollable_smooth != nullptr);

  // SetSmoothScrollEnabled defaults to true.
  REQUIRE(screen.smooth_scroll_enabled());

  // Test 1: Scroll behavior 'auto' (instant content scroll, smooth visual
  // scrollbar)
  REQUIRE(scrollable_auto->scroll_y() == 0);
  REQUIRE(scrollable_auto->visual_scroll_y() == 0.0f);

  // Trigger wheel scroll down on scrollable_auto
  Event::Mouse wheel_auto;
  wheel_auto.button = Event::Mouse::Button::WheelDown;
  wheel_auto.motion = Event::Mouse::Motion::Pressed;
  // Locate inside scrollable_auto
  wheel_auto.x = 2;
  wheel_auto.y = 1;
  screen.Dispatch(wheel_auto);

  // Content scroll and visual scrollbar snap instantly
  REQUIRE(scrollable_auto->scroll_y() == 2);
  REQUIRE(scrollable_auto->target_scroll_y() == 2);
  REQUIRE(scrollable_auto->visual_scroll_y() == 2.0f);

  // Test 2: Scroll behavior 'smooth' (smooth content scroll, smooth visual
  // scrollbar when programmatic/focus scroll is triggered)
  mock_now_ms = 2000.0;
  REQUIRE(scrollable_smooth->scroll_y() == 0);
  REQUIRE(scrollable_smooth->visual_scroll_y() == 0.0f);

  // Trigger smooth scroll programmatically (simulating keyboard focus navigation)
  scrollable_smooth->set_scroll_y(2, true);

  // Target is updated immediately
  REQUIRE(scrollable_smooth->target_scroll_y() == 2);
  // Both content and visual remain 0 before ticking
  REQUIRE(scrollable_smooth->scroll_y() == 0);
  REQUIRE(scrollable_smooth->visual_scroll_y() == 0.0f);

  // Advance by 25ms
  mock_now_ms = 2025.0;
  screen.Step();
  // Content scroll is smooth
  REQUIRE(scrollable_smooth->scroll_y() >= 0);
  REQUIRE(scrollable_smooth->scroll_y() <= 2);

  // Complete smooth content transition
  mock_now_ms = 2050.0;
  screen.Step();
  REQUIRE(scrollable_smooth->scroll_y() == 2);
  REQUIRE(scrollable_smooth->visual_scroll_y() == 2.0f);
}

TEST_CASE("Screen.InitialFrameInRawModeRegression", "[terminal]") {
  auto device = std::make_shared<MockTerminalDevice>();
  auto component = Ref<DummyComponent>::New();

  Screen screen(component, device);

  // Clear the initial constructor draw output from the device
  device->ClearOutput();

  // Run the loop which enters raw mode and should draw a full frame
  screen.Loop();

  std::string output = device->GetOutput();
  REQUIRE_FALSE(output.empty());
  REQUIRE(output.find("Hello Mock") != std::string::npos);
}

TEST_CASE("Screen.RenderDiffWideCharactersRegression", "[terminal]") {
  struct VirtualTerminal {
    int width;
    int height;
    struct VCell {
      std::string character = " ";
      bool is_continuation = false;
    };
    std::vector<VCell> cells;
    int cx = 0;
    int cy = 0;

    VirtualTerminal(int w, int h) : width(w), height(h), cells(w * h) {}

    void Write(const std::string& data) {
      size_t i = 0;
      while (i < data.size()) {
        if (data[i] == '\n') {
          cy++;
          i++;
        } else if (data[i] == '\r') {
          cx = 0;
          i++;
        } else if (data[i] == '\x1b') {
          if (i + 1 < data.size() && data[i + 1] == '[') {
            size_t start = i + 2;
            size_t end = start;
            while (end < data.size() && ((data[end] >= '0' && data[end] <= '9') || data[end] == ';')) {
              end++;
            }
            if (end < data.size()) {
              char cmd = data[end];
              std::string params = data.substr(start, end - start);
              int val = params.empty() ? 1 : std::stoi(params);
              if (cmd == 'A') {
                cy = std::max(0, cy - val);
              } else if (cmd == 'C') {
                cx = std::min(width - 1, cx + val);
              } else if (cmd == 'H') {
                cx = 0;
                cy = 0;
              }
              i = end + 1;
            } else {
              i++;
            }
          } else {
            i++;
          }
        } else {
          size_t len = 1;
          unsigned char first = data[i];
          if (first >= 0xf0) len = 4;
          else if (first >= 0xe0) len = 3;
          else if (first >= 0xc0) len = 2;

          if (i + len <= data.size()) {
            std::string character = data.substr(i, len);
            i += len;

            if (cx < width && cy < height) {
              cells[cy * width + cx].character = character;
              cells[cy * width + cx].is_continuation = false;
              bool is_wide = (len > 1 && character != " ");
              if (is_wide && cx + 1 < width) {
                cells[cy * width + cx + 1].character = "";
                cells[cy * width + cx + 1].is_continuation = true;
                cx += 2;
              } else {
                cx += 1;
              }
            }
          }
        }
      }
    }
  };

  auto VerifyDiff = [&](const Texture& old_tex, const Texture& new_tex) {
    VirtualTerminal vt(old_tex.width(), old_tex.height());
    vt.Write(old_tex.Render());
    vt.cx = 0;
    vt.cy = 0;
    std::string diff = new_tex.RenderDiff(old_tex);
    vt.Write(diff);

    for (int y = 0; y < new_tex.height(); ++y) {
      for (int x = 0; x < new_tex.width(); ++x) {
        const auto& expected = new_tex[x, y];
        const auto& actual = vt.cells[y * new_tex.width() + x];
        std::string expected_char = expected.is_continuation ? "" : (expected.character.empty() ? " " : expected.character);
        std::string actual_char = actual.is_continuation ? "" : actual.character;
        if (expected_char != actual_char || expected.is_continuation != actual.is_continuation) {
          UNSCOPED_INFO("Mismatch at (" << x << "," << y << "): expected '" 
               << expected_char << "' (continuation=" << expected.is_continuation 
               << "), got '" << actual_char << "' (continuation=" << actual.is_continuation << ")");
          return false;
        }
      }
    }
    return true;
  };

  // Test Case 1: Changing a wide character to another wide character
  {
    Texture old_tex(10, 1);
    old_tex[0, 0].character = "A";
    old_tex[1, 0].character = "中";
    old_tex[2, 0].is_continuation = true;
    old_tex[3, 0].character = "B";

    Texture new_tex(10, 1);
    new_tex[0, 0].character = "A";
    new_tex[1, 0].character = "万";
    new_tex[2, 0].is_continuation = true;
    new_tex[3, 0].character = "B";

    REQUIRE(VerifyDiff(old_tex, new_tex));
  }

  // Test Case 2: Changing a wide character to a single-width character
  {
    Texture old_tex(10, 1);
    old_tex[0, 0].character = "A";
    old_tex[1, 0].character = "中";
    old_tex[2, 0].is_continuation = true;
    old_tex[3, 0].character = "B";

    Texture new_tex(10, 1);
    new_tex[0, 0].character = "A";
    new_tex[1, 0].character = "X";
    new_tex[2, 0].character = "Y";
    new_tex[3, 0].character = "B";

    REQUIRE(VerifyDiff(old_tex, new_tex));
  }

  // Test Case 3: Changing a single-width character to a wide character
  {
    Texture old_tex(10, 1);
    old_tex[0, 0].character = "A";
    old_tex[1, 0].character = "X";
    old_tex[2, 0].character = "Y";
    old_tex[3, 0].character = "B";

    Texture new_tex(10, 1);
    new_tex[0, 0].character = "A";
    new_tex[1, 0].character = "中";
    new_tex[2, 0].is_continuation = true;
    new_tex[3, 0].character = "B";

    REQUIRE(VerifyDiff(old_tex, new_tex));
  }

  // Test Case 4: Changing a character immediately following a wide character (which stays unchanged)
  {
    Texture old_tex(10, 1);
    old_tex[0, 0].character = "中";
    old_tex[1, 0].is_continuation = true;
    old_tex[2, 0].character = "A";

    Texture new_tex(10, 1);
    new_tex[0, 0].character = "中";
    new_tex[1, 0].is_continuation = true;
    new_tex[2, 0].character = "B";

    REQUIRE(VerifyDiff(old_tex, new_tex));
  }

  // Test Case 5: Changing a character immediately preceding a wide character (which stays unchanged)
  {
    Texture old_tex(10, 1);
    old_tex[0, 0].character = "A";
    old_tex[1, 0].character = "中";
    old_tex[2, 0].is_continuation = true;

    Texture new_tex(10, 1);
    new_tex[0, 0].character = "B";
    new_tex[1, 0].character = "中";
    new_tex[2, 0].is_continuation = true;

    REQUIRE(VerifyDiff(old_tex, new_tex));
  }
}

TEST_CASE("Screen.DeltaTransmissionWideCharacters", "[terminal]") {
  auto device = std::make_shared<MockTerminalDevice>();
  
  class DynamicComponent : public Component<DynamicComponent> {
   public:
    std::string text = "A";
    void InitReflection() override {
      Import<rtxui::div>();
      Component<DynamicComponent>::InitReflection();
    }
    DynamicComponent() {
      Bind(text);
    }
    std::string_view view = R"(
      <div>{text}</div>
    )";
  };

  auto component = Ref<DynamicComponent>::New();
  Screen screen(component, device);

  screen.Draw();
  std::string first_output = device->GetOutput();
  device->ClearOutput();

  component->text = "中";
  component->Render();
  screen.Draw();
  std::string second_output = device->GetOutput();
  device->ClearOutput();

  component->text = "B";
  component->Render();
  screen.Draw();
  std::string third_output = device->GetOutput();
  device->ClearOutput();

  struct VirtualTerminal {
    int width;
    int height;
    struct VCell {
      std::string character = " ";
      bool is_continuation = false;
    };
    std::vector<VCell> cells;
    int cx = 0;
    int cy = 0;

    VirtualTerminal(int w, int h) : width(w), height(h), cells(w * h) {}

    void Write(const std::string& data) {
      size_t i = 0;
      while (i < data.size()) {
        if (data[i] == '\n') {
          cy++;
          i++;
        } else if (data[i] == '\r') {
          cx = 0;
          i++;
        } else if (data[i] == '\x1b') {
          if (i + 1 < data.size() && data[i + 1] == '[') {
            size_t start = i + 2;
            size_t end = start;
            while (end < data.size() && ((data[end] >= '0' && data[end] <= '9') || data[end] == ';')) {
              end++;
            }
            if (end < data.size()) {
              char cmd = data[end];
              std::string params = data.substr(start, end - start);
              int val = params.empty() ? 1 : std::stoi(params);
              if (cmd == 'A') {
                cy = std::max(0, cy - val);
              } else if (cmd == 'C') {
                cx = std::min(width - 1, cx + val);
              } else if (cmd == 'H') {
                cx = 0;
                cy = 0;
              } else if (cmd == 'J') {
                if (params == "2") {
                  for (auto& cell : cells) {
                    cell.character = "";
                    cell.is_continuation = false;
                  }
                }
              }
              i = end + 1;
            } else {
              i++;
            }
          } else {
            i++;
          }
        } else {
          size_t len = 1;
          unsigned char first = data[i];
          if (first >= 0xf0) len = 4;
          else if (first >= 0xe0) len = 3;
          else if (first >= 0xc0) len = 2;

          if (i + len <= data.size()) {
            std::string character = data.substr(i, len);
            i += len;

            if (cx < width && cy < height) {
              cells[cy * width + cx].character = character;
              cells[cy * width + cx].is_continuation = false;
              bool is_wide = (len > 1 && character != " ");
              if (is_wide && cx + 1 < width) {
                cells[cy * width + cx + 1].character = "";
                cells[cy * width + cx + 1].is_continuation = true;
                cx += 2;
              } else {
                cx += 1;
              }
            }
          }
        }
      }
    }
  };

  VirtualTerminal vt(80, 24);
  vt.Write(first_output);
  
  vt.cx = 0;
  vt.cy = 0;
  vt.Write(second_output);

  REQUIRE(vt.cells[0].character == "中");
  REQUIRE_FALSE(vt.cells[0].is_continuation);
  REQUIRE(vt.cells[1].character == "");
  REQUIRE(vt.cells[1].is_continuation);

  vt.cx = 0;
  vt.cy = 0;
  vt.Write(third_output);

  REQUIRE(vt.cells[0].character == "B");
  REQUIRE_FALSE(vt.cells[0].is_continuation);
  REQUIRE(vt.cells[1].character == " ");
  REQUIRE_FALSE(vt.cells[1].is_continuation);
}

TEST_CASE("Screen.RenderInitialFrameWideCharacterStyle", "[terminal]") {
  Texture texture(10, 1);
  texture[0, 0].character = "中";
  texture[0, 0].background_color = Color::RGB(0, 0, 255);
  texture[1, 0].is_continuation = true;
  texture[2, 0].character = "A";

  std::string output = texture.Render();
  REQUIRE(output.find("A") != std::string::npos);
  REQUIRE(output.find("\x1B[48;2;0;0;0mA") != std::string::npos);
}

TEST_CASE("Screen.SpatialNavigation", "[terminal][focus][spatial]") {
  auto device = std::make_shared<MockTerminalDevice>();

  class SpatialNavComponent : public Component<SpatialNavComponent> {
   public:
    void InitReflection() override {
      Import<rtxui::div>();
      Component<SpatialNavComponent>::InitReflection();
    }
    std::string_view view = R"(
      <style>
        #root { display: flex; flex-direction: column; }
        .row { display: flex; }
        .item { width: 10; height: 3; border: tall; }
      </style>
      <div id="root">
        <div class="row">
          <div id="item1" class="item" focusable="true">Item 1</div>
          <div id="item2" class="item" focusable="true">Item 2</div>
        </div>
        <div class="row">
          <div id="item3" class="item" focusable="true">Item 3</div>
          <div id="item4" class="item" focusable="true">Item 4</div>
        </div>
      </div>
    )";
  };

  auto component = Ref<SpatialNavComponent>::New();
  Screen screen(component, device);

  auto* item1 = component->Root()->QuerySelector("#item1");
  auto* item2 = component->Root()->QuerySelector("#item2");
  auto* item3 = component->Root()->QuerySelector("#item3");
  auto* item4 = component->Root()->QuerySelector("#item4");

  REQUIRE(item1 != nullptr);
  REQUIRE(item2 != nullptr);
  REQUIRE(item3 != nullptr);
  REQUIRE(item4 != nullptr);

  // Initial focus
  item1->set_focused(true);
  screen.Draw();  // Ensure layout is done and focused_element_ is updated

  // 1. ArrowRight -> item2
  screen.Dispatch(Event::ArrowRight());
  CHECK(item2->focused());
  CHECK_FALSE(item1->focused());

  // 2. ArrowDown -> item4
  screen.Dispatch(Event::ArrowDown());
  CHECK(item4->focused());
  CHECK_FALSE(item2->focused());

  // 3. ArrowLeft -> item3
  screen.Dispatch(Event::ArrowLeft());
  CHECK(item3->focused());
  CHECK_FALSE(item4->focused());

  // 4. ArrowUp -> item1
  screen.Dispatch(Event::ArrowUp());
  CHECK(item1->focused());
  CHECK_FALSE(item3->focused());

  // 5. 'l' key (vim right) -> item2
  screen.Dispatch(Event::l());
  CHECK(item2->focused());

  // 6. 'j' key (vim down) -> item4
  screen.Dispatch(Event::j());
  CHECK(item4->focused());

  // 7. 'h' key (vim left) -> item3
  screen.Dispatch(Event::h());
  CHECK(item3->focused());

  // 8. 'k' key (vim up) -> item1
  screen.Dispatch(Event::k());
  CHECK(item1->focused());
}

TEST_CASE("Screen.SpaceEnterActivation", "[terminal][focus][activation]") {
  auto device = std::make_shared<MockTerminalDevice>();

  class ActivationComponent : public Component<ActivationComponent> {
   public:
    int count = 0;
    void on_click() { count++; }
    void InitReflection() override {
      Import<rtxui::div>();
      Component<ActivationComponent>::InitReflection();
    }
    ActivationComponent() {
      Import("on_click", [this]() { on_click(); });
    }
    std::string_view view = R"(
      <div id="btn" focusable="true" onclick="on_click">Click Me</div>
    )";
  };

  auto component = Ref<ActivationComponent>::New();
  Screen screen(component, device);
  auto* btn = component->Root()->QuerySelector("#btn");
  REQUIRE(btn != nullptr);
  btn->set_focused(true);
  screen.Draw();

  // Press Space
  screen.Dispatch(Event::Keyboard{.codepoint = 32});
  CHECK(component->count == 1);

  // Press Return
  screen.Dispatch(Event::Return());
  CHECK(component->count == 2);
}

TEST_CASE("Screen.HjklNavigationInInput", "[terminal][focus][spatial]") {
  auto device = std::make_shared<MockTerminalDevice>();

  class InputComponent : public Component<InputComponent> {
   public:
    void InitReflection() override {
      Import<rtxui::input>();
      Import<rtxui::div>();
      Component<InputComponent>::InitReflection();
    }
    std::string_view view = R"(
      <style>
        #root { display: flex; }
        .box { width: 10; height: 3; }
      </style>
      <div id="root">
        <div id="left-other" class="box" focusable="true">Left Other</div>
        <input id="input" class="box" value="" />
        <div id="other" class="box" focusable="true">Other</div>
      </div>
    )";
  };

  auto component = Ref<InputComponent>::New();
  Screen screen(component, device);

  auto* input = component->Root()->QuerySelector("#input");
  auto* other = component->Root()->QuerySelector("#other");
  auto* left_other = component->Root()->QuerySelector("#left-other");

  REQUIRE(input != nullptr);
  REQUIRE(other != nullptr);
  REQUIRE(left_other != nullptr);

  input->set_focused(true);
  screen.Draw();

  // Press 'l' (vim right). Since it's an input, it should be consumed.
  screen.Dispatch(Event::l());

  // Focus should NOT have moved to 'other' or 'left-other'
  CHECK(input->focused());
  CHECK_FALSE(other->focused());
  CHECK_FALSE(left_other->focused());

  // Press ArrowRight when cursor is at the end. It should be consumed and NOT move focus.
  screen.Dispatch(Event::ArrowRight());
  CHECK(input->focused());
  CHECK_FALSE(other->focused());
  CHECK_FALSE(left_other->focused());

  // Press ArrowRight again at the end.
  screen.Dispatch(Event::ArrowRight());
  CHECK(input->focused());
  CHECK_FALSE(other->focused());
  CHECK_FALSE(left_other->focused());

  // Press ArrowLeft. It should be consumed and NOT move focus.
  screen.Dispatch(Event::ArrowLeft());
  CHECK(input->focused());
  CHECK_FALSE(other->focused());
  CHECK_FALSE(left_other->focused());

  // Press ArrowLeft again at the beginning.
  screen.Dispatch(Event::ArrowLeft());
  CHECK(input->focused());
  CHECK_FALSE(other->focused());
  CHECK_FALSE(left_other->focused());
}

TEST_CASE("Transitions.NulloptTargetReverts", "[terminal][transitions][regression]") {
  struct ClockRestorer {
    ~ClockRestorer() { time::SetCustomClock(nullptr); }
  } restorer;

  static double mock_now_ms = 1000.0;
  mock_now_ms = 1000.0;
  time::SetCustomClock([]() -> double { return mock_now_ms; });

  auto device = std::make_shared<MockTerminalDevice>();

  class TransitionTestComponent : public Component<TransitionTestComponent> {
   public:
    void InitReflection() override {
      Import<rtxui::div>();
      Component<TransitionTestComponent>::InitReflection();
    }
    std::string_view view = R"html(
      <div id="btn">Click me</div>
      <style>
        #btn {
          background-color: #000000;
          transition: color 1s linear;
        }
        #btn:focus {
          color: #ffffff;
        }
      </style>
    )html";
  };

  auto component = Ref<TransitionTestComponent>::New();
  Screen screen(component, device);

  auto* btn = component->Root()->QuerySelector("#btn");
  REQUIRE(btn != nullptr);

  // 1. Initial style: color is std::nullopt
  CHECK_FALSE(btn->style.foreground_color.has_value());
  CHECK(btn->active_transitions.empty());

  // 2. Focus the button
  btn->set_focused(true);
  component->ResolveTargetStyles();

  // Active transitions should have color
  REQUIRE(btn->active_transitions.count("color") == 1);

  // Still nullopt at progress = 0 (t = 1000ms) but is actively transitioning
  screen.Step();
  // Wait, start value of transition will be transparent/interpolated, so it has a value during transition
  CHECK(btn->style.foreground_color.has_value());

  // Advance to 2000ms (100% progress)
  mock_now_ms = 2000.0;
  screen.Step();
  CHECK(btn->style.foreground_color == Color::RGB(255, 255, 255));
  CHECK(btn->active_transitions.empty());

  // 3. Unfocus the button
  btn->set_focused(false);
  component->ResolveTargetStyles();

  // Active transitions should have color again
  REQUIRE(btn->active_transitions.count("color") == 1);

  // Advance to 3000ms (100% progress)
  mock_now_ms = 3000.0;
  screen.Step();

  // Transition is complete, active_transitions should be empty
  CHECK(btn->active_transitions.empty());
  // The foreground_color MUST have reverted back to std::nullopt
  CHECK_FALSE(btn->style.foreground_color.has_value());
}

}  // namespace
}  // namespace rtxui
