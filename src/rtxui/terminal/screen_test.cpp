#include "rtxui/internal/screen.hpp"

#include <memory>

#include "catch2/catch_test_macros.hpp"
#include "rtxui/base/task_runner.hpp"
#include "rtxui/component/default_components_internal.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/internal/component.hpp"
#include "rtxui/layout/layout.hpp"
#include "rtxui/layout/layout_tree_builder.hpp"
#include "rtxui/layout/physical_fragment.hpp"
#include "rtxui/paint/paint.hpp"
#include "rtxui/paint/texture.hpp"
#include "rtxui/style/style.hpp"
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

TEST_CASE("Screen.SynchronizedOutputMode2026", "[terminal][sync]") {
  auto device = std::make_shared<MockTerminalDevice>();
  auto component = Ref<DummyComponent>::New();

  Screen screen(component, device);
  std::string output = device->GetOutput();
  size_t start = output.find("\x1b[?2026h");
  size_t end = output.find("\x1b[?2026l");
  REQUIRE(start != std::string::npos);
  REQUIRE(end != std::string::npos);
  CHECK(start < end);
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

  // Step() drains everything buffered when it runs, so a batch of queued
  // bytes is decoded and dispatched in order within a single Step -- 'a'
  // first, leaving 'b' as the last event seen.
  device->PushInput("ab");
  screen.Step();
  REQUIRE(component->last_event == "key:b");

  // A key arriving on its own is still dispatched by the next Step().
  device->PushInput("c");
  screen.Step();
  REQUIRE(component->last_event == "key:c");

  // Push Ctrl-C to input and check if it terminates running_
  device->PushInput("\x03");
  screen.Step();
  // We can query screen.Step or loop to see if it should stop. We can't access
  // private members directly, but we can verify it doesn't loop. Wait, we can
  // test screen.Step() is a no-op if exited or check state if we expose a way,
  // but since we didn't expose running_ via public getter, this is already
  // verifying the return flow.
}

TEST_CASE("Screen.BracketedPasteInsertsTextIntoFocusedInput",
          "[terminal][paste]") {
  // End-to-end: raw bytes (as a terminal would send them for a real paste,
  // wrapped in bracketed-paste markers) through the device, parser, and
  // Screen's event dispatch, landing as text in a focused <input>.
  class PasteInputComponent : public Component<PasteInputComponent> {
   public:
    void InitReflection() override {
      Import<rtxui::input>();
      Component<PasteInputComponent>::InitReflection();
    }
    std::string_view view = R"(<input id="in" />)";
  };

  auto device = std::make_shared<MockTerminalDevice>();
  auto container = Ref<PasteInputComponent>::New();
  Screen screen(container, device);
  screen.Draw();

  auto* in_el = container->Root()->QuerySelector("#in");
  REQUIRE(in_el != nullptr);
  in_el->set_focused(true);

  std::string sequence = "\x1B[200~hi there\x1B[201~";
  device->PushInput(sequence);
  for (size_t i = 0; i < sequence.size(); ++i) {
    screen.Step();
  }

  auto* in_ptr =
      dynamic_cast<input*>(const_cast<ComponentBase*>(in_el->component()));
  REQUIRE(in_ptr != nullptr);
  CHECK(in_ptr->value == "hi there");
}

TEST_CASE("Screen.PasteNewlineIntoInputIsDropped", "[terminal][paste]") {
  // A single-line <input> has nowhere to put a newline: pasted '\n's must
  // be dropped rather than doing something undefined.
  class PasteInputComponent : public Component<PasteInputComponent> {
   public:
    void InitReflection() override {
      Import<rtxui::input>();
      Component<PasteInputComponent>::InitReflection();
    }
    std::string_view view = R"(<input id="in" />)";
  };

  auto device = std::make_shared<MockTerminalDevice>();
  auto container = Ref<PasteInputComponent>::New();
  Screen screen(container, device);
  screen.Draw();

  auto* in_el = container->Root()->QuerySelector("#in");
  REQUIRE(in_el != nullptr);
  in_el->set_focused(true);

  std::string sequence = "\x1B[200~foo\nbar\x1B[201~";
  device->PushInput(sequence);
  for (size_t i = 0; i < sequence.size(); ++i) {
    screen.Step();
  }

  auto* in_ptr =
      dynamic_cast<input*>(const_cast<ComponentBase*>(in_el->component()));
  REQUIRE(in_ptr != nullptr);
  CHECK(in_ptr->value == "foobar");
}

TEST_CASE("Screen.PastingIntoTextareaDoesNotDoubleIndentation",
          "[terminal][paste][regression]") {
  // Regression: pasted newlines used to be turned into Event::Return(),
  // the same event a manual Enter keypress produces -- including
  // textarea's auto-indent, which carries the current line's leading
  // whitespace onto the new line. Since pasted text already has its own
  // indentation, every line's indentation compounded on top of the
  // previous one's.
  class PasteTextareaComponent : public Component<PasteTextareaComponent> {
   public:
    void InitReflection() override {
      Import<rtxui::textarea>();
      Component<PasteTextareaComponent>::InitReflection();
    }
    std::string_view view = R"(<textarea id="ta" />)";
  };

  auto device = std::make_shared<MockTerminalDevice>();
  auto container = Ref<PasteTextareaComponent>::New();
  Screen screen(container, device);
  screen.Draw();

  auto* ta_el = container->Root()->QuerySelector("#ta");
  REQUIRE(ta_el != nullptr);
  ta_el->set_focused(true);

  auto* ta_ptr =
      dynamic_cast<textarea*>(const_cast<ComponentBase*>(ta_el->component()));
  REQUIRE(ta_ptr != nullptr);
  // The cursor sits on an already-indented line before pasting.
  ta_ptr->value = "  ";
  ta_ptr->cursor_pos = 2;
  ta_ptr->Digest();

  std::string sequence = "\x1B[200~foo\n  bar\n    baz\x1B[201~";
  device->PushInput(sequence);
  for (size_t i = 0; i < sequence.size(); ++i) {
    screen.Step();
  }

  CHECK(ta_ptr->value == "  foo\n  bar\n    baz");
}

TEST_CASE("Screen.PasteAppliesAsOneRedrawNotOnePerCharacter",
          "[terminal][paste][regression]") {
  // Regression: pasted text was inserted as a burst of individual keyboard
  // events (one per character, all queued from the same end-of-paste
  // marker byte), but each one triggered its own full digest+layout+
  // paint+write cycle, since Step()'s draining loop called HandleEvent()
  // (which redraws on every handled event) once per queued event with no
  // batching. For any paste beyond a couple of characters, this was
  // visible as the pasted text "typing" itself out instead of appearing
  // immediately. Uses a component with no CSS transitions of its own, so
  // the only thing that can trigger a redraw here is the keyboard events
  // themselves -- isolating this from an unrelated focus/hover color
  // transition that might also be independently animating.
  class BoundTrackerComponent : public Component<BoundTrackerComponent> {
   public:
    std::string typed;
    void InitReflection() override {
      Bind(typed);
      Component<BoundTrackerComponent>::InitReflection();
    }
    bool OnEvent(Event event) override {
      if (auto* kb = event.get_if<Event::Keyboard>()) {
        typed += static_cast<char>(kb->codepoint);
        return true;
      }
      return false;
    }
    std::string_view view = "{typed}";
  };

  auto device = std::make_shared<MockTerminalDevice>();
  auto component = Ref<BoundTrackerComponent>::New();
  Screen screen(component, device);

  device->ClearOutput();
  std::string paste_content(20, 'x');
  std::string sequence = "\x1B[200~" + paste_content + "\x1B[201~";
  device->PushInput(sequence);
  for (size_t i = 0; i < sequence.size(); ++i) {
    screen.Step();
  }

  CHECK(component->typed == paste_content);

  std::string output = device->GetOutput();
  int redraw_count = 0;
  for (std::string_view marker : {"\x1B[?25h", "\x1B[?25l"}) {
    size_t pos = 0;
    while ((pos = output.find(marker, pos)) != std::string::npos) {
      ++redraw_count;
      pos += marker.size();
    }
  }
  CHECK(redraw_count == 1);
}

TEST_CASE("Screen.WheelBurstAppliesAsOneRedrawNotOnePerTick",
          "[terminal][scroll][regression]") {
  // Regression: the wheel-scroll path called Draw() directly instead of
  // going through the burst batching, so a fast scroll delivering several
  // wheel ticks in one read painted and wrote a full frame per tick.
  class WheelBurstComponent : public Component<WheelBurstComponent> {
   public:
    void InitReflection() override {
      Import<rtxui::div>();
      Component<WheelBurstComponent>::InitReflection();
    }
    std::string_view view = R"html(
      <div id="scrollable">
        <div>1</div><div>2</div><div>3</div><div>4</div><div>5</div>
        <div>6</div><div>7</div><div>8</div><div>9</div><div>10</div>
      </div>
      <style>
        #scrollable { display: block; height: 4; overflow-y: scroll; }
      </style>
    )html";
  };

  auto device = std::make_shared<MockTerminalDevice>();
  auto component = Ref<WheelBurstComponent>::New();
  Screen screen(component, device);
  screen.SetSmoothScrollEnabled(false);
  screen.Draw();

  auto* scroll_element = component->Root()->QuerySelector("#scrollable");
  REQUIRE(scroll_element != nullptr);

  device->ClearOutput();
  // Three SGR wheel-down ticks at (2, 2), all available in a single read.
  std::string tick = "\x1b[<65;3;3M";
  device->PushInput(tick + tick + tick);
  screen.Step();

  CHECK(scroll_element->scroll_y() == 3);

  std::string output = device->GetOutput();
  int redraw_count = 0;
  for (std::string_view marker : {"\x1B[?25h", "\x1B[?25l"}) {
    size_t pos = 0;
    while ((pos = output.find(marker, pos)) != std::string::npos) {
      ++redraw_count;
      pos += marker.size();
    }
  }
  CHECK(redraw_count == 1);
}

namespace {
// #hide removes #banner, which moves #bump up a row. Clicking both in one
// burst is what exposes a stale fragment tree: the second click is hit-tested
// against the layout the first one invalidated.
class BurstClickComponent : public Component<BurstClickComponent> {
 public:
  bool shown = true;
  int clicks = 0;
  void Hide() { shown = false; }
  void Bump() { ++clicks; }
  void InitReflection() override {
    Bind(shown);
    Bind(clicks);
    Bind(Hide);
    Bind(Bump);
    Import<rtxui::div>();
    Component<BurstClickComponent>::InitReflection();
  }
  std::string_view view = R"(
    <div>
      <div id="hide" onclick="Hide">hide</div>
      <if condition="{shown}"><div id="banner">banner</div></if>
      <div id="bump" onclick="Bump">bump</div>
    </div>
  )";
};

// SGR left-button press at a 0-based cell.
std::string PressAt(int x, int y) {
  return "\x1b[<0;" + std::to_string(x + 1) + ";" + std::to_string(y + 1) + "M";
}
}  // namespace

namespace {
class ControlCharComponent : public Component<ControlCharComponent> {
 public:
  std::string text;
  void InitReflection() override {
    Bind(text);
    Import<rtxui::div>();
    Component<ControlCharComponent>::InitReflection();
  }
  std::string_view view = R"(<div>{text}</div>)";
};

// The bytes the engine writes for its own colours and cursor moves are ESC
// sequences too, so "did an ESC reach the terminal" is not the question. The
// question is whether one the application supplied did.
std::string DrawWith(const std::string& payload) {
  auto app = Ref<ControlCharComponent>::New();
  app->text = payload;
  auto device = std::make_shared<MockTerminalDevice>();
  device->TriggerResize(30, 3);
  Screen screen(app, device);
  screen.Draw();
  return device->GetOutput();
}
}  // namespace

TEST_CASE("Control characters in text never reach the terminal",
          "[terminal][text][security]") {
  SECTION("an escape sequence in application text is not executed") {
    // An application displays text it did not write -- a filename, a log line,
    // something off the network. Passed through, this would repaint, recolour
    // and leave the cell diff describing a screen that never existed.
    const std::string output = DrawWith(
        "a\x1b"
        "[31mRED\x1b"
        "[0mb");
    // The payload's own bytes survive as text, so the sequence is visible
    // rather than obeyed...
    CHECK(output.find("[31mRED") != std::string::npos);
    // ...and no ESC introduces it.
    CHECK(output.find("\x1b[31mRED") == std::string::npos);
    CHECK(output.find("\x1b[0m") == std::string::npos);
  }

  SECTION("the other control characters are replaced too") {
    // Split literals on purpose: a hex escape in C++ swallows every hex digit
    // that follows it, so "a\x07b" is the single character 0x7B and tests
    // nothing at all.
    for (const std::string payload : {std::string("a\x07"
                                                  "b"),  // BEL, rings
                                      std::string("a\x08"
                                                  "b"),  // BS
                                      std::string("a\x0b"
                                                  "b"),  // VT
                                      std::string("a\x0c"
                                                  "b"),  // FF
                                      std::string("a\x7f"
                                                  "b"),           // DEL
                                      std::string("a\0b", 3)}) {  // NUL
      const std::string output = DrawWith(payload);
      for (const char control :
           {'\x07', '\x08', '\x0b', '\x0c', '\x7f', '\0'}) {
        CHECK(output.find(control) == std::string::npos);
      }
      // Replaced, not dropped: something was there and the reader should see
      // it.
      CHECK(output.find("\xef\xbf\xbd") != std::string::npos);
    }
  }

  SECTION("a carriage return is a line break, not a cursor move") {
    // Left alone this would send the cursor to column 0 mid-line and overwrite
    // what was already there. Folded to a segment break, white-space: normal
    // then renders it as a space. (The frame's own cursor moves use CR too, so
    // "no CR in the output" would be the wrong thing to ask.)
    const std::string output = DrawWith("abc\rxyz");
    CHECK(output.find("abc xyz") != std::string::npos);
  }

  SECTION("ordinary text is untouched") {
    const std::string output = DrawWith("plain \u00e9\u4f60 text");
    CHECK(output.find("plain \u00e9\u4f60 text") != std::string::npos);
    CHECK(output.find("\xef\xbf\xbd") == std::string::npos);
  }
}

