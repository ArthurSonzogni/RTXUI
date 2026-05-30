// Copyright 2020 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_TERMINAL_EVENT_HPP
#define RTXUI_TERMINAL_EVENT_HPP

#include <cstdint>  // for uint32_t
#include <iostream>
#include <string>  // for string, operator==
#include <string_view>
#include <variant>  // for variant
#include <vector>

struct Event {
  struct Modifier {
    bool alt : 1 = false;
    bool ctrl : 1 = false;
    bool meta : 1 = false;
    bool shift : 1 = false;
    std::string Print() const;
    std::strong_ordering operator<=>(const Modifier&) const = default;
  };
  struct Keyboard {
    static Keyboard From(std::uint32_t cp);
    static Keyboard From(char c);
    static Keyboard From(std::string_view str);
    std::uint32_t codepoint = 0;
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
    Special special = None;
    Modifier modifier;
    enum Motion {
      Pressed,
      Repeat,
      Released,
    };
    Motion motion = Pressed;
    std::strong_ordering operator<=>(const Keyboard&) const = default;
    std::string Print() const;
  };
  struct Mouse {
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
    Button button = Button::None;
    Motion motion = Motion::Pressed;
    Modifier modifier;
    int x = 0, y = 0;
    std::strong_ordering operator<=>(const Mouse&) const = default;
    std::string Print() const;
  };
  struct Resized {
    int width = 0, height = 0;
    std::strong_ordering operator<=>(const Resized&) const = default;
    std::string Print() const;
  };
  struct CursorShape {
    int shape;
    std::strong_ordering operator<=>(const CursorShape&) const = default;
    std::string Print() const;
  };
  struct CursorPosition {
    int x, y;
    std::strong_ordering operator<=>(const CursorPosition&) const = default;
    std::string Print() const;
  };

  Event() = delete;
  Event(const Event& other) = default;
  Event(Event&& other) = default;
  Event& operator=(const Event& other) = default;
  Event& operator=(Event&& other) = default;

  template <typename T,
            typename = std::enable_if_t<
                !std::is_same_v<std::decay_t<T>, Event> &&
                std::is_constructible_v<std::variant<Keyboard,
                                                     Mouse,
                                                     Resized,
                                                     CursorShape,
                                                     CursorPosition>,
                                        T>>>
  Event(T&& value) : data_(std::forward<T>(value)) {}

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
  static const Event& CtrlAlt##UC();

  RTXUI_DECLARE_LETTER(a, A)
  RTXUI_DECLARE_LETTER(b, B) RTXUI_DECLARE_LETTER(c, C) RTXUI_DECLARE_LETTER(
      d,
      D) RTXUI_DECLARE_LETTER(e, E) RTXUI_DECLARE_LETTER(f, F)
      RTXUI_DECLARE_LETTER(g, G) RTXUI_DECLARE_LETTER(h, H)
          RTXUI_DECLARE_LETTER(i, I) RTXUI_DECLARE_LETTER(j, J)
              RTXUI_DECLARE_LETTER(k, K) RTXUI_DECLARE_LETTER(l, L)
                  RTXUI_DECLARE_LETTER(m, M) RTXUI_DECLARE_LETTER(n, N)
                      RTXUI_DECLARE_LETTER(o, O) RTXUI_DECLARE_LETTER(p, P)
                          RTXUI_DECLARE_LETTER(q, Q) RTXUI_DECLARE_LETTER(r, R)
                              RTXUI_DECLARE_LETTER(s, S) RTXUI_DECLARE_LETTER(t,
                                                                              T)
                                  RTXUI_DECLARE_LETTER(u, U)
                                      RTXUI_DECLARE_LETTER(v, V)
                                          RTXUI_DECLARE_LETTER(w, W)
                                              RTXUI_DECLARE_LETTER(x, X)
                                                  RTXUI_DECLARE_LETTER(y, Y)
                                                      RTXUI_DECLARE_LETTER(z, Z)

#undef RTXUI_DECLARE_LETTER

                                                          std::string
      Print() const;
  std::strong_ordering operator<=>(const Event&) const = default;
  template <typename T>
  bool is() const {
    return std::holds_alternative<T>(data_);
  }
  template <typename T>
  const T& get() const {
    return std::get<T>(data_);
  }
  template <typename T>
  T* get_if() {
    return std::get_if<T>(&data_);
  }

 private:
  std::variant<Keyboard, Mouse, Resized, CursorShape, CursorPosition> data_;
};

inline std::ostream& operator<<(std::ostream& os, const Event& event) {
  os << event.Print();
  return os;
}

#endif
