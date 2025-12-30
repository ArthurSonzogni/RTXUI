#include "paint/paint.hpp"

#include <string>
#include <array>
#include <vector>

#include "layout/physical_fragment.hpp"
#include "paint/texture.hpp"

namespace rtxui {

namespace {

struct BorderData {
  const char* charset[3][3];
  uint8_t locations[3][3];
};

// Map BorderStyle to its character set and location mapping based on Textual.
// Location Modes:
// 0: Widget (normal)
// 1: Parent (background of parent)
// 2: ReverseOuter (outer.bg, inner.fg, reverse=true)
// 3: ReverseInner (inner.bg, outer.fg, reverse=true)
const BorderData& GetBorderData(BorderStyle style) {
  static const BorderData border_styles[] = {
      /* Ascii */
      {
          {
              {"+", "-", "+"},
              {"|", " ", "|"},
              {"+", "-", "+"},
          },
          {
              {0, 0, 0},
              {0, 0, 0},
              {0, 0, 0},
          },
      },
      /* Blank */
      {
          {
              {" ", " ", " "},
              {" ", " ", " "},
              {" ", " ", " "},
          },
          {
              {0, 0, 0},
              {0, 0, 0},
              {0, 0, 0},
          },
      },
      /* Dashed */
      {
          {
              {"┏", "╍", "┓"},
              {"╏", " ", "╏"},
              {"┗", "╍", "┛"},
          },
          {
              {0, 0, 0},
              {0, 0, 0},
              {0, 0, 0},
          },
      },
      /* Double */
      {
          {
              {"╔", "═", "╗"},
              {"║", " ", "║"},
              {"╚", "═", "╝"},
          },
          {
              {0, 0, 0},
              {0, 0, 0},
              {0, 0, 0},
          },
      },
      /* HKey */
      {
          {
              {"▔", "▔", "▔"},
              {" ", " ", " "},
              {"▁", "▁", "▁"},
          },
          {
              {0, 0, 0},
              {0, 0, 0},
              {0, 0, 0},
          },
      },
      /* Heavy */
      {
          {
              {"┏", "━", "┓"},
              {"┃", " ", "┃"},
              {"┗", "━", "┛"},
          },
          {
              {0, 0, 0},
              {0, 0, 0},
              {0, 0, 0},
          },
      },
      /* Inner */
      {
          {
              {"▗", "▄", "▖"},
              {"▐", " ", "▌"},
              {"▝", "▀", "▘"},
          },
          {
              {1, 1, 1},
              {1, 1, 1},
              {1, 1, 1},
          },
      },
      /* None */
      {
          {
              {" ", " ", " "},
              {" ", " ", " "},
              {" ", " ", " "},
          },
          {
              {0, 0, 0},
              {0, 0, 0},
              {0, 0, 0},
          },
      },
      /* Outer */
      {
          {
              {"▛", "▀", "▜"},
              {"▌", " ", "▐"},
              {"▙", "▄", "▟"},
          },
          {
              {0, 0, 0},
              {0, 0, 0},
              {0, 0, 0},
          },
      },
      /* Panel */
      {
          {
              {"▊", "█", "▎"},
              {"▊", " ", "▎"},
              {"▊", "▁", "▎"},
          },
          {
              {2, 0, 1},
              {2, 0, 1},
              {2, 0, 1},
          },
      },
      /* Round */
      {
          {
              {"╭", "─", "╮"},
              {"│", " ", "│"},
              {"╰", "─", "╯"},
          },
          {
              {0, 0, 0},
              {0, 0, 0},
              {0, 0, 0},
          },
      },
      /* Solid */
      {
          {
              {"┌", "─", "┐"},
              {"│", " ", "│"},
              {"└", "─", "┘"},
          },
          {
              {0, 0, 0},
              {0, 0, 0},
              {0, 0, 0},
          },
      },
      /* Tall */
      {
          {
              {"▊", "▔", "▎"},
              {"▊", " ", "▎"},
              {"▊", "▁", "▎"},
          },
          {
              {2, 0, 1},
              {2, 0, 1},
              {2, 0, 1},
          },
      },
      /* Thick */
      {
          {
              {"█", "▀", "█"},
              {"█", " ", "█"},
              {"█", "▄", "█"},
          },
          {
              {0, 0, 0},
              {0, 0, 0},
              {0, 0, 0},
          },
      },
      /* VKey */
      {
          {
              {"▏", " ", "▕"},
              {"▏", " ", "▕"},
              {"▏", " ", "▕"},
          },
          {
              {0, 0, 0},
              {0, 0, 0},
              {0, 0, 0},
          },
      },
      /* Wide */
      {
          {
              {"▁", "▁", "▁"},
              {"▎", " ", "▊"},
              {"▔", "▔", "▔"},
          },
          {
              {1, 1, 1},
              {0, 1, 3},
              {1, 1, 1},
          },
      },
  };
  return border_styles[static_cast<size_t>(style)];
}

void Paint(const PhysicalFragment* frag,
           Texture& texture,
           int off_x,
           int off_y,
           Color inherited_foreground_color,
           Color parent_background_color) {
  int abs_x = off_x;
  int abs_y = off_y;
  int w = frag->width;
  int h = frag->height;

  Color current_foreground_color =
      frag->foreground_color.value_or(inherited_foreground_color);
  Color current_background_color = frag->background_color.value_or(Color::RGBA(0, 0, 0, 0));

  // 0. Draw Background
  if (frag->background_color) {
    Color new_bg = frag->background_color.value();
    if (new_bg.a > 0) {
      for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
          int tex_x = abs_x + x;
          int tex_y = abs_y + y;
          if (tex_x >= 0 && tex_x < texture.width() && tex_y >= 0 &&
              tex_y < texture.height()) {
            auto& cell = texture[tex_x, tex_y];
            Color under_bg = cell.background_color;
            cell.background_color = Blend(new_bg, under_bg);
          }
        }
      }
    }
  }

