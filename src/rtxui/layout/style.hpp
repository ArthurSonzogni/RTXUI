#ifndef RTXUI_LAYOUT_STYLE_HPP
#define RTXUI_LAYOUT_STYLE_HPP

#include <optional>
#include <string>
#include <vector>

#include "rtxui/paint/color.hpp"

namespace rtxui {
enum class DisplayOutside {
  Block,
  Inline,
};
enum class DisplayInside {
  FlowRoot,
  Flow,
  Flex,
};

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

enum class BorderStyle {
  Ascii,
  Blank,
  Dashed,
  Double,
  HKey,
  Heavy,
  Inner,
  None,
  Outer,
  Panel,
  Round,
  Solid,
  Tall,
  Thick,
  VKey,
  Wide,
  Dotted,
  DoubleHorizontal,
  DoubleVertical,
  Shadow,
  ShadeLight,
  ShadeMedium,
  ShadeDark,
  Squiggle,
};

enum class Overflow {
  Visible,
  Hidden,
  Scroll,
};

enum class ScrollbarWidth {
  Auto,
  None,
};

enum class TextAlign {
  Left,
  Right,
  Center,
};

enum class WhiteSpace {
  Normal,
  Nowrap,
};

enum class PositionType {
  Static,
  Relative,
  Absolute,
  Fixed,
};

struct TransitionConfig {
  std::string property;
  float duration_seconds = 0.0f;
  float delay_seconds = 0.0f;
  std::string timing_function = "ease";
};

// Represents the "Computed CSS values"
struct ComputedStyle {
  std::vector<TransitionConfig> transitions;

  PositionType position = PositionType::Static;
  Length top = Length::Auto();
  Length right = Length::Auto();
  Length bottom = Length::Auto();
  Length left = Length::Auto();
  std::optional<int> z_index;

  DisplayOutside display_outside = DisplayOutside::Inline;
  DisplayInside display_inside = DisplayInside::Flow;  // Default to flow
  bool display_none =
      false;  // true when display: none — element takes no space

  Direction flex_direction = Direction::Row;

  Length width = Length::Auto();
  Length height = Length::Auto();

  float flex_grow = 0.0f;
  float flex_shrink = 1.0f;

  Spacing margin;
  Spacing padding;
  Spacing border;

  BorderStyle border_style = BorderStyle::None;
  std::optional<Color> border_color_top;
  std::optional<Color> border_color_right;
  std::optional<Color> border_color_bottom;
  std::optional<Color> border_color_left;

  std::optional<Color> background_color;
  std::optional<Color> foreground_color;
  std::optional<bool> bold;
  std::optional<bool> underlined;
  std::optional<bool> underlined_double;
  std::optional<bool> strikethrough;
  std::optional<bool> blink;

  Overflow overflow_x = Overflow::Visible;
  Overflow overflow_y = Overflow::Visible;
  ScrollbarWidth scrollbar_width = ScrollbarWidth::Auto;
  std::optional<TextAlign> text_align;
  std::optional<WhiteSpace> white_space;
  int scroll_speed_x = 1;
  int scroll_speed_y = 1;

  bool IsBlockLevel() const { return display_outside == DisplayOutside::Block; }
  bool IsInlineLevel() const {
    return display_outside == DisplayOutside::Inline;
  }
};
}  // namespace rtxui
#endif  // RTXUI_LAYOUT_STYLE_HPP
