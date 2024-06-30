#include "paint/texture.hpp"

#include <sstream>

#include "core/likely.hpp"

namespace {
void Transition(std::stringstream& ss, const Cell* prev, const Cell* next) {
  // Bold
  if (UNLIKELY((next->bold ^ prev->bold) | (next->dim ^ prev->dim))) {
    // BOLD_AND_DIM_RESET:
    ss << ((prev->bold && !next->bold) || (prev->dim && !next->dim) ? "\x1B[22m"
                                                                    : "");
    ss << (next->bold ? "\x1B[1m" : "");  // BOLD_SET
    ss << (next->dim ? "\x1B[2m" : "");   // DIM_SET
  }

  // Underline
  if (UNLIKELY(next->underlined != prev->underlined ||
               next->underlined_double != prev->underlined_double)) {
    ss << (next->underlined          ? "\x1B[4m"     // UNDERLINE
           : next->underlined_double ? "\x1B[21m"    // UNDERLINE_DOUBLE
                                     : "\x1B[24m");  // UNDERLINE_RESET
  }

  // Blink
  if (UNLIKELY(next->blink != prev->blink)) {
    ss << (next->blink ? "\x1B[5m"     // BLINK_SET
                       : "\x1B[25m");  // BLINK_RESET
  }

  // Inverted
  if (UNLIKELY(next->inverted != prev->inverted)) {
    ss << (next->inverted ? "\x1B[7m"     // INVERTED_SET
                          : "\x1B[27m");  // INVERTED_RESET
  }

  // StrikeThrough
  if (UNLIKELY(next->strikethrough != prev->strikethrough)) {
    ss << (next->strikethrough ? "\x1B[9m"     // CROSSED_OUT
                               : "\x1B[29m");  // CROSSED_OUT_RESET
  }

  if (UNLIKELY(next->foreground_color != prev->foreground_color ||
               next->background_color != prev->background_color)) {
    // ss << "\x1B[" + next->foreground_color.Print(false) + "m";
    // ss << "\x1B[" + next->background_color.Print(true) + "m";
  }
}
}  // namespace

Texture::Texture(std::uint8_t width, std::uint8_t height)
    : width_(width), height_(height), cells_(width * height) {}

Cell& Texture::At(int x, int y) {
  if (x < 0 || x >= width_ || y < 0 || y >= height_) {
    return NullCell;
  }
  return cells_[y * width_ + x];
}

/// @brief Render the output as a string, using terminal escape codes.
std::string Texture::Render() const {
  std::stringstream ss;

  const Cell default_cell;
  const Cell* prev = &default_cell;

  for (int y = 0; y < height_; ++y) {
    // New line in between two lines.
    if (y != 0) {
      Transition(ss, prev, &default_cell);
      prev = &default_cell;
      ss << "\n";
    }

    for (int x = 0; x < width_; ++x) {
      const Cell& cell = cells_[y * width_ + x];
      Transition(ss, prev, &cell);
      prev = &cell;
      if (cell.character.size() == 0) {
        ss << " ";
      } else {
        ss << cell.character;
      }
    }
  }

  // Reset the style at the end of the output.
  Transition(ss, prev, &default_cell);

  return ss.str();
}
