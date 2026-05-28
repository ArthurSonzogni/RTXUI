#include "rtxui/paint/paint.hpp"

#include <string>
#include <array>
#include <vector>
#include <algorithm>

#include "rtxui/layout/physical_fragment.hpp"
#include "rtxui/paint/texture.hpp"
#include "rtxui/dom/element.hpp"

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
      /* Dotted */
      {
          {
              {"·", "·", "·"},
              {"·", " ", "·"},
              {"·", "·", "·"},
          },
          {
              {0, 0, 0},
              {0, 0, 0},
              {0, 0, 0},
          },
      },
      /* DoubleHorizontal */
      {
          {
              {"╒", "═", "╕"},
              {"│", " ", "│"},
              {"╘", "═", "╛"},
          },
          {
              {0, 0, 0},
              {0, 0, 0},
              {0, 0, 0},
          },
      },
      /* DoubleVertical */
      {
          {
              {"╓", "─", "╖"},
              {"║", " ", "║"},
              {"╙", "─", "╜"},
          },
          {
              {0, 0, 0},
              {0, 0, 0},
              {0, 0, 0},
          },
      },
      /* Shadow */
      {
          {
              {"░", "░", "▓"},
              {"░", " ", "▓"},
              {"░", "▓", "▓"},
          },
          {
              {0, 0, 0},
              {0, 0, 0},
              {0, 0, 0},
          },
      },
      /* ShadeLight */
      {
          {
              {"░", "░", "░"},
              {"░", " ", "░"},
              {"░", "░", "░"},
          },
          {
              {0, 0, 0},
              {0, 0, 0},
              {0, 0, 0},
          },
      },
      /* ShadeMedium */
      {
          {
              {"▒", "▒", "▒"},
              {"▒", " ", "▒"},
              {"▒", "▒", "▒"},
          },
          {
              {0, 0, 0},
              {0, 0, 0},
              {0, 0, 0},
          },
      },
      /* ShadeDark */
      {
          {
              {"▓", "▓", "▓"},
              {"▓", " ", "▓"},
              {"▓", "▓", "▓"},
          },
          {
              {0, 0, 0},
              {0, 0, 0},
              {0, 0, 0},
          },
      },
      /* Squiggle */
      {
          {
              {"~", "~", "~"},
              {"~", " ", "~"},
              {"~", "~", "~"},
          },
          {
              {0, 0, 0},
              {0, 0, 0},
              {0, 0, 0},
          },
      },
  };
  return border_styles[static_cast<size_t>(style)];
}

struct ClipRect {
  int x1 = 0;
  int y1 = 0;
  int x2 = 1000000;
  int y2 = 1000000;

  bool Contains(int x, int y) const {
    return x >= x1 && x < x2 && y >= y1 && y < y2;
  }

  ClipRect Intersect(const ClipRect& other) const {
    return {
        std::max(x1, other.x1),
        std::max(y1, other.y1),
        std::min(x2, other.x2),
        std::min(y2, other.y2),
    };
  }
};

