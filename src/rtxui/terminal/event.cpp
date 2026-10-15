// Copyright 2020 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/event.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

#include "rtxui/base/string.hpp"
#include "rtxui/terminal/event_internal.hpp"

// Disable warning for shadowing variable
#ifdef __clang__
#pragma clang diagnostic ignored "-Wshadow"
#elif __GNUC__
#pragma GCC diagnostic ignored "-Wshadow"
#endif

namespace rtxui {

// static
namespace {

constexpr std::uint32_t kAlt = 1 << 0;
constexpr std::uint32_t kCtrl = 1 << 1;
constexpr std::uint32_t kMeta = 1 << 2;
constexpr std::uint32_t kShift = 1 << 3;

}  // namespace

// Modifier -------------------------------------------------------------------

bool Event::Modifier::alt() const {
  return bits_ & kAlt;
}
bool Event::Modifier::ctrl() const {
  return bits_ & kCtrl;
}
bool Event::Modifier::meta() const {
  return bits_ & kMeta;
}
bool Event::Modifier::shift() const {
  return bits_ & kShift;
}

std::string Event::Modifier::Print() const {
  std::string out;
  for (auto [set, name] :
       {std::pair{alt(), "alt"}, std::pair{ctrl(), "ctrl"},
        std::pair{meta(), "meta"}, std::pair{shift(), "shift"}}) {
    if (!set) {
      continue;
    }
    if (!out.empty()) {
      out += "|";
    }
    out += name;
  }
  return out;
}

Event::Modifier EventInternals::Make(ModifierData data) {
  Event::Modifier modifier;
  modifier.bits_ = (data.alt ? kAlt : 0) | (data.ctrl ? kCtrl : 0) |
                   (data.meta ? kMeta : 0) | (data.shift ? kShift : 0);
  return modifier;
}

ModifierData EventInternals::Data(Event::Modifier modifier) {
  return {modifier.alt(), modifier.ctrl(), modifier.meta(), modifier.shift()};
}

// Keyboard -------------------------------------------------------------------

Event::Keyboard::Keyboard(std::shared_ptr<const KeyboardData> data)
    : data_(std::move(data)) {}

Event::Keyboard Event::Keyboard::From(std::uint32_t cp) {
  KeyboardData data;
  data.codepoint = cp;
  return Keyboard(std::make_shared<const KeyboardData>(data));
}

Event::Keyboard Event::Keyboard::From(char c) {
  return From(static_cast<std::uint32_t>(c));
}

Event::Keyboard Event::Keyboard::From(std::string_view str) {
  uint32_t cp = 0;
  size_t end = 0;
  EatCodePoint(str, 0, &end, &cp);
  return From(cp);
}

std::uint32_t Event::Keyboard::codepoint() const {
  return data_->codepoint;
}
Event::Keyboard::Special Event::Keyboard::special() const {
  return data_->special;
}
Event::Modifier Event::Keyboard::modifier() const {
  return EventInternals::Make(data_->modifier);
}
Event::Keyboard::Motion Event::Keyboard::motion() const {
  return data_->motion;
}
bool Event::Keyboard::from_paste() const {
  return data_->from_paste;
}
bool Event::Keyboard::operator==(const Keyboard& other) const {
  return *data_ == *other.data_;
}

std::string Event::Keyboard::Print() const {
  const KeyboardData& d = *data_;
  const Special special = d.special;
  if (special != Special::None) {
    static const auto& special_string = *new std::map<Special, std::string>{
        {Special::ArrowLeft, "ArrowLeft"},
        {Special::ArrowRight, "ArrowRight"},
        {Special::ArrowUp, "ArrowUp"},
        {Special::ArrowDown, "ArrowDown"},
        {Special::Backspace, "Backspace"},
        {Special::Delete, "Delete"},
        {Special::Escape, "Escape"},
        {Special::Return, "Return"},
        {Special::Tab, "Tab"},
        {Special::TabReverse, "TabReverse"},
        {Special::Insert, "Insert"},
        {Special::Home, "Home"},
        {Special::End, "End"},
        {Special::PageUp, "PageUp"},
        {Special::PageDown, "PageDown"},
        {Special::F1, "F1"},
        {Special::F2, "F2"},
        {Special::F3, "F3"},
        {Special::F4, "F4"},
        {Special::F5, "F5"},
        {Special::F6, "F6"},
        {Special::F7, "F7"},
        {Special::F8, "F8"},
        {Special::F9, "F9"},
        {Special::F10, "F10"},
        {Special::F11, "F11"},
        {Special::F12, "F12"},
    };
    return "Keyboard(" +
           ((std::map<Special, std::string>&)special_string)[special] + ")";
  }
  std::string out = "Keyboard(" + CodePointToString(d.codepoint);
  std::string mod = modifier().Print();
  if (!mod.empty()) {
    out += " " + mod;
  }
  out += ")";
  return out;
}

// Mouse ----------------------------------------------------------------------

Event::Mouse::Mouse(std::shared_ptr<const MouseData> data)
    : data_(std::move(data)) {}

Event::Mouse::Button Event::Mouse::button() const {
  return data_->button;
}
Event::Mouse::Motion Event::Mouse::motion() const {
  return data_->motion;
}
Event::Modifier Event::Mouse::modifier() const {
  return EventInternals::Make(data_->modifier);
}
int Event::Mouse::x() const {
  return data_->x;
}
int Event::Mouse::y() const {
  return data_->y;
}
float Event::Mouse::sub_cell_x() const {
  return data_->sub_cell_x;
}
float Event::Mouse::sub_cell_y() const {
  return data_->sub_cell_y;
}
int Event::Mouse::pixel_x() const {
  return data_->pixel_x;
}
int Event::Mouse::pixel_y() const {
  return data_->pixel_y;
}
bool Event::Mouse::is_pixel_precise() const {
  return data_->is_pixel_precise;
}
bool Event::Mouse::operator==(const Mouse& other) const {
  return *data_ == *other.data_;
}

std::string Event::Mouse::Print() const {
  const MouseData& d = *data_;
  static const auto& button_string = *new std::map<Mouse::Button, const char*>{
      {Mouse::Button::Left, "Left"},
      {Mouse::Button::Middle, "Middle"},
      {Mouse::Button::Right, "Right"},
      {Mouse::Button::WheelUp, "WheelUp"},
      {Mouse::Button::WheelDown, "WheelDown"},
      {Mouse::Button::None, "None"},
      {Mouse::Button::WheelLeft, "WheelLeft"},
      {Mouse::Button::WheelRight, "WheelRight"},
  };
  static const auto& motion_string = *new std::map<Mouse::Motion, const char*>{
      {Mouse::Motion::Pressed, "Pressed"},
      {Mouse::Motion::Released, "Released"},
      {Mouse::Motion::Moved, "Moved"},
  };
  std::string out =
      "Mouse(" +
      std::string(
          ((std::map<Mouse::Button, const char*>&)button_string)[d.button]);
  out += ", " +
         std::string(
             ((std::map<Mouse::Motion, const char*>&)motion_string)[d.motion]);
  out += ", x=" + std::to_string(d.x) + ", y=" + std::to_string(d.y);
  if (d.is_pixel_precise) {
    out += ", px=" + std::to_string(d.pixel_x) +
           ", py=" + std::to_string(d.pixel_y);
  }
  std::string mod = modifier().Print();
  if (!mod.empty()) {
    out += ", " + mod;
  }
  out += ")";
  return out;
}

// Resized --------------------------------------------------------------------

Event::Resized::Resized(std::shared_ptr<const ResizedData> data)
    : data_(std::move(data)) {}

int Event::Resized::width() const {
  return data_->width;
}
int Event::Resized::height() const {
  return data_->height;
}
bool Event::Resized::operator==(const Resized& other) const {
  return *data_ == *other.data_;
}

std::string Event::Resized::Print() const {
  return "Event::Resized(" + std::to_string(data_->width) + ", " +
         std::to_string(data_->height) + ")";
}

// Event ----------------------------------------------------------------------

Event::Event(std::shared_ptr<const internal::EventData> data)
    : data_(std::move(data)) {}
Event::Event(Keyboard keyboard)
    : Event(EventInternals::Make(*keyboard.data_)) {}
Event::Event(Mouse mouse) : Event(EventInternals::Make(*mouse.data_)) {}
Event::Event(Resized resized) : Event(EventInternals::Make(*resized.data_)) {}
Event::Event(const Event&) = default;
Event::Event(Event&&) noexcept = default;
Event& Event::operator=(const Event&) = default;
Event& Event::operator=(Event&&) noexcept = default;
Event::~Event() = default;

bool Event::IsKeyboard() const {
  return std::holds_alternative<KeyboardData>(data_->value);
}
bool Event::IsMouse() const {
  return std::holds_alternative<MouseData>(data_->value);
}
bool Event::IsResized() const {
  return std::holds_alternative<ResizedData>(data_->value);
}

// Each shares the event's data, pointing into it. Asked for the wrong kind,
// each gives an empty one rather than reading the wrong alternative.
Event::Keyboard Event::AsKeyboard() const {
  if (const auto* data = std::get_if<KeyboardData>(&data_->value)) {
    return Keyboard(std::shared_ptr<const KeyboardData>(data_, data));
  }
  return Keyboard(std::make_shared<const KeyboardData>());
}
Event::Mouse Event::AsMouse() const {
  if (const auto* data = std::get_if<MouseData>(&data_->value)) {
    return Mouse(std::shared_ptr<const MouseData>(data_, data));
  }
  return Mouse(std::make_shared<const MouseData>());
}
Event::Resized Event::AsResized() const {
  if (const auto* data = std::get_if<ResizedData>(&data_->value)) {
    return Resized(std::shared_ptr<const ResizedData>(data_, data));
  }
  return Resized(std::make_shared<const ResizedData>());
}

bool Event::operator==(const Event& other) const {
  return data_->value == other.data_->value;
}

std::string Event::Print() const {
  if (IsKeyboard()) {
    return AsKeyboard().Print();
  }
  if (IsMouse()) {
    return AsMouse().Print();
  }
  return AsResized().Print();
}

Event EventInternals::Make(KeyboardData data) {
  return Event(std::make_shared<const internal::EventData>(
      internal::EventData{std::move(data)}));
}
Event EventInternals::Make(MouseData data) {
  return Event(std::make_shared<const internal::EventData>(
      internal::EventData{std::move(data)}));
}
Event EventInternals::Make(ResizedData data) {
  return Event(std::make_shared<const internal::EventData>(
      internal::EventData{std::move(data)}));
}

const KeyboardData& EventInternals::Data(const Event::Keyboard& event) {
  return *event.data_;
}
const MouseData& EventInternals::Data(const Event::Mouse& event) {
  return *event.data_;
}
const ResizedData& EventInternals::Data(const Event::Resized& event) {
  return *event.data_;
}

namespace internal {
KeyboardData::operator Event() const {
  return EventInternals::Make(*this);
}
MouseData::operator Event() const {
  return EventInternals::Make(*this);
}
ResizedData::operator Event() const {
  return EventInternals::Make(*this);
}
}  // namespace internal

#define RTXUI_IMPL_EVENT(NAME, ...)                                    \
  const Event& Event::NAME() {                                         \
    static const Event& event = *new Event(KeyboardData{__VA_ARGS__}); \
    return event;                                                      \
  }

RTXUI_IMPL_EVENT(ArrowLeft, .special = Event::Keyboard::ArrowLeft)
RTXUI_IMPL_EVENT(ArrowRight, .special = Event::Keyboard::ArrowRight)
RTXUI_IMPL_EVENT(ArrowUp, .special = Event::Keyboard::ArrowUp)
RTXUI_IMPL_EVENT(ArrowDown, .special = Event::Keyboard::ArrowDown)
RTXUI_IMPL_EVENT(ArrowLeftCtrl,
                 .special = Event::Keyboard::ArrowLeft,
                 .modifier = {.ctrl = true})
RTXUI_IMPL_EVENT(ArrowRightCtrl,
                 .special = Event::Keyboard::ArrowRight,
                 .modifier = {.ctrl = true})
RTXUI_IMPL_EVENT(ArrowUpCtrl,
                 .special = Event::Keyboard::ArrowUp,
                 .modifier = {.ctrl = true})
RTXUI_IMPL_EVENT(ArrowDownCtrl,
                 .special = Event::Keyboard::ArrowDown,
                 .modifier = {.ctrl = true})
RTXUI_IMPL_EVENT(ArrowLeftAlt,
                 .special = Event::Keyboard::ArrowLeft,
                 .modifier = {.alt = true})
RTXUI_IMPL_EVENT(ArrowRightAlt,
                 .special = Event::Keyboard::ArrowRight,
                 .modifier = {.alt = true})
RTXUI_IMPL_EVENT(ArrowUpAlt,
                 .special = Event::Keyboard::ArrowUp,
                 .modifier = {.alt = true})
RTXUI_IMPL_EVENT(ArrowDownAlt,
                 .special = Event::Keyboard::ArrowDown,
                 .modifier = {.alt = true})
RTXUI_IMPL_EVENT(Backspace, .special = Event::Keyboard::Backspace)
RTXUI_IMPL_EVENT(BackspaceCtrl,
                 .special = Event::Keyboard::Backspace,
                 .modifier = {.ctrl = true})
RTXUI_IMPL_EVENT(BackspaceAlt,
                 .special = Event::Keyboard::Backspace,
                 .modifier = {.alt = true})
RTXUI_IMPL_EVENT(Delete, .special = Event::Keyboard::Delete)
RTXUI_IMPL_EVENT(DeleteCtrl,
                 .special = Event::Keyboard::Delete,
                 .modifier = {.ctrl = true})
RTXUI_IMPL_EVENT(DeleteAlt,
                 .special = Event::Keyboard::Delete,
                 .modifier = {.alt = true})
RTXUI_IMPL_EVENT(Escape, .special = Event::Keyboard::Escape)
RTXUI_IMPL_EVENT(Return, .special = Event::Keyboard::Return)
RTXUI_IMPL_EVENT(Tab, .special = Event::Keyboard::Tab)
RTXUI_IMPL_EVENT(TabReverse, .special = Event::Keyboard::TabReverse)
RTXUI_IMPL_EVENT(F1, .special = Event::Keyboard::F1)
RTXUI_IMPL_EVENT(F2, .special = Event::Keyboard::F2)
RTXUI_IMPL_EVENT(F3, .special = Event::Keyboard::F3)
RTXUI_IMPL_EVENT(F4, .special = Event::Keyboard::F4)
RTXUI_IMPL_EVENT(F5, .special = Event::Keyboard::F5)
RTXUI_IMPL_EVENT(F6, .special = Event::Keyboard::F6)
RTXUI_IMPL_EVENT(F7, .special = Event::Keyboard::F7)
RTXUI_IMPL_EVENT(F8, .special = Event::Keyboard::F8)
RTXUI_IMPL_EVENT(F9, .special = Event::Keyboard::F9)
RTXUI_IMPL_EVENT(F10, .special = Event::Keyboard::F10)
RTXUI_IMPL_EVENT(F11, .special = Event::Keyboard::F11)
RTXUI_IMPL_EVENT(F12, .special = Event::Keyboard::F12)
RTXUI_IMPL_EVENT(Insert, .special = Event::Keyboard::Insert)
RTXUI_IMPL_EVENT(Home, .special = Event::Keyboard::Home)
RTXUI_IMPL_EVENT(End, .special = Event::Keyboard::End)
RTXUI_IMPL_EVENT(PageUp, .special = Event::Keyboard::PageUp)
RTXUI_IMPL_EVENT(PageDown, .special = Event::Keyboard::PageDown)

// A terminal sends Shift+A as the character 'A' and reports no Shift, so
// that is what Event::A() must be to compare equal to the parsed key.
#define RTXUI_IMPL_LETTER(L, UC, CP)                                      \
  RTXUI_IMPL_EVENT(L, .codepoint = CP)                                    \
  RTXUI_IMPL_EVENT(UC, .codepoint = CP - 'a' + 'A')                       \
  RTXUI_IMPL_EVENT(Ctrl##UC, .codepoint = CP, .modifier = {.ctrl = true}) \
  RTXUI_IMPL_EVENT(Alt##UC, .codepoint = CP, .modifier = {.alt = true})   \
  RTXUI_IMPL_EVENT(CtrlAlt##UC, .codepoint = CP,                          \
                   .modifier = {.alt = true, .ctrl = true})

// clang-format off
RTXUI_IMPL_LETTER(a, A, 'a')
RTXUI_IMPL_LETTER(b, B, 'b')
RTXUI_IMPL_LETTER(c, C, 'c')
RTXUI_IMPL_LETTER(d, D, 'd')
RTXUI_IMPL_LETTER(e, E, 'e')
RTXUI_IMPL_LETTER(f, F, 'f')
RTXUI_IMPL_LETTER(g, G, 'g')
RTXUI_IMPL_LETTER(h, H, 'h')
RTXUI_IMPL_LETTER(i, I, 'i')
RTXUI_IMPL_LETTER(j, J, 'j')
RTXUI_IMPL_LETTER(k, K, 'k')
RTXUI_IMPL_LETTER(l, L, 'l')
RTXUI_IMPL_LETTER(m, M, 'm')
RTXUI_IMPL_LETTER(n, N, 'n')
RTXUI_IMPL_LETTER(o, O, 'o')
RTXUI_IMPL_LETTER(p, P, 'p')
RTXUI_IMPL_LETTER(q, Q, 'q')
RTXUI_IMPL_LETTER(r, R, 'r')
RTXUI_IMPL_LETTER(s, S, 's')
RTXUI_IMPL_LETTER(t, T, 't')
RTXUI_IMPL_LETTER(u, U, 'u')
RTXUI_IMPL_LETTER(v, V, 'v')
RTXUI_IMPL_LETTER(w, W, 'w')
RTXUI_IMPL_LETTER(x, X, 'x')
RTXUI_IMPL_LETTER(y, Y, 'y')
RTXUI_IMPL_LETTER(z, Z, 'z')
// clang-format on

}  // namespace rtxui
