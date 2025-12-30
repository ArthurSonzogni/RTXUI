#ifndef RTXUI_PAINT_PAINT_HPP
#define RTXUI_PAINT_PAINT_HPP

#include <optional>

#include "paint/color.hpp"
#include "layout/physical_fragment.hpp"
#include "paint/texture.hpp"

namespace rtxui {
void Paint(const PhysicalFragment* frag,
           Texture& texture,
           int off_x = 0,
           int off_y = 0);
}  // namespace rtxui

#endif  // RTXUI_PAINT_PAINT_HPP