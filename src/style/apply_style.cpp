#include "style/apply_style.hpp"

#include <optional>
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
  } else if (p == "foreground-color") {
    style.foreground_color = ParseColor(v);
  } else if (p == "display") {
    if (v == "block")
      style.display = Display::Block;
    else if (v == "inline")
      style.display = Display::Inline;
    else if (v == "flex")
      style.display = Display::Flex;
    else if (v == "none")
      style.display = Display::None;
  } else if (p == "flex-direction") {
    if (v == "row")
      style.flex_direction = Direction::Row;
    else if (v == "column")
      style.flex_direction = Direction::Column;
  } else if (p == "flex-grow") {
    style.flex_grow = StoF(v);
  } else if (p == "width") {
    style.width = ParseLength(v);
  } else if (p == "height") {
    style.height = ParseLength(v);
  } else if (p == "border-width") {
    int width = StoI(v);
    style.border = {width, width, width, width};
  }
  else if (p == "margin") {
    int m = StoI(v);
    style.margin = {m, m, m, m};
  } else if (p == "padding") {
    int p = StoI(v);
    style.padding = {p, p, p, p};
  }
}

}  // namespace rtxui
