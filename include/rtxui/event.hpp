// Copyright 2020 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_EVENT_HPP_
#define RTXUI_EVENT_HPP_

#include <cstdint>
#include <memory>
#include <optional>
#include <ostream>
#include <rtxui/rtxui_export.hpp>
#include <string>
#include <string_view>
#include <type_traits>

namespace rtxui {

namespace internal {
struct EventData;
struct KeyboardData;
struct MouseData;
struct ResizedData;
}  // namespace internal

/// An input event: a key, a mouse action or a resize. Only the library and
/// the named events below create one; read it with is<T>() and get<T>():
///
///   if (event.is<Event::Keyboard>()) {
///     Event::Keyboard key = event.get<Event::Keyboard>();
///     if (key.special() == Event::Keyboard::Return) { ... }
///   }
///   if (event == Event::CtrlP()) { ... }
///
/// Copies share the same immutable data.
class RTXUI_EXPORT Event {
 public:
  /// The modifier keys held during an event.
  class RTXUI_EXPORT Modifier {
   public:
    Modifier() = default;
    bool alt() const;
    bool ctrl() const;
    bool meta() const;
    bool shift() const;
    std::string Print() const;
    bool operator==(const Modifier&) const = default;

   private:
    friend struct EventInternals;
    std::uint32_t bits_ = 0;
  };

  class RTXUI_EXPORT Keyboard {
   public:
    enum Special {
      None = 0,
      ArrowLeft,
      ArrowRight,
      ArrowUp,
      ArrowDown,
      Backspace,
      Delete,
      Escape,
      Return,
      Tab,
      TabReverse,
      Insert,
      Home,
      End,
      PageUp,
      PageDown,
      F1,
      F2,
      F3,
      F4,
      F5,
      F6,
      F7,
      F8,
      F9,
      F10,
      F11,
      F12,
    };
    enum Motion {
      Pressed,
      Repeat,
      Released,
    };

    /// The key typing `cp`, `c` or the first character of `str`.
    static Keyboard From(std::uint32_t cp);
    static Keyboard From(char c);
    static Keyboard From(std::string_view str);

    /// The character typed, or 0 for a special key.
    std::uint32_t codepoint() const;
    Special special() const;
    Modifier modifier() const;
    Motion motion() const;
    /// Whether the character comes from a paste rather than from typing.
    bool from_paste() const;
    std::string Print() const;
    bool operator==(const Keyboard&) const;

   private:
    friend class Event;
    friend struct EventInternals;
    explicit Keyboard(std::shared_ptr<const internal::KeyboardData> data);
    std::shared_ptr<const internal::KeyboardData> data_;
  };

  class RTXUI_EXPORT Mouse {
   public:
    enum Button {
      Left = 0,
      Middle = 1,
      Right = 2,
      None = 3,
      WheelUp = 4,
      WheelDown = 5,
      WheelLeft = 6,
      WheelRight = 7,
    };
    enum Motion {
      Released = 0,
      Pressed = 1,
      Moved = 2,
    };

    Button button() const;
    Motion motion() const;
    Modifier modifier() const;
    /// The cell under the pointer: 1-based column and row.
    int x() const;
    int y() const;
    /// Where in the cell, from 0 to 1, when the terminal reports pixels.
    float sub_cell_x() const;
    float sub_cell_y() const;
    /// The pixel, when the terminal reports pixels.
    int pixel_x() const;
    int pixel_y() const;
    bool is_pixel_precise() const;
    std::string Print() const;
    bool operator==(const Mouse&) const;

   private:
    friend class Event;
    friend struct EventInternals;
    explicit Mouse(std::shared_ptr<const internal::MouseData> data);
    std::shared_ptr<const internal::MouseData> data_;
  };

  class RTXUI_EXPORT Resized {
   public:
    int width() const;
    int height() const;
    std::string Print() const;
    bool operator==(const Resized&) const;

   private:
    friend class Event;
    friend struct EventInternals;
    explicit Resized(std::shared_ptr<const internal::ResizedData> data);
    std::shared_ptr<const internal::ResizedData> data_;
  };

  Event() = delete;
  Event(Keyboard keyboard);  // NOLINT(google-explicit-constructor): an Event is
                             // any of the three.
  Event(Mouse mouse);        // NOLINT(google-explicit-constructor): idem.
  Event(Resized resized);    // NOLINT(google-explicit-constructor): idem.
  Event(const Event&);
  Event(Event&&) noexcept;
  Event& operator=(const Event&);
  Event& operator=(Event&&) noexcept;
  ~Event();