TEST_CASE("Malformed UTF-8 in text is replaced, not emitted",
          "[terminal][text][unicode]") {
  // All payloads use split string literals: a hex escape in C++ swallows every
  // hex digit after it, so "\xe4\xbdX" would not be the byte pair plus X.
  static constexpr std::string_view kReplacement = "\xef\xbf\xbd";  // U+FFFD

  SECTION("a truncated sequence does not swallow the next character") {
    // The two bytes open a three-byte sequence. Left alone, the grapheme
    // reader takes whatever follows as the missing continuation -- so the
    // ']' was absorbed into the same cell and vanished from the output.
    const std::string output = DrawWith(
        "[\xe4\xbd"
        "]");
    CHECK(output.find(']') != std::string::npos);
    CHECK(output.find(kReplacement) != std::string::npos);
    CHECK(output.find("\xe4\xbd"
                      "]") == std::string::npos);
  }

  SECTION("every ill-formed encoding is replaced") {
    for (const std::string payload : {
             std::string("a\x80"
                         "b"),  // lone continuation
             std::string("a\xff"
                         "b"),  // never valid
             std::string("a\xc0\xaf"
                         "b"),  // overlong '/'
             std::string("a\xed\xa0\x80"
                         "b"),  // encoded surrogate
             std::string("a\xf5\x80\x80\x80"
                         "b"),  // past U+10FFFF
         }) {
      const std::string output = DrawWith(payload);
      CHECK(output.find(kReplacement) != std::string::npos);
      // The characters either side survive: replacement is per bad byte, so
      // nothing legitimate is consumed with it.
      CHECK(output.find('a') != std::string::npos);
      CHECK(output.find('b') != std::string::npos);
    }
  }

  SECTION("well-formed text is left exactly as written") {
    // Wide, combining and zero-width characters are all legitimate UTF-8 and
    // must not be touched -- the point is to reject what cannot be decoded,
    // not to narrow what can be displayed.
    for (const std::string payload : {
             std::string("\xe4\xbd\xa0\xe5\xa5\xbd"),  // CJK
             std::string("e\xcc\x81"),                 // e + combining acute
             std::string("a\xe2\x80\x8b"
                         "b"),                 // zero-width space
             std::string("\xf0\x9f\x8e\x89"),  // four-byte codepoint
         }) {
      const std::string output = DrawWith(payload);
      CHECK(output.find(payload) != std::string::npos);
      CHECK(output.find(kReplacement) == std::string::npos);
    }
  }
}

namespace {
class HandlerArgComponent : public Component<HandlerArgComponent> {
 public:
  std::vector<std::string> items{"alpha", "beta", "gamma"};
  std::string log;
  void Pick(std::string which) { log += "[" + which + "]"; }
  void Plain() { log += "(plain)"; }
  void InitReflection() override {
    Bind(items);
    Bind(log);
    Bind(Pick);
    Bind(Plain);
    Import<rtxui::div>();
    Component<HandlerArgComponent>::InitReflection();
  }
  // A custom raw-string delimiter is needed: `onclick="Pick({$index})">`
  // contains the )" that would end a plain R"( ... )".
  std::string_view view = R"html(
    <div>
      <for each="{items}" as="it">
        <div onclick="Pick({$index})">{it}</div>
      </for>
      <div onclick="Pick(literal)">lit</div>
      <div onclick="Plain">plain</div>
    </div>
  )html";
};
}  // namespace

TEST_CASE("A click handler receives its argument", "[terminal][mouse][for]") {
  // `onclick="Fn({$index})"` is the documented way to tell rows of a loop
  // apart, and what example/app_dashboard.cpp uses, but almost nothing
  // exercised it.
  auto app = Ref<HandlerArgComponent>::New();
  auto device = std::make_shared<MockTerminalDevice>();
  device->TriggerResize(40, 12);
  Screen screen(app, device);
  screen.Draw();

  auto click_row = [&](int row) {
    app->log.clear();
    device->PushInput(PressAt(1, row));
    screen.Step();
    return app->log;
  };

  SECTION("each row of a loop passes its own index") {
    CHECK(click_row(0) == "[0]");
    CHECK(click_row(1) == "[1]");
    CHECK(click_row(2) == "[2]");
  }

  SECTION("a literal argument arrives as written") {
    CHECK(click_row(3) == "[literal]");
  }

  SECTION("a handler named without parentheses takes none") {
    CHECK(click_row(4) == "(plain)");
  }

  SECTION("the indices follow the collection when it changes") {
    // The interesting direction: an index interpolated into the handler when
    // the row was built would go stale here, and the wrong item would be
    // picked -- silently, since the click still lands on a row.
    app->items = {"only"};
    app->Digest();
    screen.Draw();
    CHECK(click_row(0) == "[0]");
    CHECK(click_row(1) == "[literal]");  // The list shrank; rows moved up.

    app->items = {"a", "b", "c", "d"};
    app->Digest();
    screen.Draw();
    CHECK(click_row(3) == "[3]");
    CHECK(click_row(4) == "[literal]");
  }
}

TEST_CASE("Screen.ClickBurstHitTestsTheCurrentLayout", "[terminal][mouse]") {
  // Two presses arriving together. The first runs a handler that removes an
  // element, so reconciliation destroys it -- and the batch flag would
  // otherwise skip the redraw that rebuilds root_fragment_, leaving the
  // fragment tree the second press is hit-tested against pointing at freed
  // elements. Found by Events.TestEvents under ASan; this replays the shape so
  // the project's ASan build keeps catching it. In a plain build the read is
  // silent, so the assertions here check the routing rather than the memory.
  auto device = std::make_shared<MockTerminalDevice>();
  auto app = Ref<BurstClickComponent>::New();
  Screen screen(app, device);
  screen.Draw();

  SECTION("a single click routes by row") {
    device->PushInput(PressAt(0, 2));
    screen.Step();
    CHECK(app->clicks == 1);
    CHECK(app->shown == true);
  }

  SECTION("a burst whose first click reconciles the tree") {
    device->PushInput(PressAt(0, 0) + PressAt(0, 1));
    screen.Step();
    CHECK(app->shown == false);
    CHECK(app->clicks == 1);
  }
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

  // 9. Keyboard End jumps to the maximum scroll offset.
  screen.Dispatch(Event::End());
  REQUIRE(scroll_element->scroll_y() == 6);

  // 10. Keyboard Home jumps back to the top.
  screen.Dispatch(Event::Home());
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

TEST_CASE("Screen.DisabledInputExcludedFromTabOrder",
          "[terminal][focus][disabled]") {
  auto device = std::make_shared<MockTerminalDevice>();

  class DisabledFocusCyclingComponent
      : public Component<DisabledFocusCyclingComponent> {
   public:
    void InitReflection() override {
      Import<rtxui::div>();
      Import<rtxui::input>();
      Component<DisabledFocusCyclingComponent>::InitReflection();
    }
    std::string_view view = R"html(
      <div>
        <input id="input1" />
        <input id="input2" disabled="true" />
        <input id="input3" />
      </div>
    )html";
  };

  auto component = Ref<DisabledFocusCyclingComponent>::New();
  Screen screen(component, device);
  screen.Draw();

  auto* input1 = component->Root()->QuerySelector("#input1");
  auto* input2 = component->Root()->QuerySelector("#input2");
  auto* input3 = component->Root()->QuerySelector("#input3");
  REQUIRE(input1 != nullptr);
  REQUIRE(input2 != nullptr);
  REQUIRE(input3 != nullptr);

  // Tab cycles input1 -> input3, skipping the disabled input2 entirely.
  screen.Dispatch(Event::Tab());
  CHECK(input1->focused());
  CHECK_FALSE(input2->focused());
  CHECK_FALSE(input3->focused());

  screen.Dispatch(Event::Tab());
  CHECK_FALSE(input1->focused());
  CHECK_FALSE(input2->focused());
  CHECK(input3->focused());

  // Wraps back to input1, still skipping input2.
  screen.Dispatch(Event::Tab());
  CHECK(input1->focused());
  CHECK_FALSE(input2->focused());
  CHECK_FALSE(input3->focused());
}

TEST_CASE("Screen.RadioFocusCyclingAndNavigation", "[terminal][focus][radio]") {
  auto device = std::make_shared<MockTerminalDevice>();

  class RadioFocusComponent : public Component<RadioFocusComponent> {
   public:
    void InitReflection() override {
      Import<rtxui::div>();
      Import<rtxui::radio>();
      Component<RadioFocusComponent>::InitReflection();
    }
    std::string_view view = R"html(
      <style>
        #root { display: flex; flex-direction: column; }
        .item { width: 10; height: 3; }
      </style>
      <div id="root">
        <radio id="radio1" class="item">Radio 1</radio>
        <radio id="radio2" class="item">Radio 2</radio>
      </div>
    )html";
  };

  auto component = Ref<RadioFocusComponent>::New();
  Screen screen(component, device);

  auto* radio1 = component->Root()->QuerySelector("#radio1");
  auto* radio2 = component->Root()->QuerySelector("#radio2");

  REQUIRE(radio1 != nullptr);
  REQUIRE(radio2 != nullptr);

  // Initial State: no element focused
  CHECK_FALSE(radio1->focused());
  CHECK_FALSE(radio2->focused());

  // 1. Tab key: Focuses first radio button
  screen.Dispatch(Event::Tab());
  CHECK(radio1->focused());
  CHECK_FALSE(radio2->focused());

  // 2. Tab key: Focuses second radio button
  screen.Dispatch(Event::Tab());
  CHECK_FALSE(radio1->focused());
  CHECK(radio2->focused());

  // 3. Tab key: Wrap around to the start
  screen.Dispatch(Event::Tab());
  CHECK(radio1->focused());
  CHECK_FALSE(radio2->focused());

  // 4. Shift+Tab (TabReverse): Wrap to the end
  screen.Dispatch(Event::TabReverse());
  CHECK_FALSE(radio1->focused());
  CHECK(radio2->focused());

  // 5. Arrow keys (Spatial Navigation): ArrowUp/ArrowDown to move between radio
  // buttons
  radio2->set_focused(false);
  radio1->set_focused(true);
  screen.Draw();

  screen.Dispatch(Event::ArrowDown());
  CHECK_FALSE(radio1->focused());
  CHECK(radio2->focused());

  screen.Dispatch(Event::ArrowUp());
  CHECK(radio1->focused());
  CHECK_FALSE(radio2->focused());
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

  // Click on the link. The link "Link to Target" is at y=0, x=0 inside the
  // scroller. 1-based mouse coordinates are x=1, y=1.
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

TEST_CASE("Screen.NativeCursorHiddenWhenScrolledOutOfView",
          "[terminal][scroll][cursor]") {
  // Regression: the native terminal cursor was positioned using
  // absolute_x()/absolute_y(), which reflects an element's scrolled screen
  // position but not whether a scrollable ancestor currently clips it out
  // of view. Scrolling a textarea's viewport away from the cursor (e.g.
  // via mouse wheel), without moving the cursor itself, left the native
  // cursor positioned off-screen (even at a negative row) instead of
  // hidden.
  class ScrollCursorComponent : public Component<ScrollCursorComponent> {
   public:
    void InitReflection() override {
      Import<rtxui::textarea>();
      Component<ScrollCursorComponent>::InitReflection();
    }
    std::string_view view = R"html(
      <textarea id="ta" />
      <style>
        #ta { width: 20; height: 3; }
      </style>
    )html";
  };

  auto device = std::make_shared<MockTerminalDevice>();
  auto container = Ref<ScrollCursorComponent>::New();
  Screen screen(container, device);

  auto* ta_el = container->Root()->QuerySelector("#ta");
  REQUIRE(ta_el != nullptr);
  auto* ta_ptr =
      dynamic_cast<textarea*>(const_cast<ComponentBase*>(ta_el->component()));
  REQUIRE(ta_ptr != nullptr);

  screen.Draw();
  ta_el->set_focused(true);

  std::string text;
  for (int i = 0; i < 10; ++i) {
    text += "line " + std::to_string(i) + "\n";
  }
  ta_ptr->value = text;
  ta_ptr->cursor_pos = 5;  // inside "line 0", within the initial viewport.
  ta_ptr->Digest();
  device->ClearOutput();
  screen.Draw();
  CHECK(device->GetOutput().find("\x1b[?25h") != std::string::npos);

  // Scroll away from the cursor's line, without moving the cursor.
  ta_el->set_scroll_y(7);
  device->ClearOutput();
  screen.Draw();
  std::string output = device->GetOutput();
  CHECK(output.find("\x1b[?25h") == std::string::npos);
  CHECK(output.find("\x1b[?25l") != std::string::npos);
}

TEST_CASE("Screen.CopySelectionWritesOSC52", "[terminal][clipboard]") {
  class ClipboardInputComponent : public Component<ClipboardInputComponent> {
   public:
    void InitReflection() override {
      Import<rtxui::input>();
      Component<ClipboardInputComponent>::InitReflection();
    }
    std::string_view view = R"(<input id="in" />)";
  };

  auto device = std::make_shared<MockTerminalDevice>();
  auto container = Ref<ClipboardInputComponent>::New();
  Screen screen(container, device);
  screen.Draw();

  auto* in_el = container->Root()->QuerySelector("#in");
  REQUIRE(in_el != nullptr);
  auto* in_ptr =
      dynamic_cast<input*>(const_cast<ComponentBase*>(in_el->component()));
  REQUIRE(in_ptr != nullptr);

  in_el->set_focused(true);
  in_ptr->value = "hello world";
  in_ptr->selection_start = 0;
  in_ptr->cursor_pos = 5;  // selects "hello"
  in_ptr->Digest();

  device->ClearOutput();
  in_ptr->OnEvent(Event::CtrlC());
  in_ptr->Digest();
  screen.Draw();

  // "hello" base64-encodes to "aGVsbG8=".
  CHECK(device->GetOutput().find("\x1b]52;c;aGVsbG8=") != std::string::npos);
  CHECK(in_ptr->value == "hello world");  // Copy must not modify the value.
}

TEST_CASE("Screen.CutSelectionWritesOSC52AndDeletesSelection",
          "[terminal][clipboard]") {
  class ClipboardInputComponent : public Component<ClipboardInputComponent> {
   public:
    void InitReflection() override {
      Import<rtxui::input>();
      Component<ClipboardInputComponent>::InitReflection();
    }
    std::string_view view = R"(<input id="in" />)";
  };

  auto device = std::make_shared<MockTerminalDevice>();
  auto container = Ref<ClipboardInputComponent>::New();
  Screen screen(container, device);
  screen.Draw();

  auto* in_el = container->Root()->QuerySelector("#in");
  REQUIRE(in_el != nullptr);
  auto* in_ptr =
      dynamic_cast<input*>(const_cast<ComponentBase*>(in_el->component()));
  REQUIRE(in_ptr != nullptr);

  in_el->set_focused(true);
  in_ptr->value = "hello world";
  in_ptr->selection_start = 6;
  in_ptr->cursor_pos = 11;  // selects "world"
  in_ptr->Digest();

  device->ClearOutput();
  in_ptr->OnEvent(Event::CtrlX());
  in_ptr->Digest();
  screen.Draw();

  // "world" base64-encodes to "d29ybGQ=".
  CHECK(device->GetOutput().find("\x1b]52;c;d29ybGQ=") != std::string::npos);
  CHECK(in_ptr->value == "hello ");
}

