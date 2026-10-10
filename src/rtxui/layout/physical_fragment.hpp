#ifndef RTXUI_LAYOUT_PHYSICAL_FRAGMENT_HPP
#define RTXUI_LAYOUT_PHYSICAL_FRAGMENT_HPP

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "rtxui/color.hpp"
#include "rtxui/layout/layout_arena.hpp"
#include "rtxui/layout/style.hpp"

namespace rtxui {
class Element;

// The paint fields of a fragment that only animations can change between two
// layouts: see RefreshPaint().
enum PaintField : uint8_t {
  kPaintBackground = 1 << 0,
  kPaintForeground = 1 << 1,
  kPaintOpacity = 1 << 2,
  kPaintBorderColors = 1 << 3,
  kPaintAll = 0b1111,
};

struct PhysicalFragment {
  Element* dom_node = nullptr;
  // A fragment's position lives in its parent's ChildLink, not here: layout
  // never assigns x/y. They are zero-initialised so that reading them yields
  // something predictable rather than indeterminate memory, but callers that
  // need a position must take it from the ChildLink.
  int x = 0, y = 0;
  int width, height;
  int scroll_x = 0;
  int scroll_y = 0;
  float visual_scroll_x = 0.0f;
  float visual_scroll_y = 0.0f;
  bool clips_descendants = false;
  bool is_text = false;
  std::string_view text_content;
  bool has_border = false;
  // Per-side border widths, so paint.cpp can draw only the sides/corners
  // that are actually present instead of always drawing a full box outline
  // whenever `has_border` is set.
  Spacing border_width;
  BorderStyle border_style = BorderStyle::None;
  std::optional<Color> border_color_top;
  std::optional<Color> border_color_right;
  std::optional<Color> border_color_bottom;
  std::optional<Color> border_color_left;

  float opacity = 1.0f;
  std::optional<Color> background_color;
  std::optional<Color> foreground_color;
  std::optional<bool> bold;
  std::optional<bool> dim;
  std::optional<bool> italic;
  std::optional<bool> underlined;
  std::optional<bool> underlined_double;
  std::optional<bool> strikethrough;
  std::optional<bool> overlined;
  std::optional<bool> blink;
  Visibility visibility = Visibility::Visible;
  // The PaintField bits layout set from `dom_node`'s style.
  uint8_t paint_fields = 0;
  // How far ApplyTranslate() moved this fragment: included in its position in
  // its parent's ChildLink.
  int translated_x = 0, translated_y = 0;

  struct ChildLink {
    std::shared_ptr<PhysicalFragment> fragment;
    int x, y;
  };
  // Optimization: Use thread-local LayoutArenaAllocator to prevent heap churn.
  // Yields ~7% speedup in Layout/Paint (draw).
  std::vector<ChildLink, LayoutArenaAllocator<ChildLink>> children;

  PhysicalFragment(int w, int h) : width(w), height(h) {}
};
}  // namespace rtxui
#endif  // RTXUI_LAYOUT_PHYSICAL_FRAGMENT_HPP
