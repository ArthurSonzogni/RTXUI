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

#include "core/string.hpp"

/// @brief Represent an event. It can be key press event, a terminal resize, or
/// more ...
///
/// For example:
/// - Printable character can be created using Event::Keyboard::Char('a').
/// - Some special are predefined, like Event::ArrowLeft.
/// - One can find arbitrary code for special Events using:
///   ./example/util/print_key_press
///  For instance, CTLR+A maps to Event::CtrlA.
///
/// Useful documentation about xterm specification:
/// https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
struct Event {
  struct Keyboard {
    static Keyboard From(std::uint32_t cp);
    static Keyboard From(char c);
    static Keyboard From(std::string_view str);

    // For keys that are associated with a codepoint. 0 if not applicable.
    std::uint32_t codepoint = 0;

    // For keys that aren't associated with a codepoint. None if not applicable.
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

    bool alt : 1 = false;
    bool ctrl : 1 = false;
    bool meta : 1 = false;
    bool shift : 1 = false;

    enum Motion {
      Pressed,
      Repeat,
      Released,
    };
    Motion motion = Pressed;

    std::strong_ordering operator<=>(const Keyboard&) const = default;
  };

  struct Mouse {
    enum Button {
      Left = 0,
      Middle = 1,
      Right = 2,
      None = 3,
      WheelUp = 4,
      WheelDown = 5,
      WheelLeft = 6,   /// Supported terminal only.
      WheelRight = 7,  /// Supported terminal only.
    };

    enum Motion {
      Released = 0,
      Pressed = 1,
      Moved = 2,
    };

    // Button
    Button button = Button::None;

    // Motion
    Motion motion = Motion::Pressed;

    // Modifiers:
    bool shift = false;
    bool meta = false;
    bool control = false;

    // Coordinates:
    int x = 0;
    int y = 0;

    std::strong_ordering operator<=>(const Mouse&) const = default;
  };

  struct Resized {
    int width = 0;
    int height = 0;

    std::strong_ordering operator<=>(const Resized&) const = default;
  };

  struct CursorShape {
    int shape;

    std::strong_ordering operator<=>(const CursorShape&) const = default;
  };

  struct CursorPosition {
    int x;
    int y;

    std::strong_ordering operator<=>(const CursorPosition&) const = default;
  };

  struct Special {
    Special(std::string_view seq) : sequence(seq) {}
    Special(std::vector<int> bytes) {
      for (auto b : bytes) {
        sequence.push_back(static_cast<char>(b));
      }
    }
    std::string sequence;

    std::strong_ordering operator<=>(const Special&) const = default;
  };

  Event() = delete;
  Event(const Event& other) = default;
  Event(Event&& other) = default;
  Event& operator=(const Event& other) = default;
  Event& operator=(Event&& other) = default;

  template <typename T,
            typename = std::enable_if_t<
                !std::is_same_v<std::decay_t<T>, Event> &&
                std::is_constructible_v<
                    std::variant<Keyboard, Mouse, Resized, CursorShape,
                                 CursorPosition, Special>,
                    T>>>
  Event(T&& value) : data_(std::forward<T>(value)) {}

  // --- Arrow ---
  static const Event ArrowLeft;
  static const Event ArrowRight;
  static const Event ArrowUp;
  static const Event ArrowDown;

  static const Event ArrowLeftCtrl;
  static const Event ArrowRightCtrl;
  static const Event ArrowUpCtrl;
  static const Event ArrowDownCtrl;

  // --- ---
  static const Event Backspace;
  static const Event Delete;
  static const Event Return;
  static const Event Escape;
  static const Event Tab;
  static const Event TabReverse;

  // --- Navigation keys ---
  static const Event Insert;
  static const Event Home;
  static const Event End;
  static const Event PageUp;
  static const Event PageDown;

  // --- Function keys ---
  static const Event F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12;

  // --- Letter keys ---
  static const Event a, A, CtrlA, AltA, CtrlAltA;
  static const Event b, B, CtrlB, AltB, CtrlAltB;
  static const Event c, C, CtrlC, AltC, CtrlAltC;
  static const Event d, D, CtrlD, AltD, CtrlAltD;
  static const Event e, E, CtrlE, AltE, CtrlAltE;
  static const Event f, F, CtrlF, AltF, CtrlAltF;
  static const Event g, G, CtrlG, AltG, CtrlAltG;
  static const Event h, H, CtrlH, AltH, CtrlAltH;
  static const Event i, I, CtrlI, AltI, CtrlAltI;
  static const Event j, J, CtrlJ, AltJ, CtrlAltJ;
  static const Event k, K, CtrlK, AltK, CtrlAltK;
  static const Event l, L, CtrlL, AltL, CtrlAltL;
  static const Event m, M, CtrlM, AltM, CtrlAltM;
  static const Event n, N, CtrlN, AltN, CtrlAltN;
  static const Event o, O, CtrlO, AltO, CtrlAltO;
  static const Event p, P, CtrlP, AltP, CtrlAltP;
  static const Event q, Q, CtrlQ, AltQ, CtrlAltQ;
  static const Event r, R, CtrlR, AltR, CtrlAltR;
  static const Event s, S, CtrlS, AltS, CtrlAltS;
  static const Event t, T, CtrlT, AltT, CtrlAltT;
  static const Event u, U, CtrlU, AltU, CtrlAltU;
  static const Event v, V, CtrlV, AltV, CtrlAltV;
  static const Event w, W, CtrlW, AltW, CtrlAltW;
  static const Event x, X, CtrlX, AltX, CtrlAltX;
  static const Event y, Y, CtrlY, AltY, CtrlAltY;
  static const Event z, Z, CtrlZ, AltZ, CtrlAltZ;

  // Debug
  std::string DebugString() const;

  std::strong_ordering operator<=>(const Event&) const = default;

  template<typename T>
  bool is() const {
    return std::holds_alternative<T>(data_);
  }

  template<typename T>
  const T& get() const {
    return std::get<T>(data_);
  }

  template<typename T>
  T* get_if() {
    return std::get_if<T>(&data_);
  }

 private:
  std::variant<Keyboard, Mouse, Resized, CursorShape, CursorPosition, Special>
      data_;
};

inline std::ostream& operator<<(std::ostream& os, const Event& event) {
  os << event.DebugString();
  return os;
}


#endif /* end of include guard: RTXUI_TERMINAL_EVENT_HPP */