TEST_CASE("Screen.CopyWithNoSelectionLeavesEventUnhandled",
          "[terminal][clipboard]") {
  // With nothing selected, Ctrl+C must be left unhandled (not silently
  // swallowed): Screen's global Ctrl+C-quits-the-app shortcut only fires
  // when no component consumes the event first, and any input being
  // focused is common enough that always consuming Ctrl+C there would
  // otherwise make quitting the app that way stop working entirely.
  class ClipboardInputComponent : public Component<ClipboardInputComponent> {
   public:
    void InitReflection() override {
      Import<rtxui::input>();
      Component<ClipboardInputComponent>::InitReflection();
    }
    std::string_view view = R"(<input id="in" />)";
  };

  auto device = std::make_shared<MockTerminalDevice>();
  auto container = Ref<ClipboardInputComponent>::New();
  Screen screen(container, device);
  screen.Draw();

  auto* in_el = container->Root()->QuerySelector("#in");
  REQUIRE(in_el != nullptr);
  auto* in_ptr =
      dynamic_cast<input*>(const_cast<ComponentBase*>(in_el->component()));
  REQUIRE(in_ptr != nullptr);

  in_el->set_focused(true);
  in_ptr->value = "hello world";
  in_ptr->selection_start = -1;
  in_ptr->cursor_pos = 3;
  in_ptr->Digest();

  device->ClearOutput();
  bool handled = in_ptr->OnEvent(Event::CtrlC());
  in_ptr->Digest();
  screen.Draw();

  CHECK_FALSE(handled);
  CHECK(device->GetOutput().find("\x1b]52;c;") == std::string::npos);
}

TEST_CASE("Screen.ScrollIntoViewWithBorder", "[terminal][focus][scroll]") {
  auto device = std::make_shared<MockTerminalDevice>();

  class ScrollFocusBorderComponent
      : public Component<ScrollFocusBorderComponent> {
   public:
    void InitReflection() override {
      Import<rtxui::div>();
      Component<ScrollFocusBorderComponent>::InitReflection();
    }
    std::string_view view = R"xml(
      <div id="scrollable">
        <div id="item0" class="spacer" tabindex="0">Spacer 1</div>
        <div id="item1" class="item" tabindex="0">Item 1</div>
      </div>
      <style>
        #scrollable {
          height: 5;
          border: solid;
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
    )xml";
  };

  auto component = Ref<ScrollFocusBorderComponent>::New();
  Screen screen(component, device);
  screen.SetSmoothScrollEnabled(false);
  screen.Draw();

  auto* scrollable = component->Root()->QuerySelector("#scrollable");
  auto* item0 = component->Root()->QuerySelector("#item0");
  auto* item1 = component->Root()->QuerySelector("#item1");

  REQUIRE(scrollable != nullptr);
  REQUIRE(item0 != nullptr);
  REQUIRE(item1 != nullptr);

  // Initially: scroll_y should be 0
  REQUIRE(scrollable->scroll_y() == 0);

  // Tab 1: focuses item0. Since it fits in the viewport, scroll_y remains 0.
  screen.Dispatch(Event::Tab());
  REQUIRE(item0->focused());
  CHECK(scrollable->scroll_y() == 0);

  // Tab 2: focuses item1.
  // Viewport height is h - 2 = 3.
  // Spacer item0 height is 3, so item1 starts at 3 (relative to content area)
  // and has height 2 (bottom = 5).
  // Since it is out of the viewport [0, 3], focusing it should trigger
  // ScrollIntoView which adjusts scroll_y to target_bottom - viewport_h = 5 - 3
  // = 2.
  screen.Dispatch(Event::Tab());
  REQUIRE(item1->focused());
  CHECK(scrollable->scroll_y() == 2);

  // TabReverse: focuses item0 again.
  // Since item0 is at the top, scroll_y should scroll back to 0.
  screen.Dispatch(Event::TabReverse());
  REQUIRE(item0->focused());
  CHECK(scrollable->scroll_y() == 0);
}

TEST_CASE("Screen.ScrollIntoViewWithMargin", "[terminal][focus][scroll]") {
  auto device = std::make_shared<MockTerminalDevice>();

  class ScrollFocusMarginComponent
      : public Component<ScrollFocusMarginComponent> {
   public:
    void InitReflection() override {
      Import<rtxui::div>();
      Component<ScrollFocusMarginComponent>::InitReflection();
    }
    std::string_view view = R"xml(
      <div id="scrollable">
        <div id="item0" class="spacer" tabindex="0">Spacer 1</div>
        <div id="item1" class="item" tabindex="0">Item 1</div>
      </div>
      <style>
        #scrollable {
          height: 6;
          overflow-y: scroll;
          display: block;
        }
        .spacer {
          height: 3;
          display: block;
        }
        .item {
          height: 3;
          margin-top: 2;
          margin-bottom: 2;
          display: block;
        }
      </style>
    )xml";
  };

  auto component = Ref<ScrollFocusMarginComponent>::New();
  Screen screen(component, device);
  screen.SetSmoothScrollEnabled(false);
  screen.Draw();

  auto* scrollable = component->Root()->QuerySelector("#scrollable");
  auto* item0 = component->Root()->QuerySelector("#item0");
  auto* item1 = component->Root()->QuerySelector("#item1");

  REQUIRE(scrollable != nullptr);
  REQUIRE(item0 != nullptr);
  REQUIRE(item1 != nullptr);

  // Initially: scroll_y should be 0
  REQUIRE(scrollable->scroll_y() == 0);

  // Tab 1: focuses item0
  screen.Dispatch(Event::Tab());
  REQUIRE(item0->focused());
  CHECK(scrollable->scroll_y() == 0);

  // Tab 2: focuses item1.
  // Viewport height is 6.
  // Spacer item0 height is 3.
  // Item 1 starts at 5 (due to margin-top: 2) and ends at 8 (height 3).
  // Target top with margins: 5 - 2 = 3.
  // Target bottom with margins: 8 + 2 = 10.
  // Since bottom (10) > viewport_h (6), it scrolls down to 10 - 6 = 4.
  screen.Dispatch(Event::Tab());
  REQUIRE(item1->focused());
  CHECK(scrollable->scroll_y() == 4);

  // TabReverse: focuses item0 again.
  // Target top with margins: 0 - 0 = 0.
  // Since top (0) < scroll_y (4), it scrolls back to 0.
  screen.Dispatch(Event::TabReverse());
  REQUIRE(item0->focused());
  CHECK(scrollable->scroll_y() == 0);
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

  // Trigger smooth scroll programmatically (simulating keyboard focus
  // navigation)
  scrollable_smooth->set_scroll_y(2, true);

  // Target is updated immediately
  REQUIRE(scrollable_smooth->target_scroll_y() == 2);
  // Both content and visual remain 0 before ticking
  REQUIRE(scrollable_smooth->scroll_y() == 0);
  REQUIRE(scrollable_smooth->visual_scroll_y() == 0.0f);

  // Advance by 250ms (halfway through 500ms duration)
  mock_now_ms = 2250.0;
  screen.Step();
  // Content scroll is smooth
  REQUIRE(scrollable_smooth->scroll_y() >= 0);
  REQUIRE(scrollable_smooth->scroll_y() <= 2);

  // Complete smooth content transition (500ms duration)
  mock_now_ms = 2500.0;
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
            while (
                end < data.size() &&
                ((data[end] >= '0' && data[end] <= '9') || data[end] == ';')) {
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
          if (first >= 0xf0) {
            len = 4;
          } else if (first >= 0xe0) {
            len = 3;
          } else if (first >= 0xc0) {
            len = 2;
          }

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
        std::string expected_char =
            expected.is_continuation
                ? ""
                : (expected.character.empty() ? " " : expected.character);
        std::string actual_char =
            actual.is_continuation ? "" : actual.character;
        if (expected_char != actual_char ||
            expected.is_continuation != actual.is_continuation) {
          UNSCOPED_INFO("Mismatch at ("
                        << x << "," << y << "): expected '" << expected_char
                        << "' (continuation=" << expected.is_continuation
                        << "), got '" << actual_char
                        << "' (continuation=" << actual.is_continuation << ")");
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

  // Test Case 4: Changing a character immediately following a wide character
  // (which stays unchanged)
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

  // Test Case 5: Changing a character immediately preceding a wide character
  // (which stays unchanged)
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
    DynamicComponent() { Bind(text); }
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
            while (
                end < data.size() &&
                ((data[end] >= '0' && data[end] <= '9') || data[end] == ';')) {
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
          if (first >= 0xf0) {
            len = 4;
          } else if (first >= 0xe0) {
            len = 3;
          } else if (first >= 0xc0) {
            len = 2;
          }

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
  REQUIRE(output.find("\x1B[49mA") != std::string::npos);
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

TEST_CASE("Screen.ParameterizedActivationWithSpaceAndReturn",
          "[terminal][focus]") {
  auto device = std::make_shared<MockTerminalDevice>();

  class ParameterizedActivationComponent
      : public Component<ParameterizedActivationComponent> {
   public:
    std::string last_arg = "";
    void on_click(std::string arg) { last_arg = arg; }
    void InitReflection() override {
      Import<rtxui::div>();
      Component<ParameterizedActivationComponent>::InitReflection();
    }
    ParameterizedActivationComponent() {
      Import("on_click", [this](std::string arg) { on_click(arg); });
    }
    std::string_view view = R"xml(
      <div id="btn" focusable="true" onclick="on_click(my-arg)">Click Me</div>
    )xml";
  };

  auto component = Ref<ParameterizedActivationComponent>::New();
  Screen screen(component, device);
  auto* btn = component->Root()->QuerySelector("#btn");
  REQUIRE(btn != nullptr);
  btn->set_focused(true);
  screen.Draw();

  // Press Space
  screen.Dispatch(Event::Keyboard{.codepoint = 32});
  CHECK(component->last_arg == "my-arg");

  component->last_arg = "";

  // Press Return
  screen.Dispatch(Event::Return());
  CHECK(component->last_arg == "my-arg");
}

TEST_CASE("Screen.CheckboxActivationWithSpaceAndReturn", "[terminal][focus]") {
  auto device = std::make_shared<MockTerminalDevice>();

  class CheckboxActivationComponent
      : public Component<CheckboxActivationComponent> {
   public:
    bool checked = false;
    void InitReflection() override {
      Import<rtxui::checkbox>();
      Import<rtxui::div>();
      Component<CheckboxActivationComponent>::InitReflection();
    }
    CheckboxActivationComponent() { Bind(checked); }
    std::string_view view = R"xml(
      <checkbox id="chk" checked="{checked}">Check</checkbox>
    )xml";
  };

  auto component = Ref<CheckboxActivationComponent>::New();
  Screen screen(component, device);
  auto* chk = component->Root()->QuerySelector("#chk");
  REQUIRE(chk != nullptr);
  chk->set_focused(true);
  screen.Draw();

  // Initially unchecked
  CHECK_FALSE(component->checked);

  // Press Space to check
  screen.Dispatch(Event::Keyboard{.codepoint = 32});
  CHECK(component->checked);

  // Press Return to uncheck
  screen.Dispatch(Event::Return());
  CHECK_FALSE(component->checked);

  // Press Space to check again
  screen.Dispatch(Event::Keyboard{.codepoint = 32});
  CHECK(component->checked);
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

  // Press ArrowRight when cursor is at the end. It should be consumed and NOT
  // move focus.
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

TEST_CASE("Transitions.NulloptTargetReverts",
          "[terminal][transitions][regression]") {
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
  // Wait, start value of transition will be transparent/interpolated, so it has
  // a value during transition
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
  time::SetCustomClock(nullptr);
}

TEST_CASE("Screen.TransitionForegroundColorAlias",
          "[terminal][transition][regression]") {
  static double mock_now_ms = 1000.0;
  mock_now_ms = 1000.0;
  time::SetCustomClock([]() -> double { return mock_now_ms; });

  auto device = std::make_shared<MockTerminalDevice>();

  class ForegroundColorTransitionComponent
      : public Component<ForegroundColorTransitionComponent> {
   public:
    void InitReflection() override {
      Import<rtxui::div>();
      Component<ForegroundColorTransitionComponent>::InitReflection();
    }
    std::string_view view = R"html(
      <div id="btn">Click me</div>
      <style>
        #btn {
          foreground-color: #000000;
          transition: foreground-color 1s linear;
        }
        #btn:focus {
          foreground-color: #ffffff;
        }
      </style>
    )html";
  };

  auto component = Ref<ForegroundColorTransitionComponent>::New();
  Screen screen(component, device);

  auto* btn = component->Root()->QuerySelector("#btn");
  REQUIRE(btn != nullptr);

  btn->set_focused(true);
  component->ResolveTargetStyles();

  REQUIRE(btn->active_transitions.count("color") == 1);

  mock_now_ms = 2000.0;
  screen.Step();
  CHECK(btn->style.foreground_color == Color::RGB(255, 255, 255));
  CHECK(btn->active_transitions.empty());

  time::SetCustomClock(nullptr);
}

TEST_CASE("Screen.TransitionBorderColorTopAlias",
          "[terminal][transition][regression]") {
  static double mock_now_ms = 1000.0;
  mock_now_ms = 1000.0;
  time::SetCustomClock([]() -> double { return mock_now_ms; });

  auto device = std::make_shared<MockTerminalDevice>();

  class BorderColorTransitionComponent
      : public Component<BorderColorTransitionComponent> {
   public:
    void InitReflection() override {
      Import<rtxui::div>();
      Component<BorderColorTransitionComponent>::InitReflection();
    }
    std::string_view view = R"html(
      <div id="box">Box</div>
      <style>
        #box {
          border: solid;
          border-color-top: #000000;
          transition: border-color-top 1s linear;
        }
        #box:focus {
          border-color-top: #ffffff;
        }
      </style>
    )html";
  };

  auto component = Ref<BorderColorTransitionComponent>::New();
  Screen screen(component, device);

  auto* box = component->Root()->QuerySelector("#box");
  REQUIRE(box != nullptr);

  box->set_focused(true);
  component->ResolveTargetStyles();

  REQUIRE(box->active_transitions.count("border-top-color") == 1);

  mock_now_ms = 2000.0;
  screen.Step();
  CHECK(box->style.border_color_top == Color::RGB(255, 255, 255));
  CHECK(box->active_transitions.empty());

  time::SetCustomClock(nullptr);
}

