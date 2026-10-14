#ifndef RTXUI_PAINT_PAINT_HPP
#define RTXUI_PAINT_PAINT_HPP

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

#include "rtxui/color.hpp"
#include "rtxui/layout/physical_fragment.hpp"
#include "rtxui/paint/texture.hpp"

namespace rtxui {
class Element;

/// A glyph of text, for a selection: the text element it is of, and the byte
/// it starts at in that element's laid-out text.
struct TextPosition {
  const Element* node = nullptr;
  uint32_t offset = 0;
  // The glyph starts a line after a soft wrap: the text runs on from the
  // line before.
  bool after_wrap = false;
};

/// What Paint() does for a text selection.
struct PaintSelection {
  /// Whether a glyph is selected, and drawn in reverse video. Null for none.
  std::function<bool(const TextPosition&)> selected;
  /// When set, sized to the texture: Paint() records there, at
  /// `x + y * width`, the glyph of selectable text each cell shows, if any.
  std::vector<TextPosition>* glyphs = nullptr;
};

/// Whether the text of `element` can be selected, from `user-select`.
bool TextSelectable(const Element* element);

/// Paints `frag` into `texture`.
///
/// `screen_background` is what the area behind the whole tree is made of, and
/// it is the color every partially-transparent thing composites against. Left
/// transparent -- the default -- it means "whatever the terminal is showing",
/// which cannot be named in an escape sequence, so the pieces that need it
/// fall back to approximations: a reversed border cell has to ask the terminal
/// to swap its colors, and blending against it is blending against nothing.
/// Give it a concrete color and all of that becomes exact.
///
/// `screen_foreground` is the color of text no element gives a color to.
void Paint(const PhysicalFragment* frag,
           Texture& texture,
           int off_x = 0,
           int off_y = 0,
           Color screen_background = Color(),
           const PaintSelection* selection = nullptr,
           Color screen_foreground = Color::RGB(255, 255, 255));

/// Counts the `<a>` elements in existence, +1 as one is created and -1 as it
/// goes. Painting looks for the link around each text only while there is
/// one, so that a screen without links does not pay for them.
void CountLinkElements(int delta);
}  // namespace rtxui

#endif  // RTXUI_PAINT_PAINT_HPP