  // --- Singleton Events ---
  static const Event& ArrowLeft();
  static const Event& ArrowRight();
  static const Event& ArrowUp();
  static const Event& ArrowDown();
  static const Event& ArrowLeftCtrl();
  static const Event& ArrowRightCtrl();
  static const Event& ArrowUpCtrl();
  static const Event& ArrowDownCtrl();
  static const Event& ArrowLeftAlt();
  static const Event& ArrowRightAlt();
  static const Event& ArrowUpAlt();
  static const Event& ArrowDownAlt();
  static const Event& Backspace();
  static const Event& BackspaceCtrl();
  static const Event& BackspaceAlt();
  static const Event& Delete();
  static const Event& DeleteCtrl();
  static const Event& DeleteAlt();
  static const Event& Return();
  static const Event& Escape();
  static const Event& Tab();
  static const Event& TabReverse();
  static const Event& Insert();
  static const Event& Home();
  static const Event& End();
  static const Event& PageUp();
  static const Event& PageDown();
  static const Event& F1();
  static const Event& F2();
  static const Event& F3();
  static const Event& F4();
  static const Event& F5();
  static const Event& F6();
  static const Event& F7();
  static const Event& F8();
  static const Event& F9();
  static const Event& F10();
  static const Event& F11();
  static const Event& F12();

#define RTXUI_DECLARE_LETTER(L, UC) \
  static const Event& L();          \
  static const Event& UC();         \
  static const Event& Ctrl##UC();   \
  static const Event& Alt##UC();    \
  static const Event& CtrlAlt##UC()

  RTXUI_DECLARE_LETTER(a, A);
  RTXUI_DECLARE_LETTER(b, B);
  RTXUI_DECLARE_LETTER(c, C);
  RTXUI_DECLARE_LETTER(d, D);
  RTXUI_DECLARE_LETTER(e, E);
  RTXUI_DECLARE_LETTER(f, F);
  RTXUI_DECLARE_LETTER(g, G);
  RTXUI_DECLARE_LETTER(h, H);
  RTXUI_DECLARE_LETTER(i, I);
  RTXUI_DECLARE_LETTER(j, J);
  RTXUI_DECLARE_LETTER(k, K);
  RTXUI_DECLARE_LETTER(l, L);
  RTXUI_DECLARE_LETTER(m, M);
  RTXUI_DECLARE_LETTER(n, N);
  RTXUI_DECLARE_LETTER(o, O);
  RTXUI_DECLARE_LETTER(p, P);
  RTXUI_DECLARE_LETTER(q, Q);
  RTXUI_DECLARE_LETTER(r, R);
  RTXUI_DECLARE_LETTER(s, S);
  RTXUI_DECLARE_LETTER(t, T);
  RTXUI_DECLARE_LETTER(u, U);
  RTXUI_DECLARE_LETTER(v, V);
  RTXUI_DECLARE_LETTER(w, W);
  RTXUI_DECLARE_LETTER(x, X);
  RTXUI_DECLARE_LETTER(y, Y);
  RTXUI_DECLARE_LETTER(z, Z);

#undef RTXUI_DECLARE_LETTER

  std::string Print() const;
  bool operator==(const Event&) const;

  /// Whether this is a T: Event::Keyboard, Event::Mouse or Event::Resized.
  template <typename T>
  bool is() const {
    if constexpr (std::is_same_v<T, Keyboard>) {
      return IsKeyboard();
    } else if constexpr (std::is_same_v<T, Mouse>) {
      return IsMouse();
    } else {
      static_assert(std::is_same_v<T, Resized>);
      return IsResized();
    }
  }
  /// The event as a T. Call it only when is<T>(): otherwise the T is empty.
  template <typename T>
  T get() const {
    if constexpr (std::is_same_v<T, Keyboard>) {
      return AsKeyboard();
    } else if constexpr (std::is_same_v<T, Mouse>) {
      return AsMouse();
    } else {
      static_assert(std::is_same_v<T, Resized>);
      return AsResized();
    }
  }
  /// The event as a T when it is one.
  template <typename T>
  std::optional<T> get_if() const {
    if (!is<T>()) {
      return std::nullopt;
    }
    return get<T>();
  }

 private:
  friend struct EventInternals;
  explicit Event(std::shared_ptr<const internal::EventData> data);
  bool IsKeyboard() const;
  bool IsMouse() const;
  bool IsResized() const;
  Keyboard AsKeyboard() const;
  Mouse AsMouse() const;
  Resized AsResized() const;

  std::shared_ptr<const internal::EventData> data_;
};

inline std::ostream& operator<<(std::ostream& os, const Event& event) {
  os << event.Print();
  return os;
}

}  // namespace rtxui

#endif
