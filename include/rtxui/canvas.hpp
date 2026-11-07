// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_CANVAS_HPP_
#define RTXUI_CANVAS_HPP_

#include <rtxui/rtxui_export.hpp>
#include <string_view>

#include "rtxui/color.hpp"
#include "rtxui/event.hpp"

namespace rtxui {

struct CanvasSurface;

/// What a `<canvas ondraw="Draw">` hands its draw function: a surface the
/// size of the element's content box, in cells, cleared before each call.
/// Anything drawn outside it is clipped. A cell left alone shows what is
/// beneath it: the element's CSS background, or its parent.
///
/// A color left out is the element's CSS `color`, as it is when painted.
class RTXUI_EXPORT Canvas {
 public:
  explicit Canvas(CanvasSurface& surface);

  /// The size of the surface, in cells.
  int width() const;
  int height() const;
  /// The element's CSS `color`, to derive others from.
  Color color() const;

  // Cells --------------------------------------------------------------------
  /// One grapheme at a cell. A wide one (CJK, most emoji) takes two cells.
  void Set(int x, int y, std::string_view grapheme);
  void Set(int x, int y, std::string_view grapheme, Color foreground);
  /// A cell's background.
  void SetBackground(int x, int y, Color background);
  /// A line of text from a cell, rightward. Not wrapped.
  void Text(int x, int y, std::string_view text);
  void Text(int x, int y, std::string_view text, Color foreground);
  /// Every cell's background.
  void Fill(Color background);

  // Pixels -------------------------------------------------------------------
  /// How many dots a cell holds, for Point() and Line().
  enum class Pixels {
    Braille,    ///< 2×4 dots per cell (the default), one color per cell.
    Quadrant,   ///< 2×2 blocks per cell, one color per cell.
    HalfBlock,  ///< 1×2 square-ish blocks per cell, a color for each.
  };
  void SetPixels(Pixels pixels);
  Pixels pixels() const;
  /// The size of the surface, in dots.
  int pixel_width() const;
  int pixel_height() const;
  void Point(int x, int y);
  void Point(int x, int y, Color color);
  void Line(int x0, int y0, int x1, int y1);
  void Line(int x0, int y0, int x1, int y1, Color color);

 private:
  void PointImpl(int x, int y, const Color* color);
  void LineImpl(int x0, int y0, int x1, int y1, const Color* color);
  CanvasSurface* surface_;
};

/// What a `<canvas onpointer="OnPointer">` hands its handler: a mouse event
/// over the canvas, in its own coordinates. A press captures the pointer
/// until its release, so a drag keeps reporting, with coordinates outside the
/// canvas when it leaves it.
struct PointerEvent {
  /// The cell, from the content box's top left.
  int x = 0;
  int y = 0;
  /// The dot, in the canvas's current Canvas::Pixels grid. Precise when the
  /// terminal reports pixels, else the middle of the cell.
  int pixel_x = 0;
  int pixel_y = 0;
  /// The button, motion (pressed, moved, released) and modifiers.
  Event::Mouse mouse;
};

}  // namespace rtxui

#endif  // RTXUI_CANVAS_HPP_
