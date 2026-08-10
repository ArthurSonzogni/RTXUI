#ifndef RTXUI_LAYOUT_STYLE_HPP
#define RTXUI_LAYOUT_STYLE_HPP

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
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

enum class BoxSizing {
  ContentBox,
  BorderBox,
};

enum class MeasureMode { Exactly, AtMost, Undefined };
enum class Direction { Row, RowReverse, Column, ColumnReverse };
enum class Unit { Auto, Cells, Percent, Fr, Calc, MinMax };

struct MinMaxExpr;
/// Returns the interned expression for an id (see RegisterMinMaxExpr).
MinMaxExpr GetMinMaxExpr(int id);

// A min()/max()/clamp() expression over linear (cells + percent) operands.
// Instances are interned in a global table (see RegisterMinMaxExpr) and
// referenced from Length by id, keeping Length small for the common case.
// An operand may additionally reference another interned expression
// (`coef * Evaluate(registry[ref])`), which is how min()/max()/clamp() nest
// inside each other and inside calc(). References always point at earlier
// registry entries, so evaluation cannot cycle.
struct MinMaxExpr {
  enum class Op { Min, Max, Clamp };
  Op op = Op::Min;
  // Min/Max use a and b; Clamp is clamp(lo=a, preferred=b, hi=c).
  float a_cells = 0, a_percent = 0;
  float b_cells = 0, b_percent = 0;
  float c_cells = 0, c_percent = 0;
  int a_ref = -1, b_ref = -1, c_ref = -1;
  float a_ref_coef = 0, b_ref_coef = 0, c_ref_coef = 0;

  bool operator==(const MinMaxExpr&) const = default;

  bool DependsOnBasis() const {
    // Referenced expressions always depend on the basis: constant ones are
    // folded before registration.
    return a_percent != 0 || b_percent != 0 || c_percent != 0 ||
           a_ref != -1 || b_ref != -1 || c_ref != -1;
  }

  int Evaluate(int basis) const {
    auto lin = [basis](float cells, float percent, int ref, float ref_coef) {
      float v = cells + basis * (percent / 100.0f);
      if (ref != -1) {
        v += ref_coef * GetMinMaxExpr(ref).Evaluate(basis);
      }
      return v;
    };
    float a = lin(a_cells, a_percent, a_ref, a_ref_coef);
    float b = lin(b_cells, b_percent, b_ref, b_ref_coef);
    switch (op) {
      case Op::Min:
        return static_cast<int>(std::min(a, b));
      case Op::Max:
        return static_cast<int>(std::max(a, b));
      case Op::Clamp:
        return static_cast<int>(
            std::max(a, std::min(b, lin(c_cells, c_percent, c_ref, c_ref_coef))));
    }
    return 0;
  }
};

/// Interns the expression and returns its id (stable for the process
/// lifetime; identical expressions share an id).
int RegisterMinMaxExpr(const MinMaxExpr& expr);

struct Length {
  float value = 0;
  Unit unit = Unit::Auto;
  // Percent component for Unit::Calc. calc() expressions are folded at parse
  // time into the linear form `value + calc_percent% of basis`, which keeps
  // Length trivially copyable. For Unit::MinMax this is the interned
  // expression id (see RegisterMinMaxExpr), stored as float exactly.
  float calc_percent = 0;

  bool operator==(const Length&) const = default;

  static Length Auto() { return {0, Unit::Auto}; }
  static Length Cells(float v) { return {v, Unit::Cells}; }
  static Length Pct(float v) { return {v, Unit::Percent}; }
  static Length Fr(float v) { return {v, Unit::Fr}; }
  static Length MakeCalc(float cells, float percent) {
    return {cells, Unit::Calc, percent};
  }
  static Length MakeMinMax(int id) {
    return {static_cast<float>(id), Unit::MinMax};
  }

