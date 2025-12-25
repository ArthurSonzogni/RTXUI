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

Color Blend(Color over, Color under) {
  if (over.a == 255)
    return over;
  if (over.a == 0)
    return under;
  if (under.a == 0)
    return over;

  float a_over = over.a / 255.f;
  float a_under = under.a / 255.f;

  float a_out = a_over + a_under * (1.f - a_over);
  if (a_out == 0.f)
    return Color::RGBA(0, 0, 0, 0);

  uint8_t r = (over.r * a_over + under.r * a_under * (1.f - a_over)) / a_out;
  uint8_t g = (over.g * a_over + under.g * a_under * (1.f - a_over)) / a_out;
  uint8_t b = (over.b * a_over + under.b * a_under * (1.f - a_over)) / a_out;

  return Color::RGBA(r, g, b, a_out * 255.f);
}