  // 1. Draw Border
  if (frag->has_border && frag->border_style != BorderStyle::None) {
    Color border_color = frag->border_color.value_or(current_foreground_color);

    auto set_char = [&](int x, int y, const char* c, uint8_t mode) {
      if (c == nullptr || *c == '\0' || *c == ' ') return;
      if (x >= 0 && x < texture.width() && y >= 0 && y < texture.height()) {
        auto& cell = texture[x, y];
        cell.character = c;

        Color inner_bg = current_background_color;
        Color outer_bg = parent_background_color;
        
        switch (mode) {
          case 0: // Widget
            cell.foreground_color = Blend(border_color, cell.background_color);
            break;
          case 1: // Parent
            cell.foreground_color = border_color;
            cell.background_color = outer_bg;
            break;
          case 2: // ReverseOuter
            cell.foreground_color = outer_bg;
            cell.background_color = Blend(border_color, inner_bg);
            break;
          case 3: // ReverseInner
            cell.foreground_color = inner_bg;
            cell.background_color = Blend(border_color, outer_bg);
            break;
        }
      }
    };

    const auto& data = GetBorderData(frag->border_style);

    // Corners
    set_char(abs_x, abs_y, data.charset[0][0], data.locations[0][0]);           // tl
    set_char(abs_x + w - 1, abs_y, data.charset[0][2], data.locations[0][2]);   // tr
    set_char(abs_x, abs_y + h - 1, data.charset[2][0], data.locations[2][0]);   // bl
    set_char(abs_x + w - 1, abs_y + h - 1, data.charset[2][2], data.locations[2][2]); // br

    // Top/Bottom
    for (int i = 1; i < w - 1; ++i) {
      set_char(abs_x + i, abs_y, data.charset[0][1], data.locations[0][1]);     // t
      set_char(abs_x + i, abs_y + h - 1, data.charset[2][1], data.locations[2][1]); // b
    }

    // Left/Right
    for (int i = 1; i < h - 1; ++i) {
      set_char(abs_x, abs_y + i, data.charset[1][0], data.locations[1][0]);     // l
      set_char(abs_x + w - 1, abs_y + i, data.charset[1][2], data.locations[1][2]); // r
    }
  }

  // 2. Draw Text
  if (frag->is_text) {
    for (size_t i = 0; i < frag->text_content.size(); ++i) {
      int x = abs_x + static_cast<int>(i) + (frag->has_border ? 1 : 0);
      int y = abs_y + (frag->has_border ? 1 : 0);
      if (x >= 0 && x < texture.width() && y >= 0 && y < texture.height()) {
        auto& cell = texture[x, y];
        cell.character = std::string(1, frag->text_content[i]);
        Color fg = current_foreground_color;
        Color bg = cell.background_color;
        cell.foreground_color = Blend(fg, bg);
      }
    }
  }

  // 3. Recurse
  for (auto& child : frag->children) {
    Paint(child.fragment.get(), texture, abs_x + child.x, abs_y + child.y,
          current_foreground_color, current_background_color);
  }
}
}  // namespace

void Paint(const PhysicalFragment* frag,
           Texture& texture,
           int off_x,
           int off_y) {
  Paint(frag, texture, off_x, off_y, Color::RGB(255, 255, 255), Color::RGB(0, 0, 0));
}

}  // namespace rtxui