  int Resolve(int basis) const {
    if (unit == Unit::Cells) {
      return static_cast<int>(value);
    }
    if (unit == Unit::Percent) {
      return static_cast<int>(basis * (value / 100.0f));
    }
    if (unit == Unit::Calc) {
      return static_cast<int>(value + basis * (calc_percent / 100.0f));
    }
    if (unit == Unit::MinMax) {
      return GetMinMaxExpr(static_cast<int>(value)).Evaluate(basis);
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
  Justify,
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
  // Like Normal (this engine already preserves interior whitespace and
  // honors newlines), kept distinct for CSS fidelity.
  PreWrap,
  // Collapses runs of spaces/tabs to a single space, honors newlines, wraps.
  PreLine,
};

enum class TextTransform {
  None,
  Uppercase,
  Lowercase,
  Capitalize,
};

// How words longer than the line are handled. Unlike CSS, the default is
// Anywhere (emergency-break at the container edge): overflowing the box is
// rarely what a terminal UI wants.
enum class OverflowWrap {
  Anywhere,
  Normal,
};

// Whether normal (non-emergency) line breaking may occur mid-word. Unlike
// overflow-wrap (a last-resort fallback when a word would otherwise
// overflow), break-all treats every character boundary as a break
// opportunity, so words wrap as soon as they'd cross the line even when
// pushing them whole to the next line would have fit.
enum class WordBreak {
  Normal,
  BreakAll,
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

// The trivially-copyable part of ComputedStyle, kept in its own base so that
// copying it compiles down to a single memcpy instead of ~100 individual field
// assignments. Style copies are one of the hottest operations in a frame: every
// Element holds three ComputedStyle members (style, base_style, target_style)
// and Element::TriggerTransitions() copies one of them per element per frame.
struct ComputedStyleCore {
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

  // RTXUI's width/height have always described the border-box (outer) size
  // — unlike web CSS, whose initial value is content-box — so BorderBox is
  // the default here to keep every pre-existing layout unchanged.
  BoxSizing box_sizing = BoxSizing::BorderBox;
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
  // Flex item ordering. Lower values are laid out first; items sharing a
  // value keep document order. May be negative.
  int order = 0;
  Length row_gap = Length::Cells(0.0f);
  Length column_gap = Length::Cells(0.0f);
  JustifyContent justify_content = JustifyContent::FlexStart;
  AlignItems align_items = AlignItems::Stretch;
  AlignSelf align_self = AlignSelf::Auto;
  // Inline-axis alignment inside grid cells (justify-items / justify-self).
  AlignItems justify_items = AlignItems::Stretch;
  AlignSelf justify_self = AlignSelf::Auto;
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
  std::optional<bool> dim;
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
  // Extra cells inserted between grapheme clusters; inherited.
  std::optional<int> letter_spacing;
  // Minimum rows each line box occupies; inherited. 1 means normal.
  std::optional<int> line_height;
  std::optional<OverflowWrap> overflow_wrap;
  std::optional<WordBreak> word_break;
  // Width / height ratio in cells; 0 means auto (no preferred ratio).
  float aspect_ratio = 0;
  std::optional<ListStyleType> list_style_type;
  int scroll_speed_x = 1;
  int scroll_speed_y = 1;
  ScrollBehavior scroll_behavior = ScrollBehavior::Auto;

  int grid_column_span = 1;
  int grid_row_span = 1;

  Cursor cursor = Cursor::Auto;
  Visibility visibility = Visibility::Visible;
  TextOverflow text_overflow = TextOverflow::Clip;

  bool IsBlockLevel() const { return display_outside == DisplayOutside::Block; }
  bool IsInlineLevel() const {
    return display_outside == DisplayOutside::Inline;
  }
};

static_assert(std::is_trivially_copyable_v<ComputedStyleCore>,
              "ComputedStyleCore must stay trivially copyable: copying it is a "
              "per-element, per-frame hot path that relies on being a memcpy. "
              "Add fields needing a non-trivial copy (containers, strings, "
              "smart pointers) to ComputedStyle instead.");

// Represents the "Computed CSS values"
struct ComputedStyle : ComputedStyleCore {
  // Optimization: Wrapping transitions in unique_ptr avoids heap allocation
  // and destruction overhead (std::vector<TransitionConfig> with std::string
  // members) for the vast majority of elements which have no transitions.
  // Each Element has 3 ComputedStyle copies (style, base_style, target_style),
  // so this eliminates ~40% of CPU time spent in Element::~Element().
  std::unique_ptr<std::vector<TransitionConfig>> transitions;

  // Grid tracks only exist on grid containers, a small minority of elements.
  // They live here rather than in ComputedStyleCore so that the core stays
  // trivially copyable; copying an empty vector costs far less than the field
  // by field copy their presence in the core would force on every element.
  std::vector<Length> grid_template_columns;
  std::vector<Length> grid_template_rows;

  ComputedStyle() = default;

  ComputedStyle(const ComputedStyle& other)
      : ComputedStyleCore(other),
        grid_template_columns(other.grid_template_columns),
        grid_template_rows(other.grid_template_rows) {
    if (other.transitions) {
      transitions =
          std::make_unique<std::vector<TransitionConfig>>(*other.transitions);
    }
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
    grid_template_columns = other.grid_template_columns;
    grid_template_rows = other.grid_template_rows;
    ComputedStyleCore::operator=(other);
    return *this;
  }

  ComputedStyle(ComputedStyle&&) noexcept = default;
  ComputedStyle& operator=(ComputedStyle&&) noexcept = default;
};

}  // namespace rtxui
#endif  // RTXUI_LAYOUT_STYLE_HPP