void PaintImpl(const PhysicalFragment* frag,
           Texture& texture,
           int off_x,
           int off_y,
           Color inherited_foreground_color,
           Color parent_background_color,
           ClipRect clip) {
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
              tex_y < texture.height() && clip.Contains(tex_x, tex_y)) {
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
    Color top_border_color =
        frag->border_color_top.value_or(current_foreground_color);
    Color right_border_color =
        frag->border_color_right.value_or(current_foreground_color);
    Color bottom_border_color =
        frag->border_color_bottom.value_or(current_foreground_color);
    Color left_border_color =
        frag->border_color_left.value_or(current_foreground_color);

    auto set_char = [&](int x, int y, const char* c, uint8_t mode,
                        const Color& border_color) {
      if (c == nullptr || *c == '\0' || *c == ' ')
        return;
      if (x >= 0 && x < texture.width() && y >= 0 && y < texture.height() &&
          clip.Contains(x, y)) {
        auto& cell = texture[x, y];
        cell.character = c;

        Color inner_bg = current_background_color;
        Color outer_bg = parent_background_color;

        switch (mode) {
          case 0:  // Widget
            cell.foreground_color = Blend(border_color, cell.background_color);
            break;
          case 1:  // Parent
            cell.foreground_color = border_color;
            cell.background_color = outer_bg;
            break;
          case 2:  // ReverseOuter
            cell.foreground_color = outer_bg;
            cell.background_color = Blend(border_color, inner_bg);
            break;
          case 3:  // ReverseInner
            cell.foreground_color = inner_bg;
            cell.background_color = Blend(border_color, outer_bg);
            break;
        }
      }
    };

    const auto& data = GetBorderData(frag->border_style);

    // Corners
    set_char(abs_x, abs_y, data.charset[0][0], data.locations[0][0],
             left_border_color);  // tl
    set_char(abs_x + w - 1, abs_y, data.charset[0][2], data.locations[0][2],
             right_border_color);  // tr
    set_char(abs_x, abs_y + h - 1, data.charset[2][0], data.locations[2][0],
             left_border_color);  // bl
    set_char(abs_x + w - 1, abs_y + h - 1, data.charset[2][2],
             data.locations[2][2], right_border_color);  // br

    // Top/Bottom
    for (int i = 1; i < w - 1; ++i) {
      set_char(abs_x + i, abs_y, data.charset[0][1], data.locations[0][1],
               top_border_color);  // t
      set_char(abs_x + i, abs_y + h - 1, data.charset[2][1],
               data.locations[2][1], bottom_border_color);  // b
    }

    // Left/Right
    for (int i = 1; i < h - 1; ++i) {
      set_char(abs_x, abs_y + i, data.charset[1][0], data.locations[1][0],
               left_border_color);  // l
      set_char(abs_x + w - 1, abs_y + i, data.charset[1][2],
               data.locations[1][2], right_border_color);  // r
    }
  }

  // 2. Draw Text
  if (frag->is_text) {
    for (size_t i = 0; i < frag->text_content.size(); ++i) {
      int x = abs_x + static_cast<int>(i) + (frag->has_border ? 1 : 0);
      int y = abs_y + (frag->has_border ? 1 : 0);
      if (x >= 0 && x < texture.width() && y >= 0 && y < texture.height() &&
          clip.Contains(x, y)) {
        auto& cell = texture[x, y];
        cell.character = std::string(1, frag->text_content[i]);
        Color fg = current_foreground_color;
        Color bg = cell.background_color;
        cell.foreground_color = Blend(fg, bg);
      }
    }
  }

  // 3. Draw Scrollbar
  bool draw_scrollbar = false;
  if (frag->dom_node && frag->dom_node->style.overflow_y == Overflow::Scroll &&
      frag->dom_node->style.scrollbar_width == ScrollbarWidth::Auto) {
    draw_scrollbar = true;
  }

  if (draw_scrollbar) {
    int border_right = (frag->has_border && frag->border_style != BorderStyle::None) ? 1 : 0;
    int scrollbar_x = abs_x + w - border_right - 1;
    // When there is no horizontal scrollbar, the vertical scrollbar spans the
    // full box height (including border rows) for a cleaner look.
    bool has_horizontal_scrollbar = false; // Not yet supported.
    int track_y_start = abs_y;
    int track_h = h;
    if (has_horizontal_scrollbar) {
      int border_top = border_right; // uniform border
      int border_bottom = border_right;
      track_y_start = abs_y + border_top;
      track_h = h - border_top - border_bottom;
    }

    if (track_h > 0 && scrollbar_x >= 0 && scrollbar_x < texture.width()) {
      int scroll_height = frag->dom_node->scroll_height();
      int padding_vert = frag->dom_node->style.padding.Vert();
      int border_vert = (frag->has_border && frag->border_style != BorderStyle::None) ? 1 : 0;
      int viewport_h = std::max(1, h - 2 * border_vert - padding_vert);

      int thumb_h = std::max(1, (viewport_h * track_h) / std::max(1, scroll_height));
      thumb_h = std::min(track_h, thumb_h);
      int max_scroll = scroll_height - h;
      int thumb_y = (max_scroll > 0) ? ((track_h - thumb_h) * frag->scroll_y) / max_scroll : 0;

      for (int i = 0; i < track_h; ++i) {
        int y = track_y_start + i;
        if (y >= 0 && y < texture.height() && clip.Contains(scrollbar_x, y)) {
          auto& cell = texture[scrollbar_x, y];
          cell.character = " ";
          if (i >= thumb_y && i < thumb_y + thumb_h) {
            cell.background_color = Color::RGBA(200, 200, 200, 200);
          } else {
            cell.background_color = Color::RGBA(80, 80, 80, 120);
          }
        }
      }
    }
  }

  // 4. Recurse
  ClipRect child_clip = clip;
  int scroll_y_offset = 0;
  if (frag->clips_descendants) {
    scroll_y_offset = frag->scroll_y;
    int border_l = 0, border_r = 0, border_t = 0, border_b = 0;
    int padding_l = 0, padding_r = 0, padding_t = 0, padding_b = 0;

    if (frag->dom_node) {
      padding_l = frag->dom_node->style.padding.left;
      padding_r = frag->dom_node->style.padding.right;
      padding_t = frag->dom_node->style.padding.top;
      padding_b = frag->dom_node->style.padding.bottom;
    }

    if (frag->has_border && frag->border_style != BorderStyle::None) {
      border_l = 1;
      border_r = 1;
      border_t = 1;
      border_b = 1;
    }

    int scrollbar_w = 0;
    if (frag->dom_node && frag->dom_node->style.overflow_y == Overflow::Scroll &&
        frag->dom_node->style.scrollbar_width == ScrollbarWidth::Auto) {
      scrollbar_w = 1;
    }

    ClipRect local_content_box = {
        abs_x + border_l + padding_l,
        abs_y + border_t + padding_t,
        abs_x + w - border_r - padding_r - scrollbar_w,
        abs_y + h - border_b - padding_b,
    };
    child_clip = clip.Intersect(local_content_box);
  }

  for (auto& child : frag->children) {
    PaintImpl(child.fragment.get(), texture, abs_x + child.x,
              abs_y + child.y - scroll_y_offset, current_foreground_color,
              current_background_color, child_clip);
  }
}
}  // namespace

void Paint(const PhysicalFragment* frag,
           Texture& texture,
           int off_x,
           int off_y) {
  PaintImpl(frag, texture, off_x, off_y, Color::RGB(255, 255, 255),
            Color::RGB(0, 0, 0), ClipRect{0, 0, texture.width(), texture.height()});
}

}  // namespace rtxui