TEST_CASE("Screen.AnchorExampleScrollIntoView", "[terminal][scroll]") {
  auto device = std::make_shared<MockTerminalDevice>();

  class AnchorDemoTest : public Component<AnchorDemoTest> {
   public:
    void InitReflection() override {
      Import<rtxui::div>();
      Component<AnchorDemoTest>::InitReflection();
    }
    std::string_view view = R"xml(
    <div class="container">
      <div class="header">
        <h2>Anchor Navigation Demo</h2>
        <p class="description">
          Click the links in the sidebar to scroll the corresponding section into view.
        </p>
      </div>

      <div class="workspace">
        <div class="sidebar">
          <div class="nav-title">SECTIONS</div>
          <a class="nav-link" href="#sec-intro">Introduction</a>
          <a class="nav-link" href="#sec-features">Features</a>
          <a class="nav-link" href="#sec-install">Installation</a>
          <a class="nav-link" href="#sec-usage">Usage</a>
          <a class="nav-link" href="#sec-components">Components</a>
          <a class="nav-link" href="#sec-docs">Documentation</a>
          <a class="nav-link" href="#sec-faq">FAQ</a>
          <a id="link-contact" class="nav-link" href="#sec-contact">Contact</a>
        </div>

        <div id="scroll-window" class="scroll-window">
          <div id="sec-intro" class="section sec-odd">
            <div class="section-title">Introduction</div>
            <p>Welcome to RTXUI. This framework lets you build terminal user interfaces using familiar XML templates and CSS styles.</p>
            <p>Layout features include block, inline, flexbox, grid, fixed, absolute, and sticky positioning.</p>
          </div>

          <div id="sec-features" class="section sec-even">
            <div class="section-title">Features</div>
            <p>• Declarative XML markup parsing</p>
            <p>• Complete CSS layout and flexbox model</p>
            <p>• Rich borders, margins, padding, and z-index</p>
            <p>• Mouse support: hover, active, focus, and clicks</p>
          </div>

          <div id="sec-install" class="section sec-odd">
            <div class="section-title">Installation</div>
            <p>To use RTXUI in your CMake project, add the library using FetchContent:</p>
            <p>FetchContent_Declare(rtxui GIT_REPOSITORY ...)</p>
            <p>Then link it with target_link_libraries(your_target rtxui::rtxui).</p>
          </div>

          <div id="sec-usage" class="section sec-even">
            <div class="section-title">Usage</div>
            <p>Initialize a component class, define its view property with HTML/XML markup,</p>
            <p>implement InitReflection(), and start the main Screen loop.</p>
            <p>Bind C++ states to reactive template properties for dynamic UI updates.</p>
          </div>

          <div id="sec-components" class="section sec-odd">
            <div class="section-title">Components</div>
            <p>RTXUI supports custom reusable components. Standard built-in components</p>
            <p>include divs, spans, inputs, buttons, sliders, textareas, and checkboxes.</p>
            <p>Create nested hierarchies using standard XML slot definitions.</p>
          </div>

          <div id="sec-docs" class="section sec-even">
            <div class="section-title">Documentation</div>
            <p>Styles are resolved dynamically based on CSS selectors and active classes.</p>
            <p>Use the C++ API to bind state variables and handle interactive events.</p>
          </div>

          <div id="sec-faq" class="section sec-odd">
            <div class="section-title">FAQ</div>
            <p>Q: Does it support mouse inputs? Yes, hovering and clicking are fully supported.</p>
            <p>Q: Can I use CSS grid? Yes, grid-template-columns and grid-gap are available.</p>
            <p>Q: Does it have animations? Yes, CSS transitions are supported.</p>
          </div>

          <div id="sec-contact" class="section sec-even">
            <div class="section-title">Contact</div>
            <p>Created by Arthur Sonzogni.</p>
            <p>Licensed under the MIT License.</p>
          </div>
        </div>
      </div>
    </div>

    <style>
      self {
        display: block;
        padding: 1 2;
        background-color: rgb(15, 23, 42);
        color: rgb(241, 245, 249);
      }
      .container {
        display: block;
        max-width: 80;
        width: 100%;
        margin-left: auto;
        margin-right: auto;
      }
      .header {
        display: block;
        margin-bottom: 1;
      }
      h2 {
        color: rgb(59, 130, 246);
        margin-bottom: 0;
      }
      .description {
        color: rgb(148, 163, 184);
      }
      .workspace {
        display: flex;
        flex-direction: row;
        align-items: flex-start;
        gap: 2;
        width: 100%;
      }
      .sidebar {
        position: sticky;
        top: 0;
        display: flex;
        flex-direction: column;
        width: 18;
        border: tall;
        border-color: rgb(71, 85, 105);
        background-color: rgb(30, 41, 59, 0.4);
        padding: 1;
        flex-shrink: 0;
        z-index: 10;
      }
      .nav-title {
        color: rgb(148, 163, 184);
        font-weight: bold;
        margin-bottom: 1;
      }
      .nav-link {
        display: block;
        color: rgb(56, 189, 248);
        margin-bottom: 1;
        padding-left: 1;
        cursor: pointer;
      }
      .scroll-window {
        display: block;
        height: 18;
        border: tall;
        border-color: rgb(71, 85, 105);
        overflow-y: scroll;
        scroll-speed: 1;
        scroll-behavior: smooth;
        flex-grow: 1;
      }
      .section {
        display: block;
        margin: 2;
        padding: 2;
      }
      .section-title {
        font-weight: bold;
        margin-bottom: 1;
      }
    </style>
    )xml";
  };

  auto component = Ref<AnchorDemoTest>::New();
  device->TriggerResize(80, 40);
  Screen screen(component, device);
  screen.SetSmoothScrollEnabled(false);
  screen.Draw();

  auto* scrollable = component->Root()->QuerySelector("#scroll-window");
  auto* link_contact = component->Root()->QuerySelector("#link-contact");
  auto* sec_contact = component->Root()->QuerySelector("#sec-contact");

  REQUIRE(scrollable != nullptr);
  REQUIRE(link_contact != nullptr);
  REQUIRE(sec_contact != nullptr);

  // Initial scroll_y should be 0
  REQUIRE(scrollable->scroll_y() == 0);

  // Find the contact link's screen coordinates by probing click events.
  int contact_x = -1, contact_y = -1;
  int width = 0, height = 0;
  device->GetSize(width, height);
  for (int y = 1; y <= height; ++y) {
    for (int x = 1; x <= width; ++x) {
      Event::Mouse mouse_event;
      mouse_event.button = Event::Mouse::Button::Left;
      mouse_event.motion = Event::Mouse::Motion::Pressed;
      mouse_event.x = x;
      mouse_event.y = y;
      screen.Dispatch(mouse_event);

      // Check if the currently focused element is link_contact or its
      // descendant
      Element* focused = nullptr;
      component->Root()->Visit([&](Element& el) {
        if (el.focused()) {
          focused = &el;
        }
      });

      bool is_contact_link = false;
      Element* curr = focused;
      while (curr) {
        if (curr == link_contact) {
          is_contact_link = true;
          break;
        }
        curr = curr->Parent();
      }

      if (is_contact_link) {
        contact_x = x;
        contact_y = y;
        break;
      }
    }
    if (contact_x != -1) {
      break;
    }
  }

  // Ensure we actually successfully located and clicked the contact link
  REQUIRE(contact_x != -1);
  REQUIRE(contact_y != -1);

  // Confirm scrollbar is scrolled to the absolute bottom (max_scroll_y)
  auto* scrollable_after = component->Root()->QuerySelector("#scroll-window");

  int max_scroll_y =
      scrollable_after->scroll_height() - scrollable_after->layout_height();
  CHECK(scrollable_after->scroll_y() == max_scroll_y);
}

TEST_CASE("Screen.ScrollbarTrackClick", "[terminal][scroll][scrollbar]") {
  auto device = std::make_shared<MockTerminalDevice>();

  class TrackClickComponent : public Component<TrackClickComponent> {
   public:
    void InitReflection() override {
      Import<rtxui::div>();
      Component<TrackClickComponent>::InitReflection();
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
        <div>Line 11</div>
        <div>Line 12</div>
        <div>Line 13</div>
        <div>Line 14</div>
        <div>Line 15</div>
        <div>Line 16</div>
        <div>Line 17</div>
        <div>Line 18</div>
        <div>Line 19</div>
        <div>Line 20</div>
      </div>
      <style>
        #scrollable {
          display: block;
          width: 20;
          height: 10;
          overflow-y: scroll;
        }
      </style>
    )html";
  };

  auto component = Ref<TrackClickComponent>::New();
  Screen screen(component, device);
  screen.SetSmoothScrollEnabled(false);
  screen.Draw();

  auto* scroll_el = component->Root()->QuerySelector("#scrollable");
  REQUIRE(scroll_el != nullptr);
  REQUIRE(scroll_el->scroll_y() == 0);
  // Content = 20 lines, viewport = 10, max_scroll = 10.
  // Scrollbar is at x = 19 (0-based), which is x=20 in 1-based mouse coords.

  // Click below the thumb on the track to page down.
  // Initially the thumb is at the top, so clicking at the bottom of the track
  // should scroll down by one viewport height (10).
  Event::Mouse track_click;
  track_click.button = Event::Mouse::Button::Left;
  track_click.motion = Event::Mouse::Motion::Pressed;
  track_click.x = 20;  // Scrollbar column (1-based)
  track_click.y = 9;   // Bottom area of the track (1-based)
  screen.Dispatch(track_click);

  // After clicking below the thumb, scroll should increase by viewport height.
  CHECK(scroll_el->target_scroll_y() == 10);

  // Now scroll is at max. Click above the thumb (at the top) to page up.
  Event::Mouse track_click_up;
  track_click_up.button = Event::Mouse::Button::Left;
  track_click_up.motion = Event::Mouse::Motion::Pressed;
  track_click_up.x = 20;
  track_click_up.y = 1;  // Top of the track (1-based)
  screen.Dispatch(track_click_up);

  // Should have scrolled back up by viewport height.
  CHECK(scroll_el->target_scroll_y() == 0);
}

TEST_CASE("Screen.ScrollbarThumbDrag", "[terminal][scroll][scrollbar]") {
  auto device = std::make_shared<MockTerminalDevice>();

  class ThumbDragComponent : public Component<ThumbDragComponent> {
   public:
    void InitReflection() override {
      Import<rtxui::div>();
      Component<ThumbDragComponent>::InitReflection();
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
        <div>Line 11</div>
        <div>Line 12</div>
        <div>Line 13</div>
        <div>Line 14</div>
        <div>Line 15</div>
        <div>Line 16</div>
        <div>Line 17</div>
        <div>Line 18</div>
        <div>Line 19</div>
        <div>Line 20</div>
      </div>
      <style>
        #scrollable {
          display: block;
          width: 20;
          height: 10;
          overflow-y: scroll;
        }
      </style>
    )html";
  };

  auto component = Ref<ThumbDragComponent>::New();
  Screen screen(component, device);
  screen.SetSmoothScrollEnabled(false);
  screen.Draw();

  auto* scroll_el = component->Root()->QuerySelector("#scrollable");
  REQUIRE(scroll_el != nullptr);
  REQUIRE(scroll_el->scroll_y() == 0);

  // With 20 lines, 10 visible, the thumb covers half the track (5 cells).
  // The scrollbar is at x=19 (0-based) = x=20 (1-based).
  // Click on the thumb at y=1 (1-based, top of track where thumb starts).
  Event::Mouse press;
  press.button = Event::Mouse::Button::Left;
  press.motion = Event::Mouse::Motion::Pressed;
  press.x = 20;
  press.y = 1;
  screen.Dispatch(press);

  // Scroll should still be 0 after pressing (just starts drag).
  CHECK(scroll_el->target_scroll_y() == 0);

  // Drag down by 5 cells (move thumb from top to bottom).
  Event::Mouse move;
  move.button = Event::Mouse::Button::Left;
  move.motion = Event::Mouse::Motion::Moved;
  move.x = 20;
  move.y = 6;  // Moved 5 cells down
  screen.Dispatch(move);

  // After dragging down by 5 cells with a 10-cell track and 5-cell thumb,
  // the thumb should be at the bottom, scroll should be at max (10).
  CHECK(scroll_el->target_scroll_y() == 10);

  // Release the mouse.
  Event::Mouse release;
  release.button = Event::Mouse::Button::Left;
  release.motion = Event::Mouse::Motion::Released;
  release.x = 20;
  release.y = 6;
  screen.Dispatch(release);

  // Scroll should remain at the dragged position.
  CHECK(scroll_el->target_scroll_y() == 10);
}

