#include "rtxui/paint/paint.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <vector>

#include "rtxui/core/string.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/layout/physical_fragment.hpp"
#include "rtxui/paint/texture.hpp"

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
               int accum_scroll_x,
               int accum_scroll_y,
               Color inherited_foreground_color,
               Color parent_background_color,
               bool inherited_bold,
               bool inherited_underlined,
               bool inherited_underlined_double,
               bool inherited_strikethrough,
               bool inherited_blink,
               ClipRect clip) {
  int abs_x = off_x;
  int abs_y = off_y;
  int w = frag->width;
  int h = frag->height;

  if (frag->dom_node) {
    const_cast<Element*>(frag->dom_node)->set_absolute_position(abs_x, abs_y);
  }

  Color current_foreground_color =
      frag->foreground_color.value_or(inherited_foreground_color);
  Color current_background_color =
      frag->background_color.value_or(Color::RGBA(0, 0, 0, 0));
  bool current_bold = frag->bold.value_or(inherited_bold);
  bool current_underlined = frag->underlined.value_or(inherited_underlined);
  bool current_underlined_double = frag->underlined_double.value_or(inherited_underlined_double);
  bool current_strikethrough = frag->strikethrough.value_or(inherited_strikethrough);
  bool current_blink = frag->blink.value_or(inherited_blink);

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
      if (c == nullptr || *c == '\0' || *c == ' ') {
        return;
      }
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
    int border_offset = frag->has_border ? 1 : 0;
    int cell_x = 0;  // cell column offset within the text fragment
    int y = abs_y + border_offset;
    for (const Grapheme& g : Graphemes(frag->text_content)) {
      // Orphan combining marks (width==0, no base codepoint in this fragment)
      // are appended to the previous cell so the terminal can compose them.
      if (g.width == 0) {
        int x_prev = abs_x + border_offset + cell_x - 1;
        if (x_prev >= 0 && x_prev < texture.width() && y >= 0 &&
            y < texture.height() && clip.Contains(x_prev, y)) {
          texture[x_prev, y].character += std::string(g.text);
        }
        continue;  // no column advance
      }

      int x = abs_x + border_offset + cell_x;
      if (x >= 0 && x < texture.width() && y >= 0 && y < texture.height() &&
          clip.Contains(x, y)) {
        auto& cell = texture[x, y];
        cell.character = std::string(g.text);
        Color fg = current_foreground_color;
        Color bg = cell.background_color;
        cell.foreground_color = Blend(fg, bg);
        cell.bold = current_bold;
        cell.underlined = current_underlined;
        cell.underlined_double = current_underlined_double;
        cell.strikethrough = current_strikethrough;
        cell.blink = current_blink;
      }
      // For double-width graphemes, mark the continuation cell so the renderer
      // skips it — the terminal cursor already advanced 2 columns.
      if (g.width == 2) {
        int x2 = x + 1;
        if (x2 >= 0 && x2 < texture.width() && y >= 0 && y < texture.height() &&
            clip.Contains(x2, y)) {
          texture[x2, y].is_continuation = true;
        }
      }
      cell_x += g.width;
    }
  }

  // 3. Draw Scrollbars
  bool draw_v_scrollbar = false;
  if (frag->dom_node && frag->dom_node->style.overflow_y == Overflow::Scroll &&
      frag->dom_node->style.scrollbar_width == ScrollbarWidth::Auto) {
    draw_v_scrollbar = true;
  }

  bool draw_h_scrollbar = false;
  if (frag->dom_node && frag->dom_node->style.overflow_x == Overflow::Scroll &&
      frag->dom_node->style.scrollbar_width == ScrollbarWidth::Auto) {
    draw_h_scrollbar = true;
  }

  if (draw_v_scrollbar) {
    int border_right =
        (frag->has_border && frag->border_style != BorderStyle::None) ? 1 : 0;
    int scrollbar_x = abs_x + w - border_right - 1;
    int track_y_start = abs_y;
    int track_h = h;
    if (frag->has_border && frag->border_style != BorderStyle::None) {
      if (draw_h_scrollbar) {
        track_y_start = abs_y + 1;
        track_h = h - 2;
      }
    } else {
      if (draw_h_scrollbar) {
        track_h = h - 1;
      }
    }

    if (track_h > 0 && scrollbar_x >= 0 && scrollbar_x < texture.width()) {
      int scroll_height = frag->dom_node->scroll_height();
      int padding_vert = frag->dom_node->style.padding.Vert();
      int border_vert =
          (frag->has_border && frag->border_style != BorderStyle::None) ? 1 : 0;
      int viewport_h = std::max(1, h - 2 * border_vert - padding_vert);

      int thumb_h =
          std::max(1, (viewport_h * track_h) / std::max(1, scroll_height));
      thumb_h = std::min(track_h, thumb_h);
      int max_scroll = scroll_height - h;
      int thumb_y = (max_scroll > 0)
                        ? ((track_h - thumb_h) * frag->scroll_y) / max_scroll
                        : 0;

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

  if (draw_h_scrollbar) {
    int border_bottom =
        (frag->has_border && frag->border_style != BorderStyle::None) ? 1 : 0;
    int scrollbar_y = abs_y + h - border_bottom - 1;
    int track_x_start = abs_x;
    int track_w = w;
    if (frag->has_border && frag->border_style != BorderStyle::None) {
      if (draw_v_scrollbar) {
        track_x_start = abs_x + 1;
        track_w = w - 2;
      }
    } else {
      if (draw_v_scrollbar) {
        track_w = w - 1;
      }
    }

    if (track_w > 0 && scrollbar_y >= 0 && scrollbar_y < texture.height()) {
      int scroll_width = frag->dom_node->scroll_width();
      int padding_horiz = frag->dom_node->style.padding.Horiz();
      int border_horiz =
          (frag->has_border && frag->border_style != BorderStyle::None) ? 2 : 0;
      int viewport_w = std::max(1, w - border_horiz - padding_horiz);

      int thumb_w =
          std::max(1, (viewport_w * track_w) / std::max(1, scroll_width));
      thumb_w = std::min(track_w, thumb_w);
      int max_scroll = scroll_width - w;
      int thumb_x = (max_scroll > 0)
                        ? ((track_w - thumb_w) * frag->scroll_x) / max_scroll
                        : 0;

      for (int i = 0; i < track_w; ++i) {
        int x = track_x_start + i;
        if (x >= 0 && x < texture.width() && clip.Contains(x, scrollbar_y)) {
          auto& cell = texture[x, scrollbar_y];
          cell.character = " ";
          if (i >= thumb_x && i < thumb_x + thumb_w) {
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
  int scroll_x_offset = 0;
  int scroll_y_offset = 0;
  if (frag->clips_descendants) {
    scroll_x_offset = frag->scroll_x;
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
    if (frag->dom_node &&
        frag->dom_node->style.overflow_y == Overflow::Scroll &&
        frag->dom_node->style.scrollbar_width == ScrollbarWidth::Auto) {
      scrollbar_w = 1;
    }

    int scrollbar_h = 0;
    if (frag->dom_node &&
        frag->dom_node->style.overflow_x == Overflow::Scroll &&
        frag->dom_node->style.scrollbar_width == ScrollbarWidth::Auto) {
      scrollbar_h = 1;
    }

    ClipRect local_content_box = {
        abs_x + border_l + padding_l,
        abs_y + border_t + padding_t,
        abs_x + w - border_r - padding_r - scrollbar_w,
        abs_y + h - border_b - padding_b - scrollbar_h,
    };
    child_clip = clip.Intersect(local_content_box);
  }

  auto sorted_children = frag->children;
  std::stable_sort(sorted_children.begin(), sorted_children.end(),
                   [](const PhysicalFragment::ChildLink& a, const PhysicalFragment::ChildLink& b) {
                     int az = (a.fragment && a.fragment->dom_node) ? a.fragment->dom_node->style.z_index.value_or(0) : 0;
                     int bz = (b.fragment && b.fragment->dom_node) ? b.fragment->dom_node->style.z_index.value_or(0) : 0;
                     return az < bz;
                   });

  int next_accum_scroll_x = accum_scroll_x + scroll_x_offset;
  int next_accum_scroll_y = accum_scroll_y + scroll_y_offset;

  for (auto& child : sorted_children) {
    bool is_fixed = (child.fragment && child.fragment->dom_node &&
                     child.fragment->dom_node->style.position == PositionType::Fixed);

    int child_off_x = abs_x + child.x;
    int child_off_y = abs_y + child.y;
    int child_accum_scroll_x = next_accum_scroll_x;
    int child_accum_scroll_y = next_accum_scroll_y;
    ClipRect child_clip_to_pass = child_clip;

    if (is_fixed) {
      child_off_x += accum_scroll_x;
      child_off_y += accum_scroll_y;
      child_accum_scroll_x = 0;
      child_accum_scroll_y = 0;
      child_clip_to_pass = ClipRect{0, 0, texture.width(), texture.height()};
    } else {
      child_off_x -= scroll_x_offset;
      child_off_y -= scroll_y_offset;
    }

    PaintImpl(child.fragment.get(), texture, child_off_x, child_off_y,
              child_accum_scroll_x, child_accum_scroll_y,
              current_foreground_color, current_background_color,
              current_bold,
              current_underlined,
              current_underlined_double,
              current_strikethrough,
              current_blink,
              child_clip_to_pass);
  }
}
}  // namespace

void Paint(const PhysicalFragment* frag,
           Texture& texture,
           int off_x,
           int off_y) {
  PaintImpl(frag, texture, off_x, off_y, 0, 0, Color::RGB(255, 255, 255),
            Color::RGB(0, 0, 0),
            false,
            false,
            false,
            false,
            false,
            ClipRect{0, 0, texture.width(), texture.height()});
}

}  // namespace rtxui
