#include "style/apply_style.hpp"

#include <optional>
#include <stdexcept>
#include <string>

#include "paint/color.hpp"

namespace rtxui {
namespace {

float StoF(std::string_view s) {
  std::string temp(s);
  return std::stof(temp);
}
int StoI(std::string_view s) {
  try {
    std::string temp(s);
    return std::stoi(temp);
  } catch (...) {
    return 0;
  }
}

std::optional<Color> ParseColor(std::string_view value) {
  // Parse rgb(r, g, b)
  if (value.substr(0, 4) == "rgb(" && value.back() == ')') {
    value.remove_prefix(4);
    value.remove_suffix(1);
    size_t first_comma = value.find(',');
    if (first_comma == std::string_view::npos)
      return std::nullopt;
    size_t second_comma = value.find(',', first_comma + 1);
    if (second_comma == std::string_view::npos)
      return std::nullopt;
    std::string_view r_str = value.substr(0, first_comma);
    std::string_view g_str = value.substr(first_comma + 1, second_comma - first_comma - 1);
    std::string_view b_str = value.substr(second_comma + 1);
    try {
      int r = StoI(r_str);
      int g = StoI(g_str);
      int b = StoI(b_str);
      return Color::RGB(r, g, b);
    } catch (const std::invalid_argument&) {
      return std::nullopt;
    } catch (const std::out_of_range&) {
      return std::nullopt;
    }
  }

  // Parse rgba(r, g, b, a)
  if (value.substr(0, 5) == "rgba(" && value.back() == ')') {
    value.remove_prefix(5);
    value.remove_suffix(1);
    size_t first_comma = value.find(',');
    if (first_comma == std::string_view::npos)
      return std::nullopt;
    size_t second_comma = value.find(',', first_comma + 1);
    if (second_comma == std::string_view::npos)
      return std::nullopt;
    size_t third_comma = value.find(',', second_comma + 1);
    if (third_comma == std::string_view::npos)
      return std::nullopt;
    std::string_view r_str = value.substr(0, first_comma);
    std::string_view g_str = value.substr(first_comma + 1, second_comma - first_comma - 1);
    std::string_view b_str = value.substr(second_comma + 1, third_comma - second_comma - 1);
    std::string_view a_str = value.substr(third_comma + 1);
    try {
      int r = StoI(r_str);
      int g = StoI(g_str);
      int b = StoI(b_str);
      float a = StoF(a_str);
      return Color::RGBA(r, g, b, a);
    } catch (const std::invalid_argument&) {
      return std::nullopt;
    } catch (const std::out_of_range&) {
      return std::nullopt;
    }
  }

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

Length ParseLength(std::string_view value) {
  if (value.back() == '%') {
    value.remove_suffix(1);
    return Length::Pct(StoF(value));
  }
  return Length::Cells(StoF(value));
}

std::optional<BorderStyle> ParseBorderStyle(std::string_view v) {
  if (v == "none")
    return BorderStyle::None;
  if (v == "ascii")
    return BorderStyle::Ascii;
  if (v == "blank")
    return BorderStyle::Blank;
  if (v == "dashed")
    return BorderStyle::Dashed;
  if (v == "double")
    return BorderStyle::Double;
  if (v == "heavy")
    return BorderStyle::Heavy;
  if (v == "hkey")
    return BorderStyle::HKey;
  if (v == "inner")
    return BorderStyle::Inner;
  if (v == "outer")
    return BorderStyle::Outer;
  if (v == "panel")
    return BorderStyle::Panel;
  if (v == "round" || v == "rounded")
    return BorderStyle::Round;
  if (v == "solid")
    return BorderStyle::Solid;
  if (v == "tall")
    return BorderStyle::Tall;
  if (v == "thick")
    return BorderStyle::Thick;
  if (v == "vkey")
    return BorderStyle::VKey;
  if (v == "wide")
    return BorderStyle::Wide;
  return std::nullopt;
}

}  // namespace

void ApplyStyle(ComputedStyle& style, const css::Declaration& declaration) {
  auto p = declaration.property;
  auto v = declaration.value;

  if (p == "background-color") {
    style.background_color = ParseColor(v);
    return;
  }

  if (p == "color" || p == "foreground-color") {
    style.foreground_color = ParseColor(v);
    return;
  }

  if (p == "margin") {
    int m = StoI(v);
    style.margin = {m, m, m, m};
    return;
  }

  if (p == "margin-top") {
    int m = StoI(v);
    style.margin.top = m;
    return;
  }

  if (p == "margin-bottom") {
    int m = StoI(v);
    style.margin.bottom = m;
    return;
  }

  if (p == "margin-left") {
    int m = StoI(v);
    style.margin.left = m;
    return;
  }

  if (p == "margin-right") {
    int m = StoI(v);
    style.margin.right = m;
    return;
  }

  if (p == "padding") {
    int p = StoI(v);
    style.padding = {p, p, p, p};
    return;
  }

  if (p == "padding-top") {
    int p = StoI(v);
    style.padding.top = p;
    return;
  }

  if (p == "padding-bottom") {
    int p = StoI(v);
    style.padding.bottom = p;
    return;
  }

  if (p == "padding-left") {
    int p = StoI(v);
    style.padding.left = p;
    return;
  }

  if (p == "padding-right") {
    int p = StoI(v);
    style.padding.right = p;
    return;
  }
  
  if (p == "border-width") {
    int bw = StoI(v);
    style.border = {bw, bw, bw, bw};
    return;
  }

  if (p == "border") {
    if (auto style_opt = ParseBorderStyle(v)) {
      style.border_style = *style_opt;
      if (style.border.top == 0 && style.border.bottom == 0 &&
          style.border.left == 0 && style.border.right == 0) {
        style.border = {1, 1, 1, 1};
      }
      return;
    }
    // Try to parse as a number for width
    try {
      int bw = StoI(v);
      style.border = {bw, bw, bw, bw};
      style.border_style = BorderStyle::Solid;
      return;  // Return only on success
    } catch (const std::invalid_argument&) {
      // Not a number, fall through to other properties or do nothing
    } catch (const std::out_of_range&) {
      // Out of range for int.
    }
    return;
  }
  
  if (p == "border-top") {
    int bw = StoI(v);
    style.border.top = bw;
    return;
  }

  if (p == "border-bottom") {
    int bw = StoI(v);
    style.border.bottom = bw;
    return;
  }

  if (p == "border-left") {
    int bw = StoI(v);
    style.border.left = bw;
    return;
  }

  if (p == "border-right") {
    int bw = StoI(v);
    style.border.right = bw;
    return;
  }

  if (p == "border-style") {
    if (auto style_opt = ParseBorderStyle(v)) {
      style.border_style = *style_opt;
    }
    return;
  }

  if (p == "border-color") {
    auto color = ParseColor(v);
    style.border_color_top = color;
    style.border_color_right = color;
    style.border_color_bottom = color;
    style.border_color_left = color;
    return;
  }

  if (p == "border-color-top") {
    style.border_color_top = ParseColor(v);
    return;
  }

  if (p == "border-color-right") {
    style.border_color_right = ParseColor(v);
    return;
  }

  if (p == "border-color-bottom") {
    style.border_color_bottom = ParseColor(v);
    return;
  }

  if (p == "border-color-left") {
    style.border_color_left = ParseColor(v);
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
