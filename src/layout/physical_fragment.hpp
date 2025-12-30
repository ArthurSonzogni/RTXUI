#ifndef RTXUI_LAYOUT_PHYSICAL_FRAGMENT_HPP
#define RTXUI_LAYOUT_PHYSICAL_FRAGMENT_HPP

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "layout/style.hpp"
#include "paint/color.hpp"

namespace rtxui {
struct PhysicalFragment {
  int x, y, width, height;
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
