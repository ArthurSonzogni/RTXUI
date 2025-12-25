#ifndef RTXUI_LAYOUT_PHYSICAL_FRAGMENT_HPP
#define RTXUI_LAYOUT_PHYSICAL_FRAGMENT_HPP

#include <memory>
#include <string>
#include <vector>

#include "paint/color.hpp"

namespace rtxui {
struct PhysicalFragment {
  int x, y, width, height;
  std::string tag;
  bool is_text = false;
  std::string text_content;
  bool has_border = false;

  Color background_color;
  Color foreground_color;

  struct ChildLink {
    std::shared_ptr<PhysicalFragment> fragment;
    int x, y;
  };
  std::vector<ChildLink> children;

  PhysicalFragment(int w, int h, std::string t) : width(w), height(h), tag(t) {}
};
}  // namespace rtxui
#endif  // RTXUI_LAYOUT_PHYSICAL_FRAGMENT_HPP
