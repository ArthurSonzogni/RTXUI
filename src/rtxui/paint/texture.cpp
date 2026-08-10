// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/paint/texture.hpp"

#include <sstream>

#include "rtxui/base/likely.hpp"

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

  // Italic
  if (UNLIKELY(next->italic != prev->italic)) {
    ss << (next->italic ? "\x1B[3m"     // ITALIC_SET
                        : "\x1B[23m");  // ITALIC_RESET
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

  // Overline
  if (UNLIKELY(next->overlined != prev->overlined)) {
    ss << (next->overlined ? "\x1B[53m"     // OVERLINED
                           : "\x1B[55m");  // OVERLINED_RESET
  }

  if (UNLIKELY(next->foreground_color != prev->foreground_color)) {
    if (next->foreground_color.a == 0) {
      ss << "\x1B[39m";
    } else {
      ss << "\x1B[38;2";
      ss << ";" << static_cast<int>(next->foreground_color.r);
      ss << ";" << static_cast<int>(next->foreground_color.g);
      ss << ";" << static_cast<int>(next->foreground_color.b);
      ss << "m";
    }
  }

  if (UNLIKELY(next->background_color != prev->background_color)) {
    if (next->background_color.a == 0) {
      ss << "\x1B[49m";
    } else {
      ss << "\x1B[48;2";
      ss << ";" << static_cast<int>(next->background_color.r);
      ss << ";" << static_cast<int>(next->background_color.g);
      ss << ";" << static_cast<int>(next->background_color.b);
      ss << "m";
    }
  }
}
}  // namespace

Texture::Texture(std::uint8_t width, std::uint8_t height)
    : width_(width), height_(height), cells_(width * height) {}

Cell& Texture::operator[](int x, int y) {
  if (x < 0 || x >= width_ || y < 0 || y >= height_) {
    return NullCell;
  }
  return cells_[y * width_ + x];
}

const Cell& Texture::operator[](int x, int y) const {
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
      // Continuation cells are the second half of a double-width grapheme.
      // The terminal cursor already advanced past this column when the wide
      // character was printed, so we must emit nothing here.
      if (cell.is_continuation) {
        continue;
      }
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

std::string Texture::RenderDiff(const Texture& old_texture) const {
  std::stringstream ss;

  if (old_texture.width() != width_ || old_texture.height() != height_) {
    return Render();
  }

  const Cell default_cell;
  const Cell* prev = &default_cell;

  int cursor_x = 0;
  int cursor_y = 0;

  auto MoveCursor = [&](int tx, int ty) {
    if (ty > cursor_y) {
      ss << std::string(ty - cursor_y, '\n') << "\r";
      cursor_y = ty;
      cursor_x = 0;
    } else if (ty < cursor_y) {
      ss << "\x1b[" << (cursor_y - ty) << "A";
      cursor_y = ty;
    }

    if (tx > cursor_x) {
      ss << "\x1b[" << (tx - cursor_x) << "C";
      cursor_x = tx;
    } else if (tx < cursor_x) {
      ss << "\r";
      if (tx > 0) {
        ss << "\x1b[" << tx << "C";
      }
      cursor_x = tx;
    }
  };

  for (int y = 0; y < height_; ++y) {
    for (int x = 0; x < width_; ++x) {
      const Cell& cell = cells_[y * width_ + x];
      const Cell& old_cell = old_texture.cells_[y * width_ + x];

      if (cell == old_cell) {
        continue;
      }

      if (cell.is_continuation) {
        continue;
      }

      MoveCursor(x, y);

      Transition(ss, prev, &cell);
      prev = &cell;

      if (cell.character.empty()) {
        ss << " ";
      } else {
        ss << cell.character;
      }

      bool is_wide = (x + 1 < width_ && cells_[y * width_ + x + 1].is_continuation);
      cursor_x += (is_wide ? 2 : 1);
    }
  }

  // Position cursor at the bottom-left of the screen.
  MoveCursor(0, height_ - 1);

  // Reset the style at the end of the output.
  Transition(ss, prev, &default_cell);

  return ss.str();
}
