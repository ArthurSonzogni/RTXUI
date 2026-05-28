#ifndef RTXUI_LAYOUT_PHYSICAL_FRAGMENT_HPP
#define RTXUI_LAYOUT_PHYSICAL_FRAGMENT_HPP

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "rtxui/layout/style.hpp"
#include "rtxui/paint/color.hpp"

namespace rtxui {
class Element;

struct PhysicalFragment {
  Element* dom_node = nullptr;
  int x, y, width, height;
  int scroll_x = 0;
  int scroll_y = 0;
  bool clips_descendants = false;
  bool is_text = false;
  std::string text_content;
  bool has_border = false;
  BorderStyle border_style = BorderStyle::None;
  std::optional<Color> border_color_top;
  std::optional<Color> border_color_right;
  std::optional<Color> border_color_bottom;
  std::optional<Color> border_color_left;

  std::optional<Color> background_color;
  std::optional<Color> foreground_color;

  struct ChildLink {
    std::shared_ptr<PhysicalFragment> fragment;
    int x, y;
  };
  std::vector<ChildLink> children;

  PhysicalFragment(int w, int h) : width(w), height(h) {}
};
}  // namespace rtxui
#endif  // RTXUI_LAYOUT_PHYSICAL_FRAGMENT_HPP
