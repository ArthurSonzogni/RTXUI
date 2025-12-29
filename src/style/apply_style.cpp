#include "style/apply_style.hpp"

#include <optional>
#include <stdexcept>
#include <string>

#include "paint/color.hpp"

namespace rtxui {
namespace {

std::optional<Color> ParseColor(std::string_view value) {
  if (value == "red")
    return Color::RGB(255, 0, 0);
  if (value == "white")
    return Color::RGB(255, 255, 255);
  if (value == "blue")
    return Color::RGB(0, 0, 255);
  if (value == "yellow")
    return Color::RGB(255, 255, 0);
  if (value == "green")
    return Color::RGB(0, 255, 0);
  if (value == "black")
    return Color::RGB(0, 0, 0);
  return std::nullopt;
}

float StoF(std::string_view s) {
  std::string temp(s);
  return std::stof(temp);
}
int StoI(std::string_view s) {
  std::string temp(s);
  return std::stoi(temp);
}

Length ParseLength(std::string_view value) {
  if (value.back() == '%') {
    value.remove_suffix(1);
    return Length::Pct(StoF(value));
  }
  return Length::Cells(StoF(value));
}

}  // namespace

void ApplyStyle(ComputedStyle& style, const css::Declaration& declaration) {
  auto p = declaration.property;
  auto v = declaration.value;

  if (p == "background-color") {
    style.background_color = ParseColor(v);
    return;
  }

  if (p == "foreground-color") {
    style.foreground_color = ParseColor(v);
    return;
  }

  if (p == "margin") {
    int m = StoI(v);
    style.margin = {m, m, m, m};
    return;
  }

  if (p == "padding") {
    int p = StoI(v);
    style.padding = {p, p, p, p};
    return;
  }
  
  if (p == "border-width") {
    int bw = StoI(v);
    style.border = {bw, bw, bw, bw};
    return;
  }

  if (p == "flex-grow") {
    style.flex_grow = StoF(v);
    return;
  }

  if (p == "flex-direction") {
    if (v == "row") {
      style.flex_direction = Direction::Row;
      return;
    }

    if (v == "column") {
      style.flex_direction = Direction::Column;
      return;
    }
  }

  if (p == "width") {
    style.width = ParseLength(v);
    return;
  }

  if (p == "height") {
    style.height = ParseLength(v);
    return;
  }

  if (p == "display") {
    // Parse combined display property (display-outside and display-inside)
    // For simplicity, handle common single-keyword values and assume default display-inside: flow
    // For two-keyword values, parse them as specified.

    // Split the value string by space
    std::string s_value(v.data(), v.size());
    
    size_t space_pos = s_value.find(' ');
    if (space_pos == std::string::npos) {
      // Single keyword value
      if (s_value == "block") {
        style.display_outside = DisplayOutside::Block;
        style.display_inside = DisplayInside::Flow;
        return;
      }
      if (s_value == "inline") {
        style.display_outside = DisplayOutside::Inline;
        style.display_inside = DisplayInside::Flow;
        return;
      }
      if (s_value == "flex") {
        style.display_outside = DisplayOutside::Block;
        style.display_inside = DisplayInside::Flex;
        return;
      }

    } else {
      std::string outside = s_value.substr(0, space_pos);
      std::string inside = s_value.substr(space_pos + 1);

      if (outside == "block") {
        style.display_outside = DisplayOutside::Block;
      }

      if (outside == "inline") {
        style.display_outside = DisplayOutside::Inline;
      }

      if (inside == "flow") {
        style.display_inside = DisplayInside::Flow;
      }

      if (inside == "flex") {
        style.display_inside = DisplayInside::Flex;
      }
      return;
    }
  }

}

}  // namespace rtxui
