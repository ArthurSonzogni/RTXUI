#include "paint/color.hpp"

// static
Color Color::RGB(std::uint8_t red, std::uint8_t green, std::uint8_t blue) {
  Color color;
  color.r = red;
  color.g = green;
  color.b = blue;
  color.a = 255;
  return color;
}

// static
Color Color::RGBA(std::uint8_t red,
                  std::uint8_t green,
                  std::uint8_t blue,
                  std::uint8_t alpha) {
  Color color;
  color.r = red;
  color.g = green;
  color.b = blue;
  color.a = alpha;
  return color;
}
