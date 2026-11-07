// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/canvas.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>

#include "rtxui/base/string.hpp"
#include "rtxui/color.hpp"
#include "rtxui/paint/canvas_surface.hpp"

namespace rtxui {

std::atomic<int> CanvasSurface::live_count = 0;

CanvasSurface::CanvasSurface() {
  ++live_count;
}

CanvasSurface::~CanvasSurface() {
  --live_count;
}

void CanvasSurface::Reset(int w, int h) {
  width = w;
  height = h;
  cells.assign(static_cast<size_t>(w) * static_cast<size_t>(h), CanvasCell());
}

CanvasCell* CanvasSurface::At(int x, int y) {
  if (x < 0 || y < 0 || x >= width || y >= height) {
    return nullptr;
  }
  return &cells[static_cast<size_t>(y * width + x)];
}

namespace {

struct DotGrid {
  int w;
  int h;
};

DotGrid Grid(Canvas::Pixels pixels) {
  return {DotsAcross(pixels), DotsDown(pixels)};
}

// The bit of a dot, in Unicode's braille order.
constexpr std::array<std::array<uint8_t, 4>, 2> kBrailleBit = {{
    {0x01, 0x02, 0x04, 0x40},
    {0x08, 0x10, 0x20, 0x80},
}};

// Indexed by top-left 1, top-right 2, bottom-left 4, bottom-right 8.
constexpr std::array<std::string_view, 16> kQuadrant = {
    " ", "▘", "▝", "▀", "▖", "▌", "▞", "▛",
    "▗", "▚", "▐", "▜", "▄", "▙", "▟", "█",
};

// Writes a cell, undoing the wide glyph it was part of.
void Clear(CanvasSurface& s, int x, int y) {
  CanvasCell* cell = s.At(x, y);
  if (cell->continuation) {
    if (CanvasCell* before = s.At(x - 1, y)) {
      before->glyph.clear();
    }
  } else if (CanvasCell* after = s.At(x + 1, y); after && after->continuation) {
    *after = CanvasCell();
  }
  const std::optional<Color> background = cell->background;
  *cell = CanvasCell();
  cell->background = background;
}

void SetCell(CanvasSurface& s,
             int x,
             int y,
             std::string_view grapheme,
             int width,
             const Color* color) {
  if (!s.At(x, y) || width <= 0 || (width == 2 && !s.At(x + 1, y))) {
    return;
  }
  Clear(s, x, y);
  CanvasCell* cell = s.At(x, y);
  cell->glyph = std::string(grapheme);
  if (color) {
    cell->foreground = *color;
  }
  if (width == 2) {
    Clear(s, x + 1, y);
    s.At(x + 1, y)->continuation = true;
  }
}

}  // namespace

Canvas::Canvas(CanvasSurface& surface) : surface_(&surface) {}

int Canvas::width() const {
  return surface_->width;
}

int Canvas::height() const {
  return surface_->height;
}

Color Canvas::color() const {
  return surface_->color;
}

void Canvas::Set(int x, int y, std::string_view grapheme) {
  SetCell(*surface_, x, y, grapheme, string_width(grapheme), nullptr);
}

void Canvas::Set(int x, int y, std::string_view grapheme, Color foreground) {
  SetCell(*surface_, x, y, grapheme, string_width(grapheme), &foreground);
}

void Canvas::SetBackground(int x, int y, Color background) {
  if (CanvasCell* cell = surface_->At(x, y)) {
    cell->background = background;
  }
}

void Canvas::Text(int x, int y, std::string_view text) {
  for (const Grapheme& g : Graphemes(text)) {
    SetCell(*surface_, x, y, g.text, g.width, nullptr);
    x += g.width;
  }
}

void Canvas::Text(int x, int y, std::string_view text, Color foreground) {
  for (const Grapheme& g : Graphemes(text)) {
    SetCell(*surface_, x, y, g.text, g.width, &foreground);
    x += g.width;
  }
}

void Canvas::Fill(Color background) {
  for (CanvasCell& cell : surface_->cells) {
    cell.background = background;
  }
}

void Canvas::SetPixels(Pixels pixels) {
  surface_->pixels = pixels;
}

Canvas::Pixels Canvas::pixels() const {
  return surface_->pixels;
}

int Canvas::pixel_width() const {
  return surface_->width * Grid(surface_->pixels).w;
}

int Canvas::pixel_height() const {
  return surface_->height * Grid(surface_->pixels).h;
}

void Canvas::Point(int x, int y) {
  PointImpl(x, y, nullptr);
}

void Canvas::Point(int x, int y, Color color) {
  PointImpl(x, y, &color);
}

void Canvas::Line(int x0, int y0, int x1, int y1) {
  LineImpl(x0, y0, x1, y1, nullptr);
}

void Canvas::Line(int x0, int y0, int x1, int y1, Color color) {
  LineImpl(x0, y0, x1, y1, &color);
}

void Canvas::PointImpl(int x, int y, const Color* color) {
  if (x < 0 || y < 0) {
    return;
  }
  const DotGrid grid = Grid(surface_->pixels);
  const int cx = x / grid.w;
  const int cy = y / grid.h;
  CanvasCell* cell = surface_->At(cx, cy);
  if (!cell) {
    return;
  }
  const int dx = x % grid.w;
  const int dy = y % grid.h;
  // A cell holding a glyph rather than dots starts from no dots.
  const bool had_dots = cell->dots != 0;
  const uint8_t dots = had_dots ? cell->dots : 0;
  const std::optional<Color> upper = cell->upper;
  const std::optional<Color> lower = cell->lower;
  const std::optional<Color> foreground = cell->foreground;
  Clear(*surface_, cx, cy);
  cell->dots = dots;
  if (had_dots) {
    cell->upper = upper;
    cell->lower = lower;
    cell->foreground = foreground;
  }

  switch (surface_->pixels) {
    case Pixels::Braille:
      cell->dots |=
          kBrailleBit[static_cast<size_t>(dx)][static_cast<size_t>(dy)];
      cell->glyph = CodePointToString(0x2800 + cell->dots);
      break;
    case Pixels::Quadrant:
      cell->dots |= static_cast<uint8_t>(1 << (dy * 2 + dx));
      cell->glyph = std::string(kQuadrant[cell->dots]);
      break;
    case Pixels::HalfBlock: {
      std::optional<Color> half =
          color ? std::optional<Color>(*color) : std::nullopt;
      if (dy == 0) {
        cell->upper = half;
      } else {
        cell->lower = half;
      }
      cell->dots |= static_cast<uint8_t>(1 << dy);
      // "▀" draws its upper half in the foreground and its lower half in the
      // background, so two colors fit, the CSS color being either.
      cell->foreground.reset();
      if (cell->dots == 1) {
        cell->glyph = "▀";
        cell->foreground = cell->upper;
      } else if (cell->dots == 2) {
        cell->glyph = "▄";
        cell->foreground = cell->lower;
      } else if (cell->upper == cell->lower) {
        cell->glyph = "█";
        cell->foreground = cell->upper;
      } else if (!cell->lower) {
        cell->glyph = "▄";
        cell->background = cell->upper;
      } else {
        cell->glyph = "▀";
        cell->foreground = cell->upper;
        cell->background = cell->lower;
      }
      return;
    }
  }
  if (color) {
    cell->foreground = *color;
  }
}

void Canvas::LineImpl(int x0, int y0, int x1, int y1, const Color* color) {
  const int dx = std::abs(x1 - x0);
  const int dy = -std::abs(y1 - y0);
  const int sx = x0 < x1 ? 1 : -1;
  const int sy = y0 < y1 ? 1 : -1;
  int error = dx + dy;
  // Bounded, so that a line far outside the surface ends.
  for (int i = 0; i <= dx - dy; ++i) {
    PointImpl(x0, y0, color);
    if (x0 == x1 && y0 == y1) {
      break;
    }
    const int e2 = 2 * error;
    if (e2 >= dy) {
      error += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      error += dx;
      y0 += sy;
    }
  }
}

}  // namespace rtxui
