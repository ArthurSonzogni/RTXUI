#ifndef RTXUI_PAINT_PAINT_HPP
#define RTXUI_PAINT_PAINT_HPP

#include <optional>

#include "rtxui/color.hpp"
#include "rtxui/layout/physical_fragment.hpp"
#include "rtxui/paint/texture.hpp"

namespace rtxui {
/// Paints `frag` into `texture`.
///
/// `screen_background` is what the area behind the whole tree is made of, and
/// it is the color every partially-transparent thing composites against. Left
/// transparent -- the default -- it means "whatever the terminal is showing",
/// which cannot be named in an escape sequence, so the pieces that need it
/// fall back to approximations: a reversed border cell has to ask the terminal
/// to swap its colors, and blending against it is blending against nothing.
/// Give it a concrete color and all of that becomes exact.
void Paint(const PhysicalFragment* frag,
           Texture& texture,
           int off_x = 0,
           int off_y = 0,
           Color screen_background = Color());

/// Counts the `<a>` elements in existence, +1 as one is created and -1 as it
/// goes. Painting looks for the link around each text only while there is
/// one, so that a screen without links does not pay for them.
void CountLinkElements(int delta);
}  // namespace rtxui

#endif  // RTXUI_PAINT_PAINT_HPP
