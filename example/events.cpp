// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Inspecting the input events: every key and mouse action the terminal sends.
//
// The root component overrides OnEvent(), which receives each event no focused
// element consumed. It reads each one as an Event::Keyboard or an
// Event::Mouse, shows the fields of the latest one, and lists the previous
// ones, newest first. It then passes the event on, so Escape and Ctrl+C still
// quit.
//
// Try it: type, press arrows and function keys, move the mouse, click, scroll.
#include <cstddef>
#include <format>
#include <rtxui/rtxui.hpp>
#include <string>
#include <string_view>
#include <vector>

using namespace rtxui;

struct Field {
  std::string name;
  std::string value;
  bool operator==(const Field&) const = default;
};

// What a template reads of a Field: {field.name} and {field.value}.
void Fields(const Field& field, FieldSink& out) {
  out("name", field.name);
  out("value", field.value);
}

struct Entry {
  int number = 0;
  std::string kind;  // "keyboard" or "mouse", also its CSS class.
  std::string text;
  bool mouse_moved = false;
  bool operator==(const Entry&) const = default;
};

// What a template reads of an Entry: {entry.number}, {entry.kind} and
// {entry.text}.
void Fields(const Entry& entry, FieldSink& out) {
  out("number", std::to_string(entry.number));
  out("kind", entry.kind);
  out("text", entry.text);
}

std::string YesNo(bool value) {
  return value ? "yes" : "no";
}

std::string OrNone(std::string text) {
  return text.empty() ? "none" : text;
}

std::vector<Field> KeyboardFields(const Event::Keyboard& key) {
  // In the order of Event::Keyboard::Special and Event::Keyboard::Motion.
  constexpr std::string_view kSpecial[] = {
      "none",       "ArrowLeft", "ArrowRight", "ArrowUp", "ArrowDown",
      "Backspace",  "Delete",    "Escape",     "Return",  "Tab",
      "TabReverse", "Insert",    "Home",       "End",     "PageUp",
      "PageDown",   "F1",        "F2",         "F3",      "F4",
      "F5",         "F6",        "F7",         "F8",      "F9",
      "F10",        "F11",       "F12",
  };
  constexpr std::string_view kMotion[] = {"Pressed", "Repeat", "Released"};
  return {
      {"codepoint", key.codepoint() == 0
                        ? "none"
                        : std::format("U+{:04X}", key.codepoint())},
      {"special", std::string(kSpecial[key.special()])},
      {"modifiers", OrNone(key.modifier().Print())},
      {"motion", std::string(kMotion[key.motion()])},
      {"from_paste", YesNo(key.from_paste())},
  };
}

std::vector<Field> MouseFields(const Event::Mouse& mouse) {
  // In the order of Event::Mouse::Button and Event::Mouse::Motion.
  constexpr std::string_view kButton[] = {
      "Left",    "Middle",    "Right",     "None",
      "WheelUp", "WheelDown", "WheelLeft", "WheelRight",
  };
  constexpr std::string_view kMotion[] = {"Released", "Pressed", "Moved"};
  return {
      {"button", std::string(kButton[mouse.button()])},
      {"motion", std::string(kMotion[mouse.motion()])},
      {"x, y", std::to_string(mouse.x()) + ", " + std::to_string(mouse.y())},
      {"modifiers", OrNone(mouse.modifier().Print())},
      {"pixel precise", YesNo(mouse.is_pixel_precise())},
  };
}

class Events : public Component<Events> {
 public:
  static constexpr size_t kMaxHistory = 50;

  std::string latest = "Waiting for an event...";
  std::vector<Field> fields;
  std::vector<Entry> history;
  int count = 0;

  Events() {
    Bind(latest);
    Bind(fields);
    Bind(history);
    Bind(count);
  }

  bool OnEvent(Event event) override {
    Entry entry{++count, "", event.Print()};
    if (event.is<Event::Keyboard>()) {
      entry.kind = "keyboard";
      fields = KeyboardFields(event.get<Event::Keyboard>());
    } else if (event.is<Event::Mouse>()) {
      const Event::Mouse mouse = event.get<Event::Mouse>();
      entry.kind = "mouse";
      entry.mouse_moved = mouse.motion() == Event::Mouse::Moved;
      fields = MouseFields(mouse);
    }
    latest = entry.text;

    // A moving mouse sends an event per cell: keep one row for a whole run of
    // them, updated in place, so they do not push everything else away.
    if (entry.mouse_moved && !history.empty() && history.front().mouse_moved) {
      history.front() = entry;
    } else {
      history.insert(history.begin(), entry);
      if (history.size() > kMaxHistory) {
        history.pop_back();
      }
    }

    // Let the default handling run too: Escape and Ctrl+C quit.
    return Component<Events>::OnEvent(event);
  }

  std::string_view view = R"html(
    <div class="latest">
      <h1>Latest event</h1>
      <p class="print">{latest}</p>
      <for each="{fields}" as="field">
        <div class="field">
          <span class="name">{field.name}</span>
          <span>{field.value}</span>
        </div>
      </for>
    </div>
    <div class="history">
      <h1>History ({count} events)</h1>
      <for each="{history}" as="entry">
        <div class="entry">
          <span class="number">{entry.number}</span>
          <span class="{entry.kind}">{entry.text}</span>
        </div>
      </for>
    </div>

    <style>
      self {
        display: flex;
        gap: 2;
        padding: 1 2;
        width: 100%;
        height: 100%;
        background-color: rgb(13, 17, 23);
        color: rgb(230, 237, 243);
      }
      .latest, .history {
        border: tall;
        border-color: rgb(48, 54, 61);
        background-color: rgb(22, 27, 34);
        padding: 0 1;
        overflow: hidden;
      }
      .latest {
        width: 36;
        flex-shrink: 0;
      }
      .history {
        flex-grow: 1;
      }
      h1 {
        color: rgb(88, 166, 255);
        font-weight: bold;
        margin-bottom: 1;
      }
      .print {
        font-weight: bold;
        margin-bottom: 1;
      }
      .field, .entry {
        display: flex;
        gap: 1;
      }
      .name {
        width: 14;
        flex-shrink: 0;
        color: rgb(139, 148, 158);
      }
      .number {
        width: 5;
        flex-shrink: 0;
        text-align: right;
        color: rgb(139, 148, 158);
      }
      .keyboard {
        color: rgb(126, 231, 135);
      }
      .mouse {
        color: rgb(210, 168, 255);
      }
    </style>
  )html";
};

int main() {
  auto app = Ref<Events>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