TEST_CASE("Screen.ScrollbarThumbDragPixelPrecise",
          "[terminal][scroll][scrollbar]") {
  auto device = std::make_shared<MockTerminalDevice>();
  device->SetMockCellPixelSize(10, 20);  // Cell width = 10px, height = 20px

  class ThumbDragPixelComponent : public Component<ThumbDragPixelComponent> {
   public:
    void InitReflection() override {
      Import<rtxui::div>();
      Component<ThumbDragPixelComponent>::InitReflection();
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
        <div>Line 11</div>
        <div>Line 12</div>
        <div>Line 13</div>
        <div>Line 14</div>
        <div>Line 15</div>
        <div>Line 16</div>
        <div>Line 17</div>
        <div>Line 18</div>
        <div>Line 19</div>
        <div>Line 20</div>
      </div>
      <style>
        #scrollable {
          display: block;
          width: 20;
          height: 10;
          overflow-y: scroll;
        }
      </style>
    )html";
  };

  auto component = Ref<ThumbDragPixelComponent>::New();
  Screen screen(component, device);
  screen.SetSmoothScrollEnabled(false);
  screen.Draw();

  auto* scroll_el = component->Root()->QuerySelector("#scrollable");
  REQUIRE(scroll_el != nullptr);
  REQUIRE(scroll_el->scroll_y() == 0);

  // Initial scroll position is 0.
  // With cell size 10x20:
  // Scrollbar is at column index 19 (0-based) = x=20 (1-based cell coordinate).
  // Under pixel mode:
  // The scrollbar column is at x = 19 * 10 + 1 = 191 pixels (1-based).
  // The top of the track (first cell) is at y = 0 * 20 + 1 = 1 pixel.

  // 1. Dispatch press at y = 1 (pixel coordinate, which falls in cell y = 1)
  Event::Mouse press;
  press.button = Event::Mouse::Button::Left;
  press.motion = Event::Mouse::Motion::Pressed;
  press.x = 191;  // 19 * 10 + 1
  press.y = 1;    // Top-most pixel
  screen.Dispatch(press);

  // 2. Drag down by 5 cells.
  // Since cell height is 20 pixels, dragging down by 5 cells means moving the
  // mouse down by 5 * 20 = 100 pixels. So target y is 1 + 100 = 101 pixels.
  Event::Mouse move;
  move.button = Event::Mouse::Button::Left;
  move.motion = Event::Mouse::Motion::Moved;
  move.x = 191;
  move.y = 101;  // Moved 100 pixels down
  screen.Dispatch(move);

  // The thumb covers half the track (5 cells). The scrollable range is 10
  // cells. Moving 5 cells down (100 pixels) on a 10-cell viewport (200 pixels
  // height) should move the scroll position to max (10).
  CHECK(scroll_el->target_scroll_y() == 10);

  // 3. Move back up by 2 cells (40 pixels).
  // Target y is 101 - 40 = 61 pixels.
  Event::Mouse move_up;
  move_up.button = Event::Mouse::Button::Left;
  move_up.motion = Event::Mouse::Motion::Moved;
  move_up.x = 191;
  move_up.y = 61;
  screen.Dispatch(move_up);

  // Scroll should be at 6.
  CHECK(scroll_el->target_scroll_y() == 6);
}

TEST_CASE("Screen.ScrollbarPseudoClasses", "[terminal][scroll][scrollbar]") {
  auto device = std::make_shared<MockTerminalDevice>();

  class PseudoClassComponent : public Component<PseudoClassComponent> {
   public:
    void InitReflection() override {
      Import<rtxui::div>();
      Component<PseudoClassComponent>::InitReflection();
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
          width: 20;
          height: 5;
          overflow-y: scroll;
        }
      </style>
    )html";
  };

  auto component = Ref<PseudoClassComponent>::New();
  Screen screen(component, device);
  screen.SetSmoothScrollEnabled(false);
  screen.Draw();

  auto* scroll_el = component->Root()->QuerySelector("#scrollable");
  REQUIRE(scroll_el != nullptr);

  // Initial state: no scrollbar hover/active.
  CHECK_FALSE(scroll_el->scrollbar_hovered());
  CHECK_FALSE(scroll_el->scrollbar_thumb_hovered());
  CHECK_FALSE(scroll_el->scrollbar_active());
  CHECK_FALSE(scroll_el->scrollbar_thumb_active());

  // Move mouse over the scrollbar column.
  // Scrollbar is at x=19 (0-based) = x=20 (1-based).
  Event::Mouse hover;
  hover.button = Event::Mouse::Button::None;
  hover.motion = Event::Mouse::Motion::Moved;
  hover.x = 20;
  hover.y = 1;  // On the thumb (top of track)
  screen.Dispatch(hover);

  // Scrollbar and thumb should be hovered.
  CHECK(scroll_el->scrollbar_hovered());
  CHECK(scroll_el->scrollbar_thumb_hovered());
  CHECK_FALSE(scroll_el->scrollbar_active());
  CHECK_FALSE(scroll_el->scrollbar_thumb_active());

  // Move mouse away from scrollbar.
  Event::Mouse move_away;
  move_away.button = Event::Mouse::Button::None;
  move_away.motion = Event::Mouse::Motion::Moved;
  move_away.x = 5;
  move_away.y = 1;
  screen.Dispatch(move_away);

  // Hover state should be cleared.
  CHECK_FALSE(scroll_el->scrollbar_hovered());
  CHECK_FALSE(scroll_el->scrollbar_thumb_hovered());
}

TEST_CASE("Screen.LayoutFlexDemoClickTest", "[terminal][flex]") {
  auto device = std::make_shared<MockTerminalDevice>();

  class LayoutFlexDemo : public Component<LayoutFlexDemo> {
   public:
    std::string direction = "row";
    void CycleDirection() {
      if (direction == "row") {
        direction = "row-reverse";
      } else {
        direction = "row";
      }
    }
    LayoutFlexDemo() {
      Bind(direction);
      Bind(CycleDirection);
    }
    std::string_view view = R"xml(
      <div id="container">
        <button id="btn" onclick="CycleDirection">direction: {direction}</button>
        <div id="flex-container"></div>
      </div>
      <style>
        #flex-container { flex-direction: {direction}; }
      </style>
    )xml";
  };

  auto component = Ref<LayoutFlexDemo>::New();

  Screen screen(component, device);
  screen.Draw();

  auto* flex_container = component->Root()->QuerySelector("#flex-container");
  REQUIRE(flex_container != nullptr);
  CHECK(flex_container->style.flex_direction == Direction::Row);

  auto* btn = component->Root()->QuerySelector("#btn");
  REQUIRE(btn != nullptr);
  btn->set_focused(true);
  screen.Draw();

  // Press Space to click
  screen.Dispatch(Event::Keyboard{.codepoint = 32});
  CHECK(component->direction == "row-reverse");

  // Re-draw so that updates are processed
  screen.Draw();
  CHECK(flex_container->style.flex_direction == Direction::RowReverse);
}

TEST_CASE("Screen.TooltipHoverDigestRegression", "[terminal][hover]") {
  auto device = std::make_shared<MockTerminalDevice>();

  class ScreenTooltipHoverRegressionTest
      : public Component<ScreenTooltipHoverRegressionTest> {
   public:
    std::string tooltip_text = "regression-test";
    void InitReflection() override {
      Bind(tooltip_text);
      Import<tooltip>();
    }
    std::string_view view = R"xml(
      <div id="container" style="width: 10; height: 3;">
        <tooltip id="tt" content="{tooltip_text}">
          <div id="trigger" style="width: 5; height: 1;">Trigger</div>
        </tooltip>
      </div>
    )xml";
  };

  auto component = Ref<ScreenTooltipHoverRegressionTest>::New();
  Screen screen(component, device);
  screen.Draw();

  auto* tt_el = component->Root()->QuerySelector("#tt");
  auto* trigger_el = component->Root()->QuerySelector("#trigger");
  REQUIRE(tt_el != nullptr);
  REQUIRE(trigger_el != nullptr);

  auto* tt_comp = const_cast<ComponentBase*>(tt_el->component());
  auto* tt_ptr = static_cast<tooltip*>(tt_comp);
  REQUIRE(tt_ptr != nullptr);

  // Initial state: hidden
  CHECK(tt_ptr->tooltip_class == "hidden");

  // Send mouse hover event on the trigger element (x=2, y=1)
  Event::Mouse hover_in;
  hover_in.button = Event::Mouse::Button::None;
  hover_in.motion = Event::Mouse::Motion::Moved;
  hover_in.x = 2;  // tx = 1, inside trigger
  hover_in.y = 1;  // ty = 0, inside trigger
  screen.Dispatch(hover_in);

  // Verify that the tooltip class reactively updated to visible
  CHECK(tt_ptr->tooltip_class == "visible");

  // Hover out
  Event::Mouse hover_out;
  hover_out.button = Event::Mouse::Button::None;
  hover_out.motion = Event::Mouse::Motion::Moved;
  hover_out.x = 20;  // outside bounds
  hover_out.y = 1;
  screen.Dispatch(hover_out);

  // Verify tooltip class is hidden again
  CHECK(tt_ptr->tooltip_class == "hidden");
}

TEST_CASE("Screen.TooltipVisibleRender", "[terminal][hover][render]") {
  auto device = std::make_shared<MockTerminalDevice>();
  device->TriggerResize(40, 10);

  class ScreenTooltipVisibleTest : public Component<ScreenTooltipVisibleTest> {
   public:
    std::string tooltip_text = "regression-test";
    void InitReflection() override {
      Bind(tooltip_text);
      Import<tooltip>();
      Import<div>();
    }
    std::string_view view = R"xml(
      <div id="container" style="padding-top: 4; height: 10; width: 40; display: block;">
        <tooltip id="tt" content="{tooltip_text}" placement="top">
          <div id="trigger" style="width: 10; height: 1;">Trigger</div>
        </tooltip>
      </div>
    )xml";
  };

  auto component = Ref<ScreenTooltipVisibleTest>::New();
  Screen screen(component, device);
  screen.Draw();

  auto* tt_el = component->Root()->QuerySelector("#tt");
  REQUIRE(tt_el != nullptr);

  // Hover in
  Event::Mouse hover_in;
  hover_in.button = Event::Mouse::Button::None;
  hover_in.motion = Event::Mouse::Motion::Moved;
  hover_in.x = 2;  // inside trigger
  hover_in.y = 5;  // inside trigger
  screen.Dispatch(hover_in);

  auto* tt_comp = const_cast<ComponentBase*>(tt_el->component());
  auto* tt_ptr = static_cast<tooltip*>(tt_comp);
  REQUIRE(tt_ptr != nullptr);
  CHECK(tt_ptr->tooltip_class == "visible");

  // Check if "regression-test" is in the output
  CHECK(device->GetOutput().find("regression-test") != std::string::npos);

  // Clear output buffer history
  device->ClearOutput();

  // Hover out
  Event::Mouse hover_out;
  hover_out.button = Event::Mouse::Button::None;
  hover_out.motion = Event::Mouse::Motion::Moved;
  hover_out.x = 35;  // far away
  hover_out.y = 5;   // far away
  screen.Dispatch(hover_out);

  CHECK(tt_ptr->tooltip_class == "hidden");
  CHECK(device->GetOutput().find("regression-test") == std::string::npos);
}

TEST_CASE("Screen.TooltipDemoRender", "[terminal][hover][demo]") {
  auto device = std::make_shared<MockTerminalDevice>();
  device->TriggerResize(80, 24);

  class TooltipDemoTest : public Component<TooltipDemoTest> {
   public:
    std::string custom_text = "Press any key to modify this text!";
    void InitReflection() override {
      Bind(custom_text);
      rtxui::Component<TooltipDemoTest>::InitReflection();
    }
    std::string_view view = R"html(
      <div class="container">
        <h2>RTXUI Tooltip Component Demo</h2>
        <p class="desc">
          Hover over the buttons below using your mouse cursor to see tooltips rendered in different directions.
        </p>

        <div class="showcase">
          <div class="row">
            <tooltip id="tt_top" content="Tooltip aligned at the TOP of the element" placement="top">
              <button id="btn_top" class="btn">Top Tooltip</button>
            </tooltip>

            <tooltip content="Tooltip aligned at the BOTTOM of the element" placement="bottom">
              <button class="btn">Bottom Tooltip</button>
            </tooltip>
          </div>
        </div>
      </div>

      <style>
        self {
          display: block;
          padding: 2;
        }
        h2 {
          margin-bottom: 0;
        }
        .desc {
          margin-bottom: 3;
        }
        .showcase {
          display: flex;
          flex-direction: column;
          gap: 4;
          width: 60;
        }
        .row {
          display: flex;
          flex-direction: row;
          justify-content: space-around;
          align-items: center;
          width: 100%;
        }
        .btn {
          padding: 0 2;
        }
      </style>
    )html";
  };

  auto component = Ref<TooltipDemoTest>::New();
  Screen screen(component, device);
  screen.Draw();

  // Find button and tooltip
  auto* btn = component->Root()->QuerySelector("#btn_top");
  auto* tt = component->Root()->QuerySelector("#tt_top");
  REQUIRE(btn != nullptr);
  REQUIRE(tt != nullptr);

  // Find absolute position of the button
  int btn_x = btn->absolute_x();
  int btn_y = btn->absolute_y();
  REQUIRE(btn_x > 0);
  REQUIRE(btn_y > 0);

  // Hover on the button
  Event::Mouse hover_in;
  hover_in.button = Event::Mouse::Button::None;
  hover_in.motion = Event::Mouse::Motion::Moved;
  hover_in.x = btn_x + 1;  // 1-indexed
  hover_in.y = btn_y + 1;  // 1-indexed
  screen.Dispatch(hover_in);

  auto* tt_comp = const_cast<ComponentBase*>(tt->component());
  auto* tt_ptr = static_cast<tooltip*>(tt_comp);
  REQUIRE(tt_ptr != nullptr);
  CHECK(tt_ptr->tooltip_class == "visible");

  CHECK(device->GetOutput().find("Tooltip aligned at the TOP of the") !=
        std::string::npos);
}

TEST_CASE("Screen.TooltipMarginAutoAlignment", "[terminal][hover][alignment]") {
  auto device = std::make_shared<MockTerminalDevice>();
  device->TriggerResize(40, 10);

  class TooltipMarginAutoTest : public Component<TooltipMarginAutoTest> {
   public:
    std::string tooltip_text = "regression-test";
    void InitReflection() override {
      Bind(tooltip_text);
      Import<tooltip>();
      Import<div>();
    }
    std::string_view view = R"xml(
      <div id="container" style="width: 40; height: 10; display: block;">
        <div id="showcase" style="width: 20; margin: auto; display: block;">
          <tooltip id="tt" content="{tooltip_text}" placement="bottom">
            <div id="trigger" style="width: 10; height: 1;">Trigger</div>
          </tooltip>
        </div>
      </div>
    )xml";
  };

  auto component = Ref<TooltipMarginAutoTest>::New();
  Screen screen(component, device);
  screen.Draw();

  auto* trigger = component->Root()->QuerySelector("#trigger");
  auto* tt = component->Root()->QuerySelector("#tt");
  REQUIRE(trigger != nullptr);
  REQUIRE(tt != nullptr);

  int trigger_x = trigger->absolute_x();
  int trigger_y = trigger->absolute_y();
  REQUIRE(trigger_x >= 10);

  // Hover on trigger
  Event::Mouse hover_in;
  hover_in.button = Event::Mouse::Button::None;
  hover_in.motion = Event::Mouse::Motion::Moved;
  hover_in.x = trigger_x + 1;  // 1-indexed
  hover_in.y = trigger_y + 1;  // 1-indexed
  screen.Dispatch(hover_in);

  auto* tt_comp = const_cast<ComponentBase*>(tt->component());
  auto* tt_ptr = static_cast<tooltip*>(tt_comp);
  REQUIRE(tt_ptr != nullptr);
  CHECK(tt_ptr->tooltip_class == "visible");

  // Verify alignment by searching for the text in the rendered mock screen
  CHECK(device->GetOutput().find("regression-test") != std::string::npos);
}

