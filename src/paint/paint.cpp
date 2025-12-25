#include "paint/paint.hpp"

#include <string>

#include "layout/physical_fragment.hpp"
#include "paint/texture.hpp"

namespace rtxui {
void Paint(const PhysicalFragment* frag,
           Texture& texture,
           int off_x,
           int off_y) {
  int abs_x = off_x;
  int abs_y = off_y;
  int w = frag->width;
  int h = frag->height;

  // Randomize the fragment background for debugging:
  //const int rand = static_cast<int>(
      //reinterpret_cast<std::uintptr_t>(frag) >> 3);  // NOLINT
  //const_cast<PhysicalFragment*>(frag)->background_color =
      //Color::RGB((rand * 37) % 256, (rand * 57) % 256, (rand * 97) % 256);

  // 0. Draw Background
  if (frag->background_color.a > 0) {
    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; ++x) {
        int tex_x = abs_x + x;
        int tex_y = abs_y + y;
        if (tex_x >= 0 && tex_x < texture.width() && tex_y >= 0 &&
            tex_y < texture.height()) {
          texture.operator[](tex_x, tex_y).background_color =
              frag->background_color;
        }
      }
    }
  }

  // 1. Draw Border (Box Drawing Characters)
  if (frag->has_border) {
    auto set_char = [&](int x, int y, const std::string& c) {
      if (x >= 0 && x < texture.width() && y >= 0 && y < texture.height()) {
        auto& cell = texture.operator[](x, y);
        cell.character = c;
        if (frag->foreground_color.a > 0) {
          cell.foreground_color = frag->foreground_color;
        }
      }
    };

    // Corners
    set_char(abs_x, abs_y, "┌");
    set_char(abs_x + w - 1, abs_y, "┐");
    set_char(abs_x, abs_y + h - 1, "└");
    set_char(abs_x + w - 1, abs_y + h - 1, "┘");

    // Top/Bottom
    for (int i = 1; i < w - 1; ++i) {
      set_char(abs_x + i, abs_y, "─");
      set_char(abs_x + i, abs_y + h - 1, "─");
    }

    // Left/Right
    for (int i = 1; i < h - 1; ++i) {
      set_char(abs_x, abs_y + i, "│");
      set_char(abs_x + w - 1, abs_y + i, "│");
    }
  }

  // 2. Draw Text
  if (frag->is_text) {
    for (size_t i = 0; i < frag->text_content.size(); ++i) {
      int x = abs_x + static_cast<int>(i);
      int y = abs_y;
      if (x >= 0 && x < texture.width() && y >= 0 && y < texture.height()) {
        auto& cell = texture.operator[](x, y);
        cell.character =
            std::string(1, frag->text_content[i]);
        if (frag->foreground_color.a > 0) {
            cell.foreground_color = frag->foreground_color;
        }
      }
    }
  }

  // 3. Recurse
  for (auto& child : frag->children) {
    Paint(child.fragment.get(), texture, abs_x + child.x, abs_y + child.y);
  }
}
}  // namespace rtxui
