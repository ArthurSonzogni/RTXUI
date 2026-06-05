#ifndef RTXUI_LAYOUT_PHYSICAL_FRAGMENT_HPP
#define RTXUI_LAYOUT_PHYSICAL_FRAGMENT_HPP

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "rtxui/layout/layout_arena.hpp"

#include "rtxui/layout/style.hpp"
#include "rtxui/paint/color.hpp"

namespace rtxui {
class Element;

struct PhysicalFragment {
  Element* dom_node = nullptr;
  int x, y, width, height;
  int scroll_x = 0;
  int scroll_y = 0;
  float visual_scroll_x = 0.0f;
  float visual_scroll_y = 0.0f;
  bool clips_descendants = false;
  bool is_text = false;
  std::string_view text_content;
  bool has_border = false;
  BorderStyle border_style = BorderStyle::None;
  std::optional<Color> border_color_top;
  std::optional<Color> border_color_right;
  std::optional<Color> border_color_bottom;
  std::optional<Color> border_color_left;

  float opacity = 1.0f;
  std::optional<Color> background_color;
  std::optional<Color> foreground_color;
  std::optional<bool> bold;
  std::optional<bool> underlined;
  std::optional<bool> underlined_double;
  std::optional<bool> strikethrough;
  std::optional<bool> blink;
  Visibility visibility = Visibility::Visible;

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
