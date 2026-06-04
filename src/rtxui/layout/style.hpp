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

enum class ScrollBehavior {
  Auto,
  Smooth,
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
  Length max_width = Length::Auto();
  Length max_height = Length::Auto();
  bool margin_left_auto = false;
  bool margin_right_auto = false;

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
  float opacity = 1.0f;
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
  ScrollBehavior scroll_behavior = ScrollBehavior::Auto;

  ComputedStyle() = default;

  ComputedStyle(const ComputedStyle& other) {
    if (!other.transitions.empty()) {
      transitions = other.transitions;
    }
    position = other.position;
    top = other.top;
    right = other.right;
    bottom = other.bottom;
    left = other.left;
    z_index = other.z_index;
    display_outside = other.display_outside;
    display_inside = other.display_inside;
    display_none = other.display_none;
    flex_direction = other.flex_direction;
    width = other.width;
    height = other.height;
    max_width = other.max_width;
    max_height = other.max_height;
    margin_left_auto = other.margin_left_auto;
    margin_right_auto = other.margin_right_auto;
    flex_grow = other.flex_grow;
    flex_shrink = other.flex_shrink;
    margin = other.margin;
    padding = other.padding;
    border = other.border;
    border_style = other.border_style;
    border_color_top = other.border_color_top;
    border_color_right = other.border_color_right;
    border_color_bottom = other.border_color_bottom;
    border_color_left = other.border_color_left;
    background_color = other.background_color;
    foreground_color = other.foreground_color;
    opacity = other.opacity;
    bold = other.bold;
    underlined = other.underlined;
    underlined_double = other.underlined_double;
    strikethrough = other.strikethrough;
    blink = other.blink;
    overflow_x = other.overflow_x;
    overflow_y = other.overflow_y;
    scrollbar_width = other.scrollbar_width;
    text_align = other.text_align;
    white_space = other.white_space;
    scroll_speed_x = other.scroll_speed_x;
    scroll_speed_y = other.scroll_speed_y;
    scroll_behavior = other.scroll_behavior;
  }

  ComputedStyle& operator=(const ComputedStyle& other) {
    if (this == &other) return *this;
    if (!transitions.empty() || !other.transitions.empty()) {
      transitions = other.transitions;
    }
    position = other.position;
    top = other.top;
    right = other.right;
    bottom = other.bottom;
    left = other.left;
    z_index = other.z_index;
    display_outside = other.display_outside;
    display_inside = other.display_inside;
    display_none = other.display_none;
    flex_direction = other.flex_direction;
    width = other.width;
    height = other.height;
    max_width = other.max_width;
    max_height = other.max_height;
    margin_left_auto = other.margin_left_auto;
    margin_right_auto = other.margin_right_auto;
    flex_grow = other.flex_grow;
    flex_shrink = other.flex_shrink;
    margin = other.margin;
    padding = other.padding;
    border = other.border;
    border_style = other.border_style;
    border_color_top = other.border_color_top;
    border_color_right = other.border_color_right;
    border_color_bottom = other.border_color_bottom;
    border_color_left = other.border_color_left;
    background_color = other.background_color;
    foreground_color = other.foreground_color;
    opacity = other.opacity;
    bold = other.bold;
    underlined = other.underlined;
    underlined_double = other.underlined_double;
    strikethrough = other.strikethrough;
    blink = other.blink;
    overflow_x = other.overflow_x;
    overflow_y = other.overflow_y;
    scrollbar_width = other.scrollbar_width;
    text_align = other.text_align;
    white_space = other.white_space;
    scroll_speed_x = other.scroll_speed_x;
    scroll_speed_y = other.scroll_speed_y;
    scroll_behavior = other.scroll_behavior;
    return *this;
  }

  bool IsBlockLevel() const { return display_outside == DisplayOutside::Block; }
  bool IsInlineLevel() const {
    return display_outside == DisplayOutside::Inline;
  }
};
}  // namespace rtxui
#endif  // RTXUI_LAYOUT_STYLE_HPP
