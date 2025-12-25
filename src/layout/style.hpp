#ifndef RTXUI_LAYOUT_STYLE_HPP
#define RTXUI_LAYOUT_STYLE_HPP

#include <string>
#include <vector>
#include <optional>

#include "paint/color.hpp"

namespace rtxui {
enum class Display { Block, Inline, Flex, None };
enum class MeasureMode { Exactly, AtMost, Undefined };
enum class Direction { Row, Column };
enum class Unit { Auto, Cells, Percent };

struct Length {
  float value = 0;
  Unit unit = Unit::Auto;

  static Length Auto() { return {0, Unit::Auto}; }
  static Length Cells(float v) { return {v, Unit::Cells}; }
  static Length Pct(float v) { return {v, Unit::Percent}; }

  int Resolve(int basis) const {
    if (unit == Unit::Cells) {
      return static_cast<int>(value);
    }
    if (unit == Unit::Percent) {
      return static_cast<int>(basis * (value / 100.0f));
    }
    return 0;  // Auto resolves to 0 or handled by logic
  }
};

struct Spacing {
  int top = 0, right = 0, bottom = 0, left = 0;
  int Horiz() const { return left + right; }
  int Vert() const { return top + bottom; }
};

struct Constraint {
  int value = 0;
  MeasureMode mode = MeasureMode::Undefined;
};

struct LayoutConstraints {
  Constraint width;
  Constraint height;
};

// Represents the "Computed CSS values"
struct ComputedStyle {
  Display display = Display::Inline;
  Direction flex_direction = Direction::Row;

  Length width = Length::Auto();
  Length height = Length::Auto();

  float flex_grow = 0.0f;
  float flex_shrink = 1.0f;

  Spacing margin;
  Spacing padding;
  Spacing border;

  std::optional<Color> background_color;
  std::optional<Color> foreground_color;

  bool IsBlockLevel() const {
    return display == Display::Block || display == Display::Flex;
  }
  bool IsInlineLevel() const { return display == Display::Inline; }
};
}  // namespace rtxui
#endif  // RTXUI_LAYOUT_STYLE_HPP
