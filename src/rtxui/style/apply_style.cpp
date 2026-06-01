#include "rtxui/style/apply_style.hpp"

#include <cctype>
#include <charconv>
#include <cstdlib>
#include <optional>
#include <stdexcept>
#include <string>

#include "rtxui/paint/color.hpp"

namespace rtxui {
namespace {

float StoF(std::string_view s) {
  if (s.empty()) {
    return 0.0f;
  }
  std::string temp(s);
  char* endptr = nullptr;
  float val = std::strtof(temp.c_str(), &endptr);
  if (endptr == temp.c_str()) {
    return 0.0f;
  }
  return val;
}
int StoI(std::string_view s) {
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
    s.remove_prefix(1);
  }
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
    s.remove_suffix(1);
  }
  int value = 0;
  auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
  if (ec == std::errc()) {
    return value;
  }
  return 0;
}

std::vector<std::string_view> SplitWords(std::string_view s) {
  std::vector<std::string_view> words;
  while (!s.empty()) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
      s.remove_prefix(1);
    }
    if (s.empty()) break;
    size_t end = 0;
    while (end < s.size() && !std::isspace(static_cast<unsigned char>(s[end]))) {
      ++end;
    }
    words.push_back(s.substr(0, end));
    s.remove_prefix(end);
  }
  return words;
}