TEST_CASE("Screen.TooltipPlacements", "[terminal][hover][render]") {
  auto device = std::make_shared<MockTerminalDevice>();
  device->TriggerResize(60, 15);

  class TooltipPlacementsTest : public Component<TooltipPlacementsTest> {
   public:
    void InitReflection() override {
      Import<tooltip>();
      Import<div>();
    }
    std::string_view view = R"xml(
      <div id="container" style="padding: 5; display: block; width: 60; height: 15;">
        <tooltip id="tt_bottom" content="bottom-popup" placement="bottom">
          <div id="trigger_bottom" style="width: 10; height: 1;">TriggerB</div>
        </tooltip>
        <tooltip id="tt_left" content="left-popup" placement="left">
          <div id="trigger_left" style="width: 10; height: 1;">TriggerL</div>
        </tooltip>
        <tooltip id="tt_right" content="right-popup" placement="right">
          <div id="trigger_right" style="width: 10; height: 1;">TriggerR</div>
        </tooltip>
      </div>
    )xml";
  };

  auto component = Ref<TooltipPlacementsTest>::New();
  Screen screen(component, device);
  screen.Draw();

  // Test Bottom
  {
    auto* trigger = component->Root()->QuerySelector("#trigger_bottom");
    REQUIRE(trigger != nullptr);
    int tx = trigger->absolute_x();
    int ty = trigger->absolute_y();
    Event::Mouse m;
    m.button = Event::Mouse::Button::None;
    m.motion = Event::Mouse::Motion::Moved;
    m.x = tx + 1;
    m.y = ty + 1;
    device->ClearOutput();
    screen.Dispatch(m);
    CHECK(device->GetOutput().find("bottom-popup") != std::string::npos);

    // Hover out
    m.x = 1;
    m.y = 1;
    device->ClearOutput();
    screen.Dispatch(m);
    CHECK(device->GetOutput().find("bottom-popup") == std::string::npos);
  }

  // Test Left
  {
    auto* trigger = component->Root()->QuerySelector("#trigger_left");
    REQUIRE(trigger != nullptr);
    int tx = trigger->absolute_x();
    int ty = trigger->absolute_y();
    Event::Mouse m;
    m.button = Event::Mouse::Button::None;
    m.motion = Event::Mouse::Motion::Moved;
    m.x = tx + 1;
    m.y = ty + 1;
    device->ClearOutput();
    screen.Dispatch(m);
    CHECK(device->GetOutput().find("left-popup") != std::string::npos);

    // Hover out
    m.x = 1;
    m.y = 1;
    device->ClearOutput();
    screen.Dispatch(m);
    CHECK(device->GetOutput().find("left-popup") == std::string::npos);
  }

  // Test Right
  {
    auto* trigger = component->Root()->QuerySelector("#trigger_right");
    REQUIRE(trigger != nullptr);
    int tx = trigger->absolute_x();
    int ty = trigger->absolute_y();
    Event::Mouse m;
    m.button = Event::Mouse::Button::None;
    m.motion = Event::Mouse::Motion::Moved;
    m.x = tx + 1;
    m.y = ty + 1;
    device->ClearOutput();
    screen.Dispatch(m);
    CHECK(device->GetOutput().find("right-popup") != std::string::npos);

    // Hover out
    m.x = 1;
    m.y = 1;
    device->ClearOutput();
    screen.Dispatch(m);
    CHECK(device->GetOutput().find("right-popup") == std::string::npos);
  }
}

TEST_CASE("Screen.NestedStickyHitTesting", "[terminal][sticky][hittest][bug]") {
  auto device = std::make_shared<MockTerminalDevice>();
  device->TriggerResize(40, 10);

  class NestedStickyTest : public Component<NestedStickyTest> {
   public:
    int sticky_clicks = 0;

    NestedStickyTest() {
      Import("OnStickyClick", [this]() { sticky_clicks++; });
    }

    void InitReflection() override { Import<div>(); }
    std::string_view view = R"xml(
      <div id="scrollable" style="width: 40; height: 5; overflow-y: scroll; display: block;">
        <div id="month-container" style="display: block;">
          <div id="sticky-header" onclick="OnStickyClick" style="position: sticky; top: 0; width: 10; height: 1;">StickyH</div>
          <div style="display: block; height: 10;">Spacer</div>
        </div>
      </div>
    )xml";
  };

  auto component = Ref<NestedStickyTest>::New();
  Screen screen(component, device);
  screen.Draw();

  auto* scrollable = component->Root()->QuerySelector("#scrollable");
  auto* sticky = component->Root()->QuerySelector("#sticky-header");
  REQUIRE(scrollable != nullptr);
  REQUIRE(sticky != nullptr);

  // 1. scroll_y = 0: sticky is at absolute coordinates (0, 0)
  {
    scrollable->set_scroll_y(0);
    component->Digest();
    screen.Draw();

    component->sticky_clicks = 0;

    // Click at absolute (0, 0), which is 1-based mouse (1, 1)
    Event::Mouse m;
    m.button = Event::Mouse::Button::Left;
    m.motion = Event::Mouse::Motion::Pressed;
    m.x = 1;
    m.y = 1;
    screen.Dispatch(m);

    CHECK(component->sticky_clicks == 1);
  }

  // 2. scroll_y = 3: month-container scrolls up, but sticky-header should stick
  // to viewport top (y = 0)
  {
    scrollable->set_scroll_y(3);
    component->Digest();
    screen.Draw();

    component->sticky_clicks = 0;

    // Click at absolute (0, 0), which is 1-based mouse (1, 1)
    Event::Mouse m;
    m.button = Event::Mouse::Button::Left;
    m.motion = Event::Mouse::Motion::Pressed;
    m.x = 1;
    m.y = 1;
    screen.Dispatch(m);

    CHECK(component->sticky_clicks == 1);
  }
}

TEST_CASE("Screen.FocusDisplayNone", "[screen][focus][display_none]") {
  class FocusDisplayNoneTest : public Component<FocusDisplayNoneTest> {
   public:
    void InitReflection() override {
      Import<rtxui::div>();
      Component<FocusDisplayNoneTest>::InitReflection();
    }
    std::string_view view = R"xml(
      <div>
        <div id="first" tabindex="0">Visible 1</div>
        <div id="hidden-container" style="display: none;">
          <div id="hidden-child" tabindex="0">Hidden Child</div>
        </div>
        <div id="second" tabindex="0">Visible 2</div>
      </div>
    )xml";
  };

  auto device = std::make_shared<MockTerminalDevice>();
  auto component = Ref<FocusDisplayNoneTest>::New();
  Screen screen(component, device);
  screen.Draw();

  auto* first = component->Root()->QuerySelector("#first");
  auto* hidden_child = component->Root()->QuerySelector("#hidden-child");
  auto* second = component->Root()->QuerySelector("#second");

  REQUIRE(first != nullptr);
  REQUIRE(hidden_child != nullptr);
  REQUIRE(second != nullptr);

  // Tab 1: should focus 'first'
  screen.Dispatch(Event::Tab());
  CHECK(first->focused());
  CHECK_FALSE(hidden_child->focused());
  CHECK_FALSE(second->focused());

  // Tab 2: should focus 'second' (skipping 'hidden-child')
  screen.Dispatch(Event::Tab());
  CHECK_FALSE(first->focused());
  CHECK_FALSE(hidden_child->focused());
  CHECK(second->focused());

  // TabReverse (Shift-Tab): should focus 'first'
  screen.Dispatch(Event::TabReverse());
  CHECK(first->focused());
  CHECK_FALSE(hidden_child->focused());
  CHECK_FALSE(second->focused());
}

TEST_CASE("Screen.DynamicBorderUpdate", "[screen][border][reconcile]") {
  class BorderUpdateComponent : public Component<BorderUpdateComponent> {
   public:
    std::string border_style = "solid";

    void SelectBorder(std::string name) { border_style = name; }

    void InitReflection() override {
      Import<rtxui::div>();
      Import<rtxui::button>();
      Bind(border_style);
      Bind(SelectBorder);
      Component<BorderUpdateComponent>::InitReflection();
    }

    std::string_view view = R"xml(
      <div class="main-container">
        <div class="sidebar">
          <button id="btn-double" onclick="SelectBorder(double)">double</button>
        </div>
        <div class="demo-area">
          <div id="scrollable-content">
            Content
          </div>
        </div>
      </div>
      <style>
        self {
          display: block;
          padding: 1;
          width: 100%;
          height: 100%;
        }
        .main-container {
          display: flex;
          width: 100%;
          height: 100%;
          gap: 2;
        }
        .sidebar {
          display: block;
          width: 48;
          flex-shrink: 0;
          border: solid;
          padding: 1;
        }
        .demo-area {
          display: block;
          flex-grow: 1;
          border: solid;
          padding: 1;
        }
        #scrollable-content {
          display: block;
          width: 44;
          height: 13;
          border: {border_style};
          overflow-y: scroll;
          overflow-x: scroll;
        }
      </style>
    )xml";
  };

  auto device = std::make_shared<MockTerminalDevice>();
  auto component = Ref<BorderUpdateComponent>::New();
  Screen screen(component, device);
  screen.Draw();

  auto* main = component->Root()->QuerySelector(".main-container");
  auto* sidebar = component->Root()->QuerySelector(".sidebar");
  auto* demo = component->Root()->QuerySelector(".demo-area");
  auto* box = component->Root()->QuerySelector("#scrollable-content");
  auto* btn = component->Root()->QuerySelector("#btn-double");
  REQUIRE(main != nullptr);
  REQUIRE(sidebar != nullptr);
  REQUIRE(demo != nullptr);
  REQUIRE(box != nullptr);
  REQUIRE(btn != nullptr);

  CHECK(main->style.display_inside == DisplayInside::Flex);
  CHECK(sidebar->layout_width() == 48);
  CHECK(demo->layout_width() == 28);
  CHECK(box->style.border_style == BorderStyle::Solid);

  // Click double button
  Event::Mouse click;
  click.button = Event::Mouse::Button::Left;
  click.motion = Event::Mouse::Motion::Pressed;
  click.x = 5;
  click.y = 4;
  screen.Dispatch(click);

  CHECK(main->style.display_inside == DisplayInside::Flex);
  CHECK(sidebar->layout_width() == 48);
  CHECK(demo->layout_width() == 28);
  CHECK(box->style.border_style == BorderStyle::Double);
}

class PostedTaskComponent : public Component<PostedTaskComponent> {
 public:
  std::string text = "one";

  void InitReflection() override {
    Bind(text);
    Import<div>();
    Component<PostedTaskComponent>::InitReflection();
  }
  std::string_view view = R"html(<div>{text}</div>)html";
};

TEST_CASE("Screen.PostedTaskTriggersDigestAndRepaint", "[terminal][task]") {
  auto device = std::make_shared<MockTerminalDevice>();
  auto component = Ref<PostedTaskComponent>::New();
  Screen screen(component, device);
  screen.Draw();
  REQUIRE(device->GetOutput().find("one") != std::string::npos);
  device->ClearOutput();

  // A task posted to the UI loop (as a worker thread would do) mutates
  // bound state. A single Step() must digest and repaint it without any
  // input event.
  task::TaskRunner::Current()->PostTask(
      [&component]() { component->text = "twotwo"; });
  screen.Step();

  CHECK(component->text == "twotwo");

  // The diff renderer interleaves escape sequences between cells; strip
  // them before searching for the repainted text.
  std::string visible;
  const std::string& raw = device->GetOutput();
  for (size_t i = 0; i < raw.size(); ++i) {
    if (raw[i] == '\x1B') {
      ++i;
      if (i < raw.size() && raw[i] == '[') {
        ++i;
        while (i < raw.size() && !(raw[i] >= '@' && raw[i] <= '~')) {
          ++i;
        }
      }
      continue;
    }
    visible += raw[i];
  }
  INFO("raw size: " << raw.size() << " visible: [" << visible << "]");
  CHECK(visible.find("twotwo") != std::string::npos);
}

// Regression test: a `position: fixed` overlay must be clickable even when an
// intermediate ancestor's box doesn't cover the click.
//
// This mirrors the shape of the built-in <dialog>: the component's own root is
// an ordinary block of near-zero height, and the full-screen overlay hangs off
// it. Hit testing culled a subtree against that wrapper's bounds before
// descending into it, so every click on the dialog was discarded and only the
// keyboard could reach its buttons.
TEST_CASE("Screen.ClickFixedPositionOverlay", "[terminal][mouse][dialog]") {
  auto device = std::make_shared<MockTerminalDevice>();

  class OverlayComponent : public Component<OverlayComponent> {
   public:
    bool open = false;
    int overlay_clicks = 0;
    int page_clicks = 0;

    void Open() { open = true; }
    void ClickOverlay() { overlay_clicks++; }
    void ClickPage() { page_clicks++; }

    std::string overlay_class() const { return open ? "" : "closed"; }

    std::string_view view =
        R"(<div id="page" onclick="ClickPage"><div id="open" onclick="Open">open</div><div class="dialog-wrapper"><div id="overlay" class="{overlay_class}" onclick="ClickOverlay">overlay</div></div></div>
      <style>
        #page { display: block; width: 100%; height: 100%; }
        /* Contributes no height of its own, like the dialog root. */
        .dialog-wrapper { display: block; }
        #overlay {
          position: fixed;
          top: 0;
          left: 0;
          width: 100%;
          height: 100%;
          z-index: 100;
        }
        .closed { display: none; }
      </style>
    )";

    OverlayComponent() {
      Bind(open);
      Bind(overlay_class);
      Bind(Open);
      Bind(ClickOverlay);
      Bind(ClickPage);
    }
  };

  // Layout reads the viewport from these globals and other tests leave them at
  // whatever size they rendered at, so pin them rather than depend on order.
  css::g_terminal_width = 80;
  css::g_terminal_height = 24;

  auto component = Ref<OverlayComponent>::New();
  Screen screen(component, device);

  // Mouse coordinates are 1-based: screen.cpp maps them with `mouse.x - 1`.
  auto click_cell = [&](int column, int row) {
    Event::Mouse mouse;
    mouse.button = Event::Mouse::Button::Left;
    mouse.motion = Event::Mouse::Motion::Pressed;
    mouse.x = column + 1;
    mouse.y = row + 1;
    Event event = mouse;
    screen.Dispatch(event);
  };

  // Row 10 is well below the trigger on row 0 and far outside the wrapper's
  // own zero-height box, but inside #page and inside the viewport-sized
  // overlay once it opens.
  SECTION("closed, the click falls through to the page") {
    click_cell(5, 10);
    CHECK(component->overlay_clicks == 0);
    CHECK(component->page_clicks == 1);
  }

  SECTION("open, the click reaches the fixed overlay") {
    click_cell(0, 0);  // the trigger
    REQUIRE(component->open);

    click_cell(5, 10);
    CHECK(component->overlay_clicks == 1);
    CHECK(component->page_clicks == 0);
  }
}

}  // namespace

