#ifndef RTXUI_LAYOUT_STYLE_HPP
#define RTXUI_LAYOUT_STYLE_HPP

#include <cstddef>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "rtxui/paint/color.hpp"

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
  Grid,
};

enum class MeasureMode { Exactly, AtMost, Undefined };
enum class Direction { Row, RowReverse, Column, ColumnReverse };
enum class Unit { Auto, Cells, Percent, Fr };

struct Length {
  float value = 0;
  Unit unit = Unit::Auto;

  bool operator==(const Length&) const = default;

  static Length Auto() { return {0, Unit::Auto}; }
  static Length Cells(float v) { return {v, Unit::Cells}; }
  static Length Pct(float v) { return {v, Unit::Percent}; }
  static Length Fr(float v) { return {v, Unit::Fr}; }

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

enum class FlexWrap {
  NoWrap,
  Wrap,
  WrapReverse,
};

enum class WhiteSpace {
  Normal,
  Nowrap,
  Pre,
};

enum class TextTransform {
  None,
  Uppercase,
  Lowercase,
  Capitalize,
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

enum class AlignSelf {
  Auto,
  Stretch,
  FlexStart,
  FlexEnd,
  Center,
  Baseline,
};

enum class AlignContent {
  Stretch,
  FlexStart,
  FlexEnd,
  Center,
  SpaceBetween,
  SpaceAround,
  SpaceEvenly,
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
  FlexWrap flex_wrap = FlexWrap::NoWrap;

  Length width = Length::Auto();
  Length height = Length::Auto();
  Length min_width = Length::Auto();
  Length min_height = Length::Auto();
  Length max_width = Length::Auto();
  Length max_height = Length::Auto();
  bool margin_left_auto = false;
  bool margin_right_auto = false;
  bool margin_top_auto = false;
  bool margin_bottom_auto = false;

  float flex_grow = 0.0f;
  float flex_shrink = 1.0f;
  Length flex_basis = Length::Auto();
  Length row_gap = Length::Cells(0.0f);
  Length column_gap = Length::Cells(0.0f);
  JustifyContent justify_content = JustifyContent::FlexStart;
  AlignItems align_items = AlignItems::Stretch;
  AlignSelf align_self = AlignSelf::Auto;
  AlignContent align_content = AlignContent::Stretch;

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
  std::optional<bool> italic;
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
  std::optional<TextTransform> text_transform;
  std::optional<ListStyleType> list_style_type;
  int scroll_speed_x = 1;
  int scroll_speed_y = 1;
  ScrollBehavior scroll_behavior = ScrollBehavior::Auto;

  std::vector<Length> grid_template_columns;
  std::vector<Length> grid_template_rows;
  int grid_column_span = 1;
  int grid_row_span = 1;

  Cursor cursor = Cursor::Auto;
  Visibility visibility = Visibility::Visible;
  TextOverflow text_overflow = TextOverflow::Clip;

  ComputedStyle() = default;

  ComputedStyle(const ComputedStyle& other) {
    if (other.transitions) {
      transitions =
          std::make_unique<std::vector<TransitionConfig>>(*other.transitions);
    }
    CopyPOD(other);
  }

  ComputedStyle& operator=(const ComputedStyle& other) {
    if (this == &other) {
      return *this;
    }
    if (other.transitions) {
      if (transitions) {
        *transitions = *other.transitions;
      } else {
        transitions =
            std::make_unique<std::vector<TransitionConfig>>(*other.transitions);
      }
    } else {
      transitions.reset();
    }
    CopyPOD(other);
    return *this;
  }

  ComputedStyle(ComputedStyle&&) noexcept = default;
  ComputedStyle& operator=(ComputedStyle&&) noexcept = default;

  bool IsBlockLevel() const { return display_outside == DisplayOutside::Block; }
  bool IsInlineLevel() const {
    return display_outside == DisplayOutside::Inline;
  }

 private:
  void CopyPOD(const ComputedStyle& other) {
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
    flex_wrap = other.flex_wrap;
    width = other.width;
    height = other.height;
    min_width = other.min_width;
    min_height = other.min_height;
    max_width = other.max_width;
    max_height = other.max_height;
    margin_left_auto = other.margin_left_auto;
    margin_right_auto = other.margin_right_auto;
    margin_top_auto = other.margin_top_auto;
    margin_bottom_auto = other.margin_bottom_auto;
    flex_grow = other.flex_grow;
    flex_shrink = other.flex_shrink;
    flex_basis = other.flex_basis;
    row_gap = other.row_gap;
    column_gap = other.column_gap;
    justify_content = other.justify_content;
    align_items = other.align_items;
    align_self = other.align_self;
    align_content = other.align_content;
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
    italic = other.italic;
    underlined = other.underlined;
    underlined_double = other.underlined_double;
    strikethrough = other.strikethrough;
    blink = other.blink;
    overflow_x = other.overflow_x;
    overflow_y = other.overflow_y;
    scrollbar_width = other.scrollbar_width;
    has_scrollbar_color_thumb = other.has_scrollbar_color_thumb;
    has_scrollbar_color_track = other.has_scrollbar_color_track;
    scrollbar_color_thumb = other.scrollbar_color_thumb;
    scrollbar_color_track = other.scrollbar_color_track;
    text_align = other.text_align;
    white_space = other.white_space;
    text_transform = other.text_transform;
    list_style_type = other.list_style_type;
    scroll_speed_x = other.scroll_speed_x;
    scroll_speed_y = other.scroll_speed_y;
    scroll_behavior = other.scroll_behavior;
    cursor = other.cursor;
    visibility = other.visibility;
    text_overflow = other.text_overflow;
    grid_template_columns = other.grid_template_columns;
    grid_template_rows = other.grid_template_rows;
    grid_column_span = other.grid_column_span;
    grid_row_span = other.grid_row_span;
  }
};
}  // namespace rtxui
#endif  // RTXUI_LAYOUT_STYLE_HPP
