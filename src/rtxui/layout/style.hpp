#ifndef RTXUI_LAYOUT_STYLE_HPP
#define RTXUI_LAYOUT_STYLE_HPP

#include <optional>
#include <string>
#include <vector>
#include <cstring>
#include <cstddef>

#include "rtxui/paint/color.hpp"
#include <memory>

namespace rtxui {
enum class DisplayOutside {
  Block,
  Inline,
};
enum class ListStyleType {
  Disc,
  Circle,
  Square,
  Decimal,
  None,
};
enum class DisplayInside {
  FlowRoot,
  Flow,
  Flex,
};

enum class MeasureMode { Exactly, AtMost, Undefined };
enum class Direction { Row, RowReverse, Column, ColumnReverse };
enum class Unit { Auto, Cells, Percent };

struct Length {
  float value = 0;
  Unit unit = Unit::Auto;

  bool operator==(const Length&) const = default;

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
  bool operator==(const Spacing&) const = default;
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
  Sticky,
};

enum class JustifyContent {
  FlexStart,
  FlexEnd,
  Center,
  SpaceBetween,
  SpaceAround,
  SpaceEvenly,
};

enum class AlignItems {
  Stretch,
  FlexStart,
  FlexEnd,
  Center,
  Baseline,
};

enum class Cursor {
  Auto,
  Default,
  Pointer,
  Text,
  Wait,
  Help,
};

enum class Visibility {
  Visible,
  Hidden,
};

enum class TextOverflow {
  Clip,
  Ellipsis,
};

struct TransitionConfig {
  std::string property;
  float duration_seconds = 0.0f;
  float delay_seconds = 0.0f;
  std::string timing_function = "ease";
  bool operator==(const TransitionConfig&) const = default;
};

// Represents the "Computed CSS values"
struct ComputedStyle {
  // Optimization: Wrapping transitions in unique_ptr avoids heap allocation
  // and destruction overhead (std::vector<TransitionConfig> with std::string
  // members) for the vast majority of elements which have no transitions.
  // Each Element has 3 ComputedStyle copies (style, base_style, target_style),
  // so this eliminates ~40% of CPU time spent in Element::~Element().
  std::unique_ptr<std::vector<TransitionConfig>> transitions;

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
  Length min_width = Length::Auto();
  Length min_height = Length::Auto();
  Length max_width = Length::Auto();
  Length max_height = Length::Auto();
  bool margin_left_auto = false;
  bool margin_right_auto = false;

  float flex_grow = 0.0f;
  float flex_shrink = 1.0f;
  Length gap = Length::Cells(0.0f);
  JustifyContent justify_content = JustifyContent::FlexStart;
  AlignItems align_items = AlignItems::Stretch;

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
  bool has_scrollbar_color_thumb = true;
  bool has_scrollbar_color_track = true;
  Color scrollbar_color_thumb = Color::RGBA(200, 200, 200, 200);
  Color scrollbar_color_track = Color::RGBA(80, 80, 80, 120);
  std::optional<TextAlign> text_align;
  std::optional<WhiteSpace> white_space;
  std::optional<ListStyleType> list_style_type;
  int scroll_speed_x = 1;
  int scroll_speed_y = 1;
  ScrollBehavior scroll_behavior = ScrollBehavior::Auto;

  Cursor cursor = Cursor::Auto;
  Visibility visibility = Visibility::Visible;
  TextOverflow text_overflow = TextOverflow::Clip;

  ComputedStyle() = default;

  ComputedStyle(const ComputedStyle& other) {
    if (other.transitions) {
      transitions = std::make_unique<std::vector<TransitionConfig>>(*other.transitions);
    }
    char* dst = reinterpret_cast<char*>(this) + offsetof(ComputedStyle, position);
    const char* src = reinterpret_cast<const char*>(&other) + offsetof(ComputedStyle, position);
    size_t size = sizeof(ComputedStyle) - offsetof(ComputedStyle, position);
    std::memcpy(dst, src, size);
  }

  ComputedStyle& operator=(const ComputedStyle& other) {
    if (this == &other) return *this;
    if (other.transitions) {
      if (transitions) {
        *transitions = *other.transitions;
      } else {
        transitions = std::make_unique<std::vector<TransitionConfig>>(*other.transitions);
      }
    } else {
      transitions.reset();
    }
    char* dst = reinterpret_cast<char*>(this) + offsetof(ComputedStyle, position);
    const char* src = reinterpret_cast<const char*>(&other) + offsetof(ComputedStyle, position);
    size_t size = sizeof(ComputedStyle) - offsetof(ComputedStyle, position);
    std::memcpy(dst, src, size);
    return *this;
  }

  ComputedStyle(ComputedStyle&&) noexcept = default;
  ComputedStyle& operator=(ComputedStyle&&) noexcept = default;

  bool IsBlockLevel() const { return display_outside == DisplayOutside::Block; }
  bool IsInlineLevel() const {
    return display_outside == DisplayOutside::Inline;
  }
};
}  // namespace rtxui
#endif  // RTXUI_LAYOUT_STYLE_HPP