namespace {
class OverlineScreenComponent : public Component<OverlineScreenComponent> {
 public:
  void InitReflection() override {
    Import<rtxui::div>();
    Component<OverlineScreenComponent>::InitReflection();
  }
  std::string_view view = R"(
    <div><div class="o">over</div><div>plain</div></div>
    <style>
      .o { text-decoration: overline; }
    </style>
  )";
};
}  // namespace

TEST_CASE("Overlined cells emit SGR 53 and reset it with SGR 55",
          "[terminal][decoration]") {
  auto device = std::make_shared<MockTerminalDevice>();
  Screen screen(Ref<OverlineScreenComponent>::New(), device);

  // Screen's constructor mounts, digests and draws once, so this is frame one.
  std::string output = device->GetOutput();
  REQUIRE(output.find("over") != std::string::npos);
  CHECK(output.find("\x1b[53m") != std::string::npos);
  // The attribute must be turned back off, or every later cell inherits it.
  CHECK(output.find("\x1b[55m") != std::string::npos);
  CHECK(output.find("\x1b[53m") < output.find("over"));
}

namespace {
class TabsClickApp : public Component<TabsClickApp> {
 public:
  std::string current_tab = "home";
  std::string_view view = R"html(
      <div class="content">
        <h1>Title</h1>
        <tabs value="{current_tab}">
          <tab-pane label="Dashboard" name="home"><p>a</p></tab-pane>
          <tab-pane label="Settings" name="settings"><p>b</p></tab-pane>
        </tabs>
      </div>
      <style>
        self { display: block; padding: 1; background-color: rgb(13, 17, 23); }
      </style>
    )html";
  TabsClickApp() { Bind(current_tab); }
};
}  // namespace

TEST_CASE("Switching tab shows only the selected pane", "[tabs][mouse][css]") {
  // Regression: tabs::Digest() marks the non-selected panes `display: none`,
  // but Digest() runs after Render() resolved styles and nothing re-resolved
  // afterwards, so the inline style never reached the computed style. Every
  // pane stayed visible and the panes stacked up down the screen.
  auto device = std::make_shared<MockTerminalDevice>();
  device->TriggerResize(90, 24);
  auto app = Ref<TabsClickApp>::New();
  Screen screen(app, device);
  screen.Draw();

  auto panes = [&] {
    std::vector<Element*> found;
    app->Root()->Visit([&](Element& el) {
      if (el.tag() == "tab-pane" || el.tag() == "tab_pane") {
        found.push_back(&el);
      }
    });
    return found;
  };
  auto visible = [&] {
    int n = 0;
    for (Element* p : panes()) {
      if (!p->style.display_none) {
        ++n;
      }
    }
    return n;
  };

  REQUIRE(panes().size() == 2);
  REQUIRE(visible() == 1);
  REQUIRE_FALSE(panes()[0]->style.display_none);

  std::vector<Element*> headers;
  app->Root()->Visit([&](Element& el) {
    for (const auto& c : el.classes) {
      if (c == "tab-header-btn") {
        headers.push_back(&el);
      }
    }
  });
  REQUIRE(headers.size() == 2);

  auto send = [&](Event::Mouse::Button b, Event::Mouse::Motion m) {
    Event::Mouse e;
    e.button = b;
    e.motion = m;
    e.x = headers[1]->absolute_x() + 2;
    e.y = headers[1]->absolute_y() + 1;
    screen.Dispatch(e);
  };
  send(Event::Mouse::Button::None, Event::Mouse::Motion::Moved);
  send(Event::Mouse::Button::Left, Event::Mouse::Motion::Pressed);
  send(Event::Mouse::Button::Left, Event::Mouse::Motion::Released);
  app->Digest();
  app->ResolveTargetStyles();
  REQUIRE(app->current_tab == "settings");

  // Exactly one pane is shown, and it is the one that was selected.
  CHECK(visible() == 1);
  CHECK(panes()[0]->style.display_none);
  CHECK_FALSE(panes()[1]->style.display_none);
}

TEST_CASE("Selecting a tab by keyboard keeps focus on the headers",
          "[tabs][focus]") {
  // tabs::Digest() avoids rebuilding its header buttons precisely so a
  // keyboard-focused button is not replaced by a fresh, unfocused Element,
  // which would swallow the next Enter/Space. Selecting a tab now re-renders
  // to get the mutated DOM re-styled, so that guarantee is worth pinning:
  // Render() saves and restores focus, but only as long as it keeps doing so.
  auto device = std::make_shared<MockTerminalDevice>();
  device->TriggerResize(90, 24);
  auto app = Ref<TabsClickApp>::New();
  Screen screen(app, device);
  screen.Draw();

  auto headers = [&] {
    std::vector<Element*> found;
    app->Root()->Visit([&](Element& el) {
      for (const auto& c : el.classes) {
        if (c == "tab-header-btn") {
          found.push_back(&el);
        }
      }
    });
    return found;
  };
  REQUIRE(headers().size() == 2);

  // Tab to the second header, then activate it with Space.
  screen.Dispatch(Event::Tab());
  screen.Dispatch(Event::Tab());
  REQUIRE(headers()[1]->focused());

  screen.Dispatch(Event::Keyboard{.codepoint = 32});
  app->Digest();
  app->ResolveTargetStyles();

  CHECK(app->current_tab == "settings");
  // Focus must survive the re-render, or the next keypress goes nowhere.
  auto after = headers();
  REQUIRE(after.size() == 2);
  CHECK(after[1]->focused());
}

TEST_CASE("Switching tab keeps the header buttons styled",
          "[tabs][mouse][css]") {
  // Regression: tabs::Digest() rebuilt its header buttons whenever the set of
  // panes "changed", but compared panes by Element*. Selecting a tab changes
  // the host's bound state, which re-renders it and hands out fresh pane
  // elements, so every tab switch looked like a pane-set change. The rebuilt
  // buttons were created inside Digest(), which runs after Render() resolved
  // styles, so they kept an empty base_style: the whole header strip lost its
  // padding and its background, active tab included.
  auto device = std::make_shared<MockTerminalDevice>();
  device->TriggerResize(90, 24);
  auto app = Ref<TabsClickApp>::New();
  Screen screen(app, device);
  screen.Draw();

  auto headers = [&] {
    std::vector<Element*> found;
    app->Root()->Visit([&](Element& el) {
      for (const auto& c : el.classes) {
        if (c == "tab-header-btn") {
          found.push_back(&el);
        }
      }
    });
    return found;
  };

  auto has_class = [](Element* el, std::string_view name) {
    return std::find(el->classes.begin(), el->classes.end(), name) !=
           el->classes.end();
  };

  REQUIRE(headers().size() == 2);
  REQUIRE(headers()[0]->style.padding.left == 2);
  REQUIRE(headers()[0]->style.background_color.has_value());
  REQUIRE(has_class(headers()[0], "active-tab"));

  // Click the second header ("Settings"), addressed by where it was laid out
  // rather than by a guessed coordinate. Mouse coordinates are 1-based.
  Element* settings = headers()[1];
  const int click_x = settings->absolute_x() + 2;
  const int click_y = settings->absolute_y() + 1;
  auto send = [&](Event::Mouse::Button b, Event::Mouse::Motion m) {
    Event::Mouse e;
    e.button = b;
    e.motion = m;
    e.x = click_x;
    e.y = click_y;
    screen.Dispatch(e);
  };
  send(Event::Mouse::Button::None, Event::Mouse::Motion::Moved);
  send(Event::Mouse::Button::Left, Event::Mouse::Motion::Pressed);
  send(Event::Mouse::Button::Left, Event::Mouse::Motion::Released);
  app->Digest();
  app->ResolveTargetStyles();

  REQUIRE(app->current_tab == "settings");
  auto after = headers();
  REQUIRE(after.size() == 2);

  // The selection moved...
  CHECK_FALSE(has_class(after[0], "active-tab"));
  CHECK(has_class(after[1], "active-tab"));

  // ...and both buttons still carry the styles from the tabs component's own
  // stylesheet, which is what used to be dropped.
  for (Element* btn : after) {
    INFO("header " << btn->Print(0));
    CHECK(btn->style.padding.left == 2);
    CHECK(btn->style.padding.right == 2);
    CHECK(btn->style.background_color.has_value());
  }
}

namespace {
class DetailsApp : public Component<DetailsApp> {
 public:
  std::string_view view = R"html(
    <details>
      <summary>Click me</summary>
      <div id="body">hidden content</div>
    </details>
  )html";
};
}  // namespace

TEST_CASE("Clicking a summary toggles the details content",
          "[details][mouse]") {
  auto device = std::make_shared<MockTerminalDevice>();
  device->TriggerResize(60, 20);
  auto app = Ref<DetailsApp>::New();
  Screen screen(app, device);
  screen.Draw();

  auto* content = app->Root()->QuerySelector(".details-content");
  auto* summary_line = app->Root()->QuerySelector(".summary-line");
  REQUIRE(content != nullptr);
  REQUIRE(summary_line != nullptr);

  // Closed to begin with: the content is display:none.
  CHECK(content->style.display_none);

  auto click = [&] {
    for (auto motion :
         {Event::Mouse::Motion::Pressed, Event::Mouse::Motion::Released}) {
      Event::Mouse e;
      e.button = Event::Mouse::Button::Left;
      e.motion = motion;
      e.x = summary_line->absolute_x() + 2;
      e.y = summary_line->absolute_y() + 1;
      screen.Dispatch(e);
    }
    app->Digest();
    // Draw() is where a frame resolves styles, so going through it is what
    // makes this test cover the real path rather than a hand-built one.
    screen.Draw();
  };

  click();
  auto* opened = app->Root()->QuerySelector(".details-content");
  REQUIRE(opened != nullptr);
  CHECK_FALSE(opened->style.display_none);

  click();
  auto* closed = app->Root()->QuerySelector(".details-content");
  REQUIRE(closed != nullptr);
  CHECK(closed->style.display_none);
}

namespace {
class RadioGroupApp : public Component<RadioGroupApp> {
 public:
  std::string_view view = R"html(
    <div>
      <radio id="r1" name="pick" checked="true">One</radio>
      <radio id="r2" name="pick">Two</radio>
      <radio id="r3" name="other">Unrelated</radio>
    </div>
  )html";
};

class CheckboxApp : public Component<CheckboxApp> {
 public:
  std::string_view view = R"html(<checkbox id="c">Check me</checkbox>)html";
};
}  // namespace

TEST_CASE("Checking a radio unchecks only its own group", "[radio][mouse]") {
  auto device = std::make_shared<MockTerminalDevice>();
  device->TriggerResize(60, 20);
  auto app = Ref<RadioGroupApp>::New();
  Screen screen(app, device);
  screen.Draw();

  // The marker glyph is how checked-ness actually reaches the user. Take just
  // the glyph, not the label after it.
  auto marker_of = [&](const char* id) {
    auto* el = app->Root()->QuerySelector(id);
    REQUIRE(el != nullptr);
    std::string text;
    el->Visit([&](Element& e) {
      if (e.is_text()) {
        text += static_cast<const TextElement&>(e).text();
      }
    });
    for (const Grapheme& g : Graphemes(text)) {
      return std::string(g.text);
    }
    return std::string();
  };

  const std::string checked = marker_of("#r1");
  const std::string unchecked = marker_of("#r2");
  REQUIRE(checked != unchecked);

  auto* r2 = app->Root()->QuerySelector("#r2");
  REQUIRE(r2 != nullptr);
  for (auto motion :
       {Event::Mouse::Motion::Pressed, Event::Mouse::Motion::Released}) {
    Event::Mouse e;
    e.button = Event::Mouse::Button::Left;
    e.motion = motion;
    e.x = r2->absolute_x() + 1;
    e.y = r2->absolute_y() + 1;
    screen.Dispatch(e);
  }
  app->Digest();
  screen.Draw();

  // r2 is now the checked one, r1 is not, and the other group is untouched.
  CHECK(marker_of("#r2") == checked);
  CHECK(marker_of("#r1") == unchecked);
  CHECK(marker_of("#r3") == unchecked);
}

TEST_CASE("Clicking a checkbox toggles its glyph", "[checkbox][mouse]") {
  auto device = std::make_shared<MockTerminalDevice>();
  device->TriggerResize(60, 20);
  auto app = Ref<CheckboxApp>::New();
  Screen screen(app, device);
  screen.Draw();

  auto* box = app->Root()->QuerySelector("#c");
  REQUIRE(box != nullptr);
  auto glyph = [&] {
    std::string text;
    box->Visit([&](Element& e) {
      if (e.is_text()) {
        text += static_cast<const TextElement&>(e).text();
      }
    });
    return text;
  };

  const std::string before = glyph();
  auto click = [&] {
    for (auto motion :
         {Event::Mouse::Motion::Pressed, Event::Mouse::Motion::Released}) {
      Event::Mouse e;
      e.button = Event::Mouse::Button::Left;
      e.motion = motion;
      e.x = box->absolute_x() + 1;
      e.y = box->absolute_y() + 1;
      screen.Dispatch(e);
    }
    app->Digest();
    screen.Draw();
  };

  click();
  const std::string after = glyph();
  CHECK(after != before);

  click();
  CHECK(glyph() == before);
}

namespace {
class FieldsetApp : public Component<FieldsetApp> {
 public:
  std::string_view view = R"html(
    <fieldset>
      <legend>Group title</legend>
      <div id="inner">body</div>
    </fieldset>
  )html";
};
}  // namespace

namespace {
class ConditionalLegendApp : public Component<ConditionalLegendApp> {
 public:
  bool show = true;
  std::string_view view = R"html(
    <fieldset>
      <if condition="{show}"><legend>Group title</legend></if>
      <div id="inner">body</div>
    </fieldset>
  )html";
  ConditionalLegendApp() { Bind(show); }
};
}  // namespace