std::optional<Color> ParseColor(std::string_view value) {
  if (value.empty()) {
    return std::nullopt;
  }

  // Parse hex colors: #RGB, #RGBA, #RRGGBB, #RRGGBBAA
  if (value.front() == '#') {
    std::string_view hex = value.substr(1);
    auto hex_val = [](char c) -> std::optional<uint8_t> {
      if (c >= '0' && c <= '9') {
        return c - '0';
      }
      if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
      }
      if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
      }
      return std::nullopt;
    };

    if (hex.length() == 3 || hex.length() == 4) {
      auto r_digit = hex_val(hex[0]);
      auto g_digit = hex_val(hex[1]);
      auto b_digit = hex_val(hex[2]);
      auto a_digit =
          (hex.length() == 4) ? hex_val(hex[3]) : std::optional<uint8_t>(15);
      if (r_digit && g_digit && b_digit && a_digit) {
        uint8_t r = (*r_digit << 4) | *r_digit;
        uint8_t g = (*g_digit << 4) | *g_digit;
        uint8_t b = (*b_digit << 4) | *b_digit;
        uint8_t a = (*a_digit << 4) | *a_digit;
        return Color::RGBA(r, g, b, a);
      }
    } else if (hex.length() == 6 || hex.length() == 8) {
      bool valid = true;
      uint8_t vals[8] = {0};
      for (size_t i = 0; i < hex.length(); ++i) {
        if (auto digit = hex_val(hex[i])) {
          vals[i] = *digit;
        } else {
          valid = false;
          break;
        }
      }
      if (valid) {
        uint8_t r = (vals[0] << 4) | vals[1];
        uint8_t g = (vals[2] << 4) | vals[3];
        uint8_t b = (vals[4] << 4) | vals[5];
        uint8_t a = (hex.length() == 8) ? ((vals[6] << 4) | vals[7]) : 255;
        return Color::RGBA(r, g, b, a);
      }
    }
    return std::nullopt;
  }

  // Parse rgb(r, g, b)
  if (value.substr(0, 4) == "rgb(" && value.back() == ')') {
    value.remove_prefix(4);
    value.remove_suffix(1);
    size_t first_comma = value.find(',');
    if (first_comma == std::string_view::npos) {
      return std::nullopt;
    }
    size_t second_comma = value.find(',', first_comma + 1);
    if (second_comma == std::string_view::npos) {
      return std::nullopt;
    }
    std::string_view r_str = value.substr(0, first_comma);
    std::string_view g_str =
        value.substr(first_comma + 1, second_comma - first_comma - 1);
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
    if (first_comma == std::string_view::npos) {
      return std::nullopt;
    }
    size_t second_comma = value.find(',', first_comma + 1);
    if (second_comma == std::string_view::npos) {
      return std::nullopt;
    }
    size_t third_comma = value.find(',', second_comma + 1);
    if (third_comma == std::string_view::npos) {
      return std::nullopt;
    }
    std::string_view r_str = value.substr(0, first_comma);
    std::string_view g_str =
        value.substr(first_comma + 1, second_comma - first_comma - 1);
    std::string_view b_str =
        value.substr(second_comma + 1, third_comma - second_comma - 1);
    std::string_view a_str = value.substr(third_comma + 1);
    try {
      int r = StoI(r_str);
      int g = StoI(g_str);
      int b = StoI(b_str);
      float a = StoF(a_str);
      return Color::RGBA(r, g, b, a * 255.f);
    } catch (const std::invalid_argument&) {
      return std::nullopt;
    } catch (const std::out_of_range&) {
      return std::nullopt;
    }
  }

  if (value == "red") {
    return Color::RGB(255, 0, 0);
  }
  if (value == "white") {
    return Color::RGB(255, 255, 255);
  }
  if (value == "blue") {
    return Color::RGB(0, 0, 255);
  }
  if (value == "yellow") {
    return Color::RGB(255, 255, 0);
  }
  if (value == "green" || value == "lime") {
    return Color::RGB(0, 255, 0);
  }
  if (value == "black") {
    return Color::RGB(0, 0, 0);
  }
  if (value == "gray" || value == "grey") {
    return Color::RGB(128, 128, 128);
  }
  if (value == "cyan" || value == "aqua") {
    return Color::RGB(0, 255, 255);
  }
  if (value == "magenta" || value == "fuchsia") {
    return Color::RGB(255, 0, 255);
  }
  if (value == "silver") {
    return Color::RGB(192, 192, 192);
  }
  if (value == "maroon") {
    return Color::RGB(128, 0, 0);
  }
  if (value == "purple") {
    return Color::RGB(128, 0, 128);
  }
  if (value == "olive") {
    return Color::RGB(128, 128, 0);
  }
  if (value == "navy") {
    return Color::RGB(0, 0, 128);
  }
  if (value == "teal") {
    return Color::RGB(0, 128, 128);
  }
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
  if (v == "none") {
    return BorderStyle::None;
  }
  if (v == "ascii") {
    return BorderStyle::Ascii;
  }
  if (v == "blank") {
    return BorderStyle::Blank;
  }
  if (v == "dashed") {
    return BorderStyle::Dashed;
  }
  if (v == "double") {
    return BorderStyle::Double;
  }
  if (v == "heavy") {
    return BorderStyle::Heavy;
  }
  if (v == "hkey") {
    return BorderStyle::HKey;
  }
  if (v == "inner") {
    return BorderStyle::Inner;
  }
  if (v == "outer") {
    return BorderStyle::Outer;
  }
  if (v == "panel") {
    return BorderStyle::Panel;
  }
  if (v == "round" || v == "rounded") {
    return BorderStyle::Round;
  }
  if (v == "solid") {
    return BorderStyle::Solid;
  }
  if (v == "tall") {
    return BorderStyle::Tall;
  }
  if (v == "thick") {
    return BorderStyle::Thick;
  }
  if (v == "vkey") {
    return BorderStyle::VKey;
  }
  if (v == "wide") {
    return BorderStyle::Wide;
  }
  if (v == "dotted") {
    return BorderStyle::Dotted;
  }
  if (v == "double-horizontal") {
    return BorderStyle::DoubleHorizontal;
  }
  if (v == "double-vertical") {
    return BorderStyle::DoubleVertical;
  }
  if (v == "shadow" || v == "3d") {
    return BorderStyle::Shadow;
  }
  if (v == "shade-light") {
    return BorderStyle::ShadeLight;
  }
  if (v == "shade-medium") {
    return BorderStyle::ShadeMedium;
  }
  if (v == "shade-dark") {
    return BorderStyle::ShadeDark;
  }
  if (v == "squiggle" || v == "wave") {
    return BorderStyle::Squiggle;
  }
  return std::nullopt;
}

std::optional<Overflow> ParseOverflow(std::string_view v) {
  if (v == "visible") {
    return Overflow::Visible;
  }
  if (v == "hidden") {
    return Overflow::Hidden;
  }
  if (v == "scroll" || v == "auto") {
    return Overflow::Scroll;
  }
  return std::nullopt;
}

std::optional<ScrollbarWidth> ParseScrollbarWidth(std::string_view v) {
  if (v == "auto") {
    return ScrollbarWidth::Auto;
  }
  if (v == "none") {
    return ScrollbarWidth::None;
  }
  return std::nullopt;
}

}  // namespace

