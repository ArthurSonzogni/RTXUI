// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_PAINT_CANVAS_SURFACE_HPP_
#define RTXUI_PAINT_CANVAS_SURFACE_HPP_

#include <atomic>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "rtxui/canvas.hpp"
#include "rtxui/color.hpp"

namespace rtxui {

// What a <canvas> drew, kept from one ondraw call to the next and copied into
// the frame by paint. Owned by the <canvas> component; its root element points
// to it (Element::canvas_surface()).
struct CanvasCell {
  // Empty: nothing drawn, the cell shows what is beneath it.
  std::string glyph;
  // Unset: the element's CSS color, at paint time.
  std::optional<Color> foreground;
  std::optional<Color> background;
  // The second half of a wide glyph drawn in the cell before.
  bool continuation = false;
  // In pixel mode: the dots set, and for half blocks the color of each half.
  uint8_t dots = 0;
  std::optional<Color> upper;
  std::optional<Color> lower;
};

// How many dots a cell holds across and down, in each Canvas::Pixels mode.
inline int DotsAcross(Canvas::Pixels pixels) {
  return pixels == Canvas::Pixels::HalfBlock ? 1 : 2;
}
inline int DotsDown(Canvas::Pixels pixels) {
  return pixels == Canvas::Pixels::Braille ? 4 : 2;
}

struct CanvasSurface {
  CanvasSurface();
  ~CanvasSurface();
  CanvasSurface(const CanvasSurface&) = delete;
  CanvasSurface& operator=(const CanvasSurface&) = delete;

  // Clears the surface to `width` x `height` empty cells.
  void Reset(int width, int height);
  CanvasCell* At(int x, int y);

  int width = 0;
  int height = 0;
  std::vector<CanvasCell> cells;
  Canvas::Pixels pixels = Canvas::Pixels::Braille;
  Color color;

  // Its size when CSS sets none, in cells: the `width` and `height`
  // attributes.
  int intrinsic_width = 30;
  int intrinsic_height = 8;

  // Where the last paint put the content box, in frame cells.
  int content_x = 0;
  int content_y = 0;

  // Whether `cells` is up to date: drawn once, at this size, since the owner's
  // state last changed (ComponentInternals::StateGeneration).
  bool drawn = false;
  uint64_t owner_generation = 0;
  // Whether a missing ondraw / onpointer was reported, once.
  bool reported_draw = false;
  bool reported_pointer = false;

  // How many surfaces exist: a frame without any skips looking for them.
  static std::atomic<int> live_count;
};

}  // namespace rtxui

#endif  // RTXUI_PAINT_CANVAS_SURFACE_HPP_