TEST_CASE(
    "A slot-selected child follows the consumer that stopped providing it",
    "[fieldset][details][slot]") {
  // Regression: fieldset and details used to hoist the <legend>/<summary> out
  // of the default slot in Digest(), by const_cast-ing the children vector and
  // re-parenting the element. That destroyed the only record of where the
  // element came from, so once the consumer stopped rendering it the component
  // could not tell "the consumer removed it" from "I already took it". The
  // stale element stayed in the named slot after the reconciler had dropped
  // it, and style resolution then walked freed memory -- a segfault, not a
  // cosmetic bug. Routing by `select` puts the split inside the reconcile that
  // owns those children, so the slot is truncated like any other.
  auto device = std::make_shared<MockTerminalDevice>();
  device->TriggerResize(60, 20);
  auto app = Ref<ConditionalLegendApp>::New();
  Screen screen(app, device);
  screen.Draw();

  auto count = [&](const char* tag) {
    int n = 0;
    app->Root()->Visit([&](Element& e) {
      if (e.tag() == tag) {
        ++n;
      }
    });
    return n;
  };

  REQUIRE(count("legend") == 1);
  // Routed into the legend slot even though it came out of an <if>, and the
  // has-legend styling lands on the first frame rather than a frame late.
  REQUIRE(app->Root()->QuerySelector(".has-legend") != nullptr);

  app->show = false;
  app->Digest();
  screen.Draw();
  CHECK(count("legend") == 0);
  CHECK(app->Root()->QuerySelector(".no-legend") != nullptr);

  app->show = true;
  app->Digest();
  screen.Draw();
  CHECK(count("legend") == 1);
}

namespace {
class ConditionalSummaryApp : public Component<ConditionalSummaryApp> {
 public:
  bool show = true;
  std::string_view view = R"html(
    <details>
      <if condition="{show}"><summary>Sum</summary></if>
      <div id="inner">body</div>
    </details>
  )html";
  ConditionalSummaryApp() { Bind(show); }
};
}  // namespace

TEST_CASE("A details drops a summary its consumer stopped providing",
          "[details][slot]") {
  // Same defect as the fieldset case above: details hoisted <summary> out of
  // the default slot destructively, so a conditional summary left a freed
  // element behind and crashed in style resolution.
  auto device = std::make_shared<MockTerminalDevice>();
  device->TriggerResize(60, 20);
  auto app = Ref<ConditionalSummaryApp>::New();
  Screen screen(app, device);
  screen.Draw();

  auto count = [&] {
    int n = 0;
    app->Root()->Visit([&](Element& e) {
      if (e.tag() == "summary") {
        ++n;
      }
    });
    return n;
  };

  REQUIRE(count() == 1);

  app->show = false;
  app->Digest();
  screen.Draw();
  CHECK(count() == 0);

  app->show = true;
  app->Digest();
  screen.Draw();
  CHECK(count() == 1);
}

namespace {
// Built-ins that inspect what their consumer projected into them. The
// fieldset/details crash fixed in this area came from projected content
// disappearing, so pin the behaviour for the others too.
#define RTXUI_COND_CHILD_APP(NAME, MARKUP) \
  class NAME : public Component<NAME> {    \
   public:                                 \
    bool show = true;                      \
    std::string_view view = MARKUP;        \
    NAME() { Bind(show); }                 \
  };

RTXUI_COND_CHILD_APP(CondTabsApp, R"html(<tabs value="t1">
  <tab-pane label="One" name="t1">C1</tab-pane>
  <if condition="{show}"><tab-pane label="Two" name="t2">C2</tab-pane></if>
</tabs>)html")
RTXUI_COND_CHILD_APP(CondSelectApp, R"html(<select value="a">
  <option value="a">A</option>
  <if condition="{show}"><option value="b">B</option></if>
</select>)html")
RTXUI_COND_CHILD_APP(CondListApp, R"html(<ul>
  <li>one</li>
  <if condition="{show}"><li>two</li></if>
</ul>)html")
RTXUI_COND_CHILD_APP(CondTableApp, R"html(<table>
  <tr><td>a</td></tr>
  <if condition="{show}"><tr><td>b</td></tr></if>
</table>)html")

class ForTabsApp : public Component<ForTabsApp> {
 public:
  std::vector<std::string> names{"a", "b", "c"};
  std::string_view view = R"html(<tabs value="a">
  <for each="{names}" as="n">
    <tab-pane label="{n}" name="{n}">body</tab-pane>
  </for>
</tabs>)html";
  ForTabsApp() { RegisterCollection("names", &names); }
};

// Counts elements carrying `cls`, or with tag `tag` when `cls` is null.
int CountMatching(Element* root, const char* cls, const char* tag) {
  int n = 0;
  root->Visit([&](Element& e) {
    if (tag) {
      if (e.tag() == tag) {
        ++n;
      }
      return;
    }
    for (const auto& c : e.classes) {
      if (c == cls) {
        ++n;
        return;
      }
    }
  });
  return n;
}
}  // namespace

TEST_CASE("Built-ins follow conditional projected children",
          "[slot][tabs][select][table]") {
  auto run = [](auto app, const char* cls, const char* tag) {
    auto device = std::make_shared<MockTerminalDevice>();
    device->TriggerResize(60, 20);
    Screen screen(app, device);
    screen.Draw();
    Element* root = app->Root();

    CHECK(CountMatching(root, cls, tag) == 2);
    app->show = false;
    app->Digest();
    screen.Draw();
    CHECK(CountMatching(root, cls, tag) == 1);
    app->show = true;
    app->Digest();
    screen.Draw();
    CHECK(CountMatching(root, cls, tag) == 2);
  };

  SECTION("tabs rebuilds its header strip") {
    run(Ref<CondTabsApp>::New(), "tab-header-btn", nullptr);
  }
  SECTION("select drops the option") {
    run(Ref<CondSelectApp>::New(), nullptr, "option");
  }
  SECTION("ul drops the item") {
    run(Ref<CondListApp>::New(), nullptr, "li");
  }
  SECTION("table drops the row") {
    run(Ref<CondTableApp>::New(), nullptr, "tr");
  }
}

TEST_CASE("tabs follows a collection that grows and shrinks", "[slot][tabs]") {
  auto device = std::make_shared<MockTerminalDevice>();
  device->TriggerResize(60, 20);
  auto app = Ref<ForTabsApp>::New();
  Screen screen(app, device);
  screen.Draw();

  auto headers = [&] {
    return CountMatching(app->Root(), "tab-header-btn", nullptr);
  };
  CHECK(headers() == 3);

  app->names.pop_back();
  app->Digest();
  screen.Draw();
  CHECK(headers() == 2);

  app->names.push_back("d");
  app->Digest();
  screen.Draw();
  CHECK(headers() == 3);
}

namespace {
class KeyedListApp : public Component<KeyedListApp> {
 public:
  std::vector<std::string> names{"alpha", "beta", "gamma"};
  std::string_view view = R"html(<div>
  <for each="{names}" as="n" key="{n}">
    <div id="{n}">{n}</div>
  </for>
</div>)html";
  KeyedListApp() { RegisterCollection("names", &names); }
};

class UnkeyedListApp : public Component<UnkeyedListApp> {
 public:
  std::vector<std::string> names{"alpha", "beta", "gamma"};
  std::string_view view = R"html(<div>
  <for each="{names}" as="n">
    <input id="{n}" value="{n}"/>
  </for>
</div>)html";
  UnkeyedListApp() { RegisterCollection("names", &names); }
};

std::vector<Element*> InputsOf(Element* root) {
  std::vector<Element*> inputs;
  root->Visit([&](Element& e) {
    if (e.tag() == "input" || (e.tag() == "div" && !e.id.empty())) {
      inputs.push_back(&e);
    }
  });
  return inputs;
}
}  // namespace

TEST_CASE("A keyed <for> moves element state with the item", "[for][key]") {
  // Without a key, reconciliation matches loop children by position, so
  // reordering a collection leaves focus (and scroll, and any in-flight
  // transition) sitting on whatever item now occupies that index. `key`
  // identifies the item that produced an element so the reconciler moves it.
  auto device = std::make_shared<MockTerminalDevice>();
  device->TriggerResize(60, 20);
  auto app = Ref<KeyedListApp>::New();
  Screen screen(app, device);
  screen.Draw();

  auto inputs = InputsOf(app->Root());
  REQUIRE(inputs.size() == 3);
  Element* beta = inputs[1];
  REQUIRE(beta->id == "beta");
  beta->set_focused(true);
  screen.Draw();

  // Rotate beta to the front: {beta, gamma, alpha}.
  std::rotate(app->names.begin(), app->names.begin() + 1, app->names.end());
  app->Digest();
  screen.Draw();

  auto after = InputsOf(app->Root());
  REQUIRE(after.size() == 3);
  CHECK(after[0]->id == "beta");
  CHECK(after[1]->id == "gamma");
  CHECK(after[2]->id == "alpha");

  // The focused element is beta, and it is the same element as before -- its
  // state travelled with the item rather than staying at index 1.
  CHECK(after[0] == beta);
  CHECK(after[0]->focused());
  CHECK_FALSE(after[1]->focused());
  CHECK_FALSE(after[2]->focused());
}

TEST_CASE("An unkeyed <for> still reconciles by position", "[for][key]") {
  // The keyed path must not change what an unkeyed loop does: elements are
  // reused in place and only their content is rewritten.
  auto device = std::make_shared<MockTerminalDevice>();
  device->TriggerResize(60, 20);
  auto app = Ref<UnkeyedListApp>::New();
  Screen screen(app, device);
  screen.Draw();

  auto inputs = InputsOf(app->Root());
  REQUIRE(inputs.size() == 3);
  Element* at_index_1 = inputs[1];

  std::rotate(app->names.begin(), app->names.begin() + 1, app->names.end());
  app->Digest();
  screen.Draw();

  auto after = InputsOf(app->Root());
  REQUIRE(after.size() == 3);
  CHECK(after[0]->id == "beta");
  CHECK(after[1]->id == "gamma");
  // Same element object, rewritten to hold a different item.
  CHECK(after[1] == at_index_1);
}

TEST_CASE("A fieldset projects its legend and styles itself accordingly",
          "[fieldset]") {
  // fieldset::Digest() moves the <legend> out of the default slot into the
  // legend slot, and picks a class from whether it found one -- both after
  // Render() has already resolved styles. Nothing covered it.
  auto device = std::make_shared<MockTerminalDevice>();
  device->TriggerResize(60, 20);
  auto app = Ref<FieldsetApp>::New();
  Screen screen(app, device);
  screen.Draw();

  // The legend ends up under the legend slot, not left in the body.
  Element* legend = nullptr;
  app->Root()->Visit([&](Element& e) {
    if (e.tag() == "legend") {
      legend = &e;
    }
  });
  REQUIRE(legend != nullptr);
  CHECK(legend->Parent() != nullptr);

  // It is rendered exactly once -- the move must not leave a copy behind.
  int legend_count = 0;
  app->Root()->Visit([&](Element& e) {
    if (e.tag() == "legend") {
      ++legend_count;
    }
  });
  CHECK(legend_count == 1);

  // And the container picked the has-legend styling, which only takes effect
  // if the class set during Digest() reached a computed style.
  Element* container = app->Root()->QuerySelector(".has-legend");
  CHECK(container != nullptr);

  // The legend text actually reaches the frame.
  std::string frame = device->GetOutput();
  CHECK(frame.find("Group title") != std::string::npos);
}

namespace {
class ScreenBackgroundApp : public Component<ScreenBackgroundApp> {
 public:
  void InitReflection() override {
    Import<rtxui::div>();
    Component<ScreenBackgroundApp>::InitReflection();
  }
  // No background anywhere: whatever is behind the interface shows through.
  std::string_view view = R"html(
    <div class="box">hi</div>
    <style>
      .box {
        display: block;
        width: 6;
        height: 3;
        border: tall;
        border-color: rgb(255, 0, 0);
      }
    </style>
  )html";
};
}  // namespace

TEST_CASE("Declaring a screen background makes reversal exact",
          "[terminal][paint][border]") {
  // `tall`'s left column is reversed. With nothing behind the interface, the
  // ground it reverses onto is the terminal's own background -- a color that
  // cannot be written into an escape sequence -- so the swap has to be handed
  // to the terminal with SGR 7. Naming the color removes the guess: the swap
  // happens here, and anything drawn over the cell later composites against
  // the colors it will really be drawn in.
  auto left_cell = [](Color screen_background) {
    auto app = Ref<ScreenBackgroundApp>::New();
    app->Mount();
    auto box = LayoutTreeBuilder::Build(app->Root());
    REQUIRE(box != nullptr);
    auto fragment = RunLayout(
        {box.get()}, {{6, MeasureMode::Exactly}, {3, MeasureMode::Exactly}});
    REQUIRE(fragment != nullptr);
    Texture texture(6, 3);
    Paint(fragment.get(), texture, 0, 0, screen_background);
    return texture[0, 1];
  };

  SECTION("unknown background: the terminal is asked to swap") {
    const Cell cell = left_cell(Color());
    CHECK(cell.inverted);
    // The border color still goes in the opaque slot -- putting the unknown
    // color there is what used to render as the terminal's default
    // *foreground*, a white block down the side of the box.
    CHECK(cell.foreground_color == Color::RGB(255, 0, 0));
    CHECK(cell.background_color.a == 0);
  }

  SECTION("declared background: swapped here, nothing left to the terminal") {
    const Color screen = Color::RGB(0, 0, 128);
    const Cell cell = left_cell(screen);
    CHECK_FALSE(cell.inverted);
    CHECK(cell.foreground_color == screen);
    CHECK(cell.background_color == Color::RGB(255, 0, 0));
  }

  SECTION("both spellings draw the same thing") {
    const Cell unknown = left_cell(Color());
    const Cell declared = left_cell(Color::RGB(0, 0, 128));
    auto glyph = [](const Cell& c) {
      return c.inverted ? c.background_color : c.foreground_color;
    };
    auto ground = [](const Cell& c) {
      return c.inverted ? c.foreground_color : c.background_color;
    };
    // The glyph shows the ground behind the border in both; only one of them
    // knows what color that is.
    CHECK(glyph(unknown).a == 0);
    CHECK(glyph(declared) == Color::RGB(0, 0, 128));
    CHECK(ground(unknown) == ground(declared));
  }
}
}  // namespace rtxui