void ApplyStyle(ComputedStyle& style, const css::Declaration& declaration) {
  auto p = declaration.property;
  auto v = declaration.value;

  if (p == "transition") {
    std::string_view value_view = v;
    style.transitions.clear();
    while (!value_view.empty()) {
      size_t comma = value_view.find(',');
      std::string_view token = (comma == std::string_view::npos) ? value_view : value_view.substr(0, comma);
      while (!token.empty() && std::isspace(static_cast<unsigned char>(token.front()))) {
        token.remove_prefix(1);
      }
      while (!token.empty() && std::isspace(static_cast<unsigned char>(token.back()))) {
        token.remove_suffix(1);
      }
      if (!token.empty()) {
        size_t space1 = token.find(' ');
        if (space1 != std::string_view::npos) {
          std::string_view prop_name = token.substr(0, space1);
          std::string_view remaining = token.substr(space1 + 1);
          while (!remaining.empty() && std::isspace(static_cast<unsigned char>(remaining.front()))) {
            remaining.remove_prefix(1);
          }
          size_t space2 = remaining.find(' ');
          std::string_view dur_str = (space2 == std::string_view::npos) ? remaining : remaining.substr(0, space2);
          float dur = 0.0f;
          if (dur_str.ends_with("ms")) {
            dur = StoF(dur_str.substr(0, dur_str.size() - 2)) / 1000.0f;
          } else if (dur_str.ends_with("s")) {
            dur = StoF(dur_str.substr(0, dur_str.size() - 1));
          } else {
            dur = StoF(dur_str);
          }
          std::string_view timing = "ease";
          if (space2 != std::string_view::npos) {
            std::string_view remaining2 = remaining.substr(space2 + 1);
            while (!remaining2.empty() && std::isspace(static_cast<unsigned char>(remaining2.front()))) {
              remaining2.remove_prefix(1);
            }
            size_t space3 = remaining2.find(' ');
            timing = (space3 == std::string_view::npos) ? remaining2 : remaining2.substr(0, space3);
          }
          style.transitions.push_back({std::string(prop_name), dur, 0.0f, std::string(timing)});
        }
      }
      if (comma == std::string_view::npos) {
        break;
      }
      value_view = value_view.substr(comma + 1);
    }
    return;
  }

  if (p == "background-color") {
    style.background_color = ParseColor(v);
    return;
  }

  if (p == "color" || p == "foreground-color") {
    style.foreground_color = ParseColor(v);
    return;
  }

  if (p == "font-weight") {
    style.bold = (v == "bold");
    return;
  }

  if (p == "text-decoration") {
    if (v == "none") {
      style.underlined = false;
      style.underlined_double = false;
      style.strikethrough = false;
      style.blink = false;
      return;
    }
    style.underlined = ((v.find("underline") != std::string_view::npos ||
                         v.find("underlined") != std::string_view::npos) &&
                        v.find("double") == std::string_view::npos);
    style.underlined_double = (v.find("double-underline") != std::string_view::npos ||
                               v.find("underlined-double") != std::string_view::npos ||
                               ((v.find("underline") != std::string_view::npos ||
                                 v.find("underlined") != std::string_view::npos) &&
                                v.find("double") != std::string_view::npos));
    style.strikethrough = (v.find("line-through") != std::string_view::npos ||
                           v.find("strikethrough") != std::string_view::npos);
    style.blink = (v.find("blink") != std::string_view::npos);
    return;
  }

  if (p == "margin") {
    auto parts = SplitWords(v);
    if (parts.size() == 1) {
      if (parts[0] == "auto") {
        style.margin_left_auto = true;
        style.margin_right_auto = true;
        style.margin = {0, 0, 0, 0};
      } else {
        int m = StoI(parts[0]);
        style.margin_left_auto = false;
        style.margin_right_auto = false;
        style.margin = {m, m, m, m};
      }
    } else if (parts.size() == 2) {
      int v_val = StoI(parts[0]);
      style.margin.top = v_val;
      style.margin.bottom = v_val;
      if (parts[1] == "auto") {
        style.margin_left_auto = true;
        style.margin_right_auto = true;
        style.margin.left = 0;
        style.margin.right = 0;
      } else {
        int h_val = StoI(parts[1]);
        style.margin_left_auto = false;
        style.margin_right_auto = false;
        style.margin.left = h_val;
        style.margin.right = h_val;
      }
    } else if (parts.size() == 3) {
      style.margin.top = StoI(parts[0]);
      style.margin.bottom = StoI(parts[2]);
      if (parts[1] == "auto") {
        style.margin_left_auto = true;
        style.margin_right_auto = true;
        style.margin.left = 0;
        style.margin.right = 0;
      } else {
        int h_val = StoI(parts[1]);
        style.margin_left_auto = false;
        style.margin_right_auto = false;
        style.margin.left = h_val;
        style.margin.right = h_val;
      }
    } else if (parts.size() >= 4) {
      style.margin.top = StoI(parts[0]);
      style.margin.bottom = StoI(parts[2]);
      if (parts[1] == "auto") {
        style.margin_right_auto = true;
        style.margin.right = 0;
      } else {
        style.margin_right_auto = false;
        style.margin.right = StoI(parts[1]);
      }
      if (parts[3] == "auto") {
        style.margin_left_auto = true;
        style.margin.left = 0;
      } else {
        style.margin_left_auto = false;
        style.margin.left = StoI(parts[3]);
      }
    }
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
    if (v == "auto") {
      style.margin_left_auto = true;
      style.margin.left = 0;
    } else {
      style.margin_left_auto = false;
      style.margin.left = StoI(v);
    }
    return;
  }

  if (p == "margin-right") {
    if (v == "auto") {
      style.margin_right_auto = true;
      style.margin.right = 0;
    } else {
      style.margin_right_auto = false;
      style.margin.right = StoI(v);
    }
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

  if (p == "position") {
    if (v == "static") {
      style.position = PositionType::Static;
      return;
    }
    if (v == "relative") {
      style.position = PositionType::Relative;
      return;
    }
    if (v == "absolute") {
      style.position = PositionType::Absolute;
      return;
    }
    if (v == "fixed") {
      style.position = PositionType::Fixed;
      return;
    }
  }

  if (p == "top") {
    style.top = ParseLength(v);
    return;
  }
  if (p == "right") {
    style.right = ParseLength(v);
    return;
  }
  if (p == "bottom") {
    style.bottom = ParseLength(v);
    return;
  }
  if (p == "left") {
    style.left = ParseLength(v);
    return;
  }

  if (p == "z-index") {
    if (v == "auto") {
      style.z_index = std::nullopt;
    } else {
      style.z_index = StoI(v);
    }
    return;
  }

  if (p == "width") {
    style.width = ParseLength(v);
    return;
  }

  if (p == "max-width") {
    style.max_width = ParseLength(v);
    return;
  }

  if (p == "max-height") {
    style.max_height = ParseLength(v);
    return;
  }

  if (p == "height") {
    style.height = ParseLength(v);
    return;
  }

  if (p == "text-align") {
    if (v == "left") {
      style.text_align = TextAlign::Left;
      return;
    }
    if (v == "right") {
      style.text_align = TextAlign::Right;
      return;
    }
    if (v == "center") {
      style.text_align = TextAlign::Center;
      return;
    }
  }

  if (p == "white-space") {
    if (v == "normal") {
      style.white_space = WhiteSpace::Normal;
      return;
    }
    if (v == "nowrap") {
      style.white_space = WhiteSpace::Nowrap;
      return;
    }
  }

  if (p == "display") {
    // Parse combined display property (display-outside and display-inside)
    // For simplicity, handle common single-keyword values and assume default
    // display-inside: flow For two-keyword values, parse them as specified.

    // Split the value string by space
    std::string s_value(v.data(), v.size());

    size_t space_pos = s_value.find(' ');
    if (space_pos == std::string::npos) {
      // Single keyword value
      if (s_value == "none") {
        style.display_none = true;
        return;
      }
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
      if (s_value == "inline-block") {
        style.display_outside = DisplayOutside::Inline;
        style.display_inside = DisplayInside::Flow;
        return;
      }
      if (s_value == "inline-flex") {
        style.display_outside = DisplayOutside::Inline;
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

  if (p == "overflow") {
    if (auto o = ParseOverflow(v)) {
      style.overflow_x = *o;
      style.overflow_y = *o;
    }
    return;
  }

  if (p == "overflow-x") {
    if (auto o = ParseOverflow(v)) {
      style.overflow_x = *o;
    }
    return;
  }

  if (p == "overflow-y") {
    if (auto o = ParseOverflow(v)) {
      style.overflow_y = *o;
    }
    return;
  }

  if (p == "scrollbar-width") {
    if (auto sw = ParseScrollbarWidth(v)) {
      style.scrollbar_width = *sw;
    }
    return;
  }

  if (p == "scroll-speed") {
    int val = StoI(v);
    style.scroll_speed_x = val;
    style.scroll_speed_y = val;
    return;
  }

  if (p == "scroll-speed-x") {
    style.scroll_speed_x = StoI(v);
    return;
  }

  if (p == "scroll-speed-y") {
    style.scroll_speed_y = StoI(v);
    return;
  }
}

}  // namespace rtxui
