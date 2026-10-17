#include "rtxui/paint/paint.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "rtxui/base/string.hpp"
#include "rtxui/color.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/layout/physical_fragment.hpp"
#include "rtxui/layout/sticky.hpp"
#include "rtxui/layout/style.hpp"
#include "rtxui/paint/texture.hpp"

namespace rtxui {

namespace {

// The selection the current Paint() shows and records, if any.
const PaintSelection* g_selection = nullptr;

}  // namespace

namespace {
// The ::selection colors of `node`'s text: those of the nearest element
// declaring some, up from it. None for reverse video.
const GeneratedContents* SelectionColors(const Element* node) {
  for (const Element* element = node; element; element = element->Parent()) {
    if (element->generated && (element->generated->selection_foreground ||
                               element->generated->selection_background)) {
      return element->generated.get();
    }
  }
  return nullptr;
}
}  // namespace

bool TextSelectable(const Element* element) {
  for (; element; element = element->Parent()) {
    if (element->style.user_select) {
      return *element->style.user_select != UserSelect::None;
    }
  }
  return true;
}

namespace {
// The cell at `x`, `y` shows no text any more.
void ForgetGlyph(const Texture& texture, int x, int y) {
  if (g_selection && g_selection->glyphs) {
    (*g_selection->glyphs)[x + (y * texture.width())] = {};
  }
}

// The line glyphs of a box drawing family, by the arms they draw: bit 1 up,
// 2 right, 4 down, 8 left. Null where no glyph has those arms.
using BoxGlyphs = std::array<const char*, 16>;
constexpr BoxGlyphs kLightGlyphs = {
    nullptr, nullptr, nullptr, "└", nullptr, "│", "┌", "├",
    nullptr, "┘",     "─",     "┴", "┐",     "┤", "┬", "┼"};
constexpr BoxGlyphs kHeavyGlyphs = {
    nullptr, nullptr, nullptr, "┗", nullptr, "┃", "┏", "┣",
    nullptr, "┛",     "━",     "┻", "┓",     "┫", "┳", "╋"};
constexpr BoxGlyphs kDoubleGlyphs = {
    nullptr, nullptr, nullptr, "╚", nullptr, "║", "╔", "╠",
    nullptr, "╝",     "═",     "╩", "╗",     "╣", "╦", "╬"};

// The family and arms of a line glyph, or no family. Rounded corners draw the
// arms of light ones.
std::pair<const BoxGlyphs*, int> BoxArms(std::string_view glyph) {
  for (const BoxGlyphs* family :
       {&kLightGlyphs, &kHeavyGlyphs, &kDoubleGlyphs}) {
    for (int arms = 0; arms < 16; ++arms) {
      if ((*family)[arms] && glyph == (*family)[arms]) {
        return {family, arms};
      }
    }
  }
  if (glyph == "╭") {
    return {&kLightGlyphs, 2 | 4};
  }
  if (glyph == "╮") {
    return {&kLightGlyphs, 4 | 8};
  }
  if (glyph == "╰") {
    return {&kLightGlyphs, 1 | 2};
  }
  if (glyph == "╯") {
    return {&kLightGlyphs, 1 | 8};
  }
  return {nullptr, 0};
}

// The glyph drawing `incoming` over `existing` where two collapsed borders
// meet: the lines of both, joined, when they are of one family. `┐` drawn
// over `┌` is `┬`. Otherwise `incoming`, which covers what was there.
const char* MergeBorderGlyph(std::string_view existing, const char* incoming) {
  const auto [family, arms] = BoxArms(incoming);
  const auto [existing_family, existing_arms] = BoxArms(existing);
  if (!family || family != existing_family) {
    return incoming;
  }
  const int merged = arms | existing_arms;
  // Unchanged arms keep their own glyph: a rounded corner stays rounded.
  if (merged == arms || !(*family)[merged]) {
    return incoming;
  }
  return (*family)[merged];
}

const Color kDefaultScrollbarThumbColor = Color::RGBA(200, 200, 200, 200);
const Color kDefaultScrollbarTrackColor = Color::RGBA(80, 80, 80, 120);

// Where each cell of a charset draws its ground, matching Textual's
// BORDER_LOCATIONS exactly. A half-block glyph sits astride the boundary
// between an element and its parent, so which of the two backgrounds fills the
// part the glyph does not cover is what makes the border read as inside the
// element or outside it -- and reversing swaps the strip to the far side of
// the cell, which is how a left-side block draws a right-side strip.
//
//   0 Inner        border color over the element's own background
//   1 Outer        border color over the parent's background
//   2 ReverseOuter reversed, so the parent's background becomes the glyph
//   3 ReverseInner reversed, so the element's background becomes the glyph
enum class Location : uint8_t {
  Inner = 0,
  Outer = 1,
  ReverseOuter = 2,
  ReverseInner = 3,
};

using LocationMap = const Location (*)[3];

constexpr Location I = Location::Inner;
constexpr Location O = Location::Outer;
constexpr Location RO = Location::ReverseOuter;
constexpr Location RI = Location::ReverseInner;

constexpr Location kAllOuter[3][3] = {{O, O, O}, {O, O, O}, {O, O, O}};
// tall and panel: the left column reversed, the right on the parent.
constexpr Location kTall[3][3] = {{RO, I, O}, {RO, I, O}, {RO, I, O}};
// wide: top and bottom on the parent, the right side reversed.
constexpr Location kWide[3][3] = {{O, O, O}, {I, O, RI}, {O, O, O}};
// block: top and bottom on the parent, the sides on the element.
constexpr Location kBlock[3][3] = {{O, O, O}, {I, I, I}, {O, O, O}};

struct BorderData {
  const char* charset[3][3];
  // Null means every cell is Location::Inner, which is all but a few styles.
  LocationMap locations = nullptr;
};

// Map BorderStyle to its character set and locations, from Textual.
const BorderData& GetBorderData(BorderStyle style) {
  static const BorderData border_styles[] = {
      /* Ascii */
      {
          {
              {"+", "-", "+"},
              {"|", " ", "|"},
              {"+", "-", "+"},
          },
      },
      /* Blank */
      {
          {
              {" ", " ", " "},
              {" ", " ", " "},
              {" ", " ", " "},
          },
      },
      /* Dashed */
      {
          {
              {"┌", "╌", "┐"},
              {"╎", " ", "╎"},
              {"└", "╌", "┘"},
          },
      },
      /* Double */
      {
          {
              {"╔", "═", "╗"},
              {"║", " ", "║"},
              {"╚", "═", "╝"},
          },
      },
      /* HKey */
      {
          {
              {"▔", "▔", "▔"},
              {" ", " ", " "},
              {"▁", "▁", "▁"},
          },
      },
      /* Heavy */
      {
          {
              {"┏", "━", "┓"},
              {"┃", " ", "┃"},
              {"┗", "━", "┛"},
          },
      },
      /* Inner */
      {
          {
              {"▗", "▄", "▖"},
              {"▐", " ", "▌"},
              {"▝", "▀", "▘"},
          },
          kAllOuter,
      },
      /* None */
      {
          {
              {" ", " ", " "},
              {" ", " ", " "},
              {" ", " ", " "},
          },
      },
      /* Outer */
      {
          {
              {"▛", "▀", "▜"},
              {"▌", " ", "▐"},
              {"▙", "▄", "▟"},
          },
      },
      /* Panel */
      {
          {
              {"▊", "█", "▎"},
              {"▊", " ", "▎"},
              {"▊", "▁", "▎"},
          },
          kTall,
      },
      /* Round */
      {
          {
              {"╭", "─", "╮"},
              {"│", " ", "│"},
              {"╰", "─", "╯"},
          },
      },
      /* Solid */
      {
          {
              {"┌", "─", "┐"},
              {"│", " ", "│"},
              {"└", "─", "┘"},
          },
      },
      /* Tall */
      {
          {
              {"▊", "▔", "▎"},
              {"▊", " ", "▎"},
              {"▊", "▁", "▎"},
          },
          kTall,
      },
      /* Thick */
      {
          {
              {"█", "▀", "█"},
              {"█", " ", "█"},
              {"█", "▄", "█"},
          },
      },
      /* VKey */
      {
          {
              {"▏", " ", "▕"},
              {"▏", " ", "▕"},
              {"▏", " ", "▕"},
          },
      },
      /* Wide */
      {
          {
              {"▁", "▁", "▁"},
              {"▎", " ", "▊"},
              {"▔", "▔", "▔"},
          },
          kWide,
      },
      /* Dotted */
      {
          {
              {"┌", "┈", "┐"},
              {"┊", " ", "┊"},
              {"└", "┈", "┘"},
          },
      },
      /* DoubleHorizontal */
      {
          {
              {"╒", "═", "╕"},
              {"│", " ", "│"},
              {"╘", "═", "╛"},
          },
      },
      /* DoubleVertical */
      {
          {
              {"╓", "─", "╖"},
              {"║", " ", "║"},
              {"╙", "─", "╜"},
          },
      },
      /* Shadow */
      {
          {
              {"░", "░", "▓"},
              {"░", " ", "▓"},
              {"░", "▓", "▓"},
          },
      },
      /* ShadeLight */
      {
          {
              {"░", "░", "░"},
              {"░", " ", "░"},
              {"░", "░", "░"},
          },
      },
      /* ShadeMedium */
      {
          {
              {"▒", "▒", "▒"},
              {"▒", " ", "▒"},
              {"▒", "▒", "▒"},
          },
      },
      /* ShadeDark */
      {
          {
              {"▓", "▓", "▓"},
              {"▓", " ", "▓"},
              {"▓", "▓", "▓"},
          },
      },
      /* Block */
      {
          {
              {"▄", "▄", "▄"},
              {"█", " ", "█"},
              {"▀", "▀", "▀"},
          },
          kBlock,
      },
      /* HeavyDashed */
      {
          {
              {"┏", "╍", "┓"},
              {"╏", " ", "╏"},
              {"┗", "╍", "┛"},
          },
      },
      /* Inset */
      {
          {
              {"┌", "─", "┐"},
              {"│", " ", "│"},
              {"└", "─", "┘"},
          },
      },
      /* Outset */
      {
          {
              {"┌", "─", "┐"},
              {"│", " ", "│"},
              {"└", "─", "┘"},
          },
      },
  };
  static_assert(sizeof(border_styles) / sizeof(border_styles[0]) ==
                static_cast<size_t>(BorderStyle::Outset) + 1);
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

// An element's outline, as found while painting the element. It is drawn
// once everything else is, so that no element painted later covers it.
struct PendingOutline {
  BorderStyle style;
  int left;
  int top;
  int right;
  int bottom;
  Color color;
  ClipRect clip;
};

thread_local std::vector<PendingOutline> g_pending_outlines;

// Notes the outline of the element painted at `x`, `y`, `w`, `h`: its outline
// style's glyphs on a ring one cell outside its border box, `outline-offset`
// further out (or in, when negative). It takes no room in the layout, so it
// overlaps the margins and neighbours around the element.
void QueueOutline(const ComputedStyleCore& style,
                  int x,
                  int y,
                  int w,
                  int h,
                  Color foreground,
                  float opacity,
                  const ClipRect& clip) {
  const int left = x - 1 - style.outline_offset;
  const int top = y - 1 - style.outline_offset;
  const int right = x + w + style.outline_offset;
  const int bottom = y + h + style.outline_offset;
  // A negative offset larger than half the box leaves no ring to draw.
  if (right <= left || bottom <= top) {
    return;
  }
  Color color = style.outline_color.value_or(foreground);
  color.a = static_cast<uint8_t>(color.a * opacity);
  g_pending_outlines.push_back(
      {style.outline_style, left, top, right, bottom, color, clip});
}

void PaintOutline(const PendingOutline& outline, Texture& texture) {
  const auto [style, left, top, right, bottom, color, clip] = outline;
  const auto& charset = GetBorderData(style).charset;
  auto set_char = [&](int cx, int cy, int row, int col) {
    const char* c = charset[row][col];
    if (c == nullptr || *c == '\0' || *c == ' ' || cx < 0 ||
        cx >= texture.width() || cy < 0 || cy >= texture.height() ||
        !clip.Contains(cx, cy)) {
      return;
    }
    auto& cell = texture[cx, cy];
    if (cell.inverted) {
      std::swap(cell.background_color, cell.foreground_color);
      cell.inverted = false;
    }
    cell.character = c;
    cell.link = 0;
    ForgetGlyph(texture, cx, cy);
    cell.foreground_color = Blend(color, cell.background_color);
  };
  set_char(left, top, 0, 0);
  set_char(right, top, 0, 2);
  set_char(left, bottom, 2, 0);
  set_char(right, bottom, 2, 2);
  for (int i = left + 1; i < right; ++i) {
    set_char(i, top, 0, 1);
    set_char(i, bottom, 2, 1);
  }
  for (int i = top + 1; i < bottom; ++i) {
    set_char(left, i, 1, 0);
    set_char(right, i, 1, 2);
  }
}

std::atomic<int> g_link_elements = 0;

// The URL a text is a link to: the `href` of the `<a>` around it, when it
// leaves the app. A `#id` target scrolls within it instead, and a relative
// path means nothing to a terminal, so only a URL with a scheme
// (`https:`, `mailto:`, `file:`) is one.
std::string_view ExternalLink(const Element* element) {
  for (; element; element = element->Parent()) {
    if (element->tag() != "a") {
      continue;
    }
    const std::string* href = element->GetAttribute("href");
    if (!href) {
      return {};
    }
    const size_t colon = href->find(':');
    if (colon == std::string::npos || colon == 0 ||
        !std::isalpha(static_cast<unsigned char>((*href)[0])) ||
        !std::all_of(href->begin(), href->begin() + colon, [](char c) {
          return std::isalnum(static_cast<unsigned char>(c)) || c == '+' ||
                 c == '-' || c == '.';
        })) {
      return {};
    }
    return *href;
  }
  return {};
}

// Whether `frag` is a positioned box with a positive z-index. Such a box is
// painted above everything else in its stacking context -- the root, or the
// nearest such box above it -- not only above its siblings: a <select>'s list
// must cover the next field even when each sits in a <div> of its own.
bool IsRaised(const PhysicalFragment* frag) {
  return frag && frag->dom_node && frag->dom_node->style.PaintsAsPositioned() &&
         frag->dom_node->style.z_index.value_or(0) > 0;
}

// The raised boxes of each stacking context being painted, innermost last:
// each with the call that paints it, in tree order.
struct RaisedPaint {
  int z_index;
  std::function<void()> paint;
};
std::vector<std::vector<RaisedPaint>> g_raised;

void PaintImpl(const PhysicalFragment* frag,
               Texture& texture,
               int off_x,
               int off_y,
               int accum_scroll_x,
               int accum_scroll_y,
               int viewport_x,
               int viewport_y,
               int viewport_w,
               int viewport_h,
               Color inherited_foreground_color,
               Color parent_background_color,
               bool inherited_bold,
               bool inherited_dim,
               bool inherited_italic,
               bool inherited_underlined,
               bool inherited_underlined_double,
               bool inherited_strikethrough,
               bool inherited_overlined,
               bool inherited_blink,
               ClipRect clip,
               float inherited_opacity) {
  if (frag->visibility == Visibility::Hidden) {
    return;
  }

  // The root and every raised box start a stacking context: the raised boxes
  // inside it are painted once the rest of it is, lowest z-index first.
  const bool stacking_context = g_raised.empty() || IsRaised(frag);
  if (stacking_context) {
    g_raised.emplace_back();
  }
  int abs_x = off_x;
  int abs_y = off_y;
  int w = frag->width;
  int h = frag->height;

  float current_opacity = inherited_opacity * frag->opacity;

  auto resolve_cell = [](Cell& cell) {
    if (cell.inverted) {
      std::swap(cell.background_color, cell.foreground_color);
      cell.inverted = false;
    }
  };

  if (frag->dom_node) {
    const_cast<Element*>(frag->dom_node)->set_absolute_position(abs_x, abs_y);
  }

  Color current_foreground_color =
      frag->foreground_color.value_or(inherited_foreground_color);
  Color current_background_color =
      frag->background_color
          ? Blend(*frag->background_color, parent_background_color)
          : parent_background_color;
  bool current_bold = frag->bold.value_or(inherited_bold);
  bool current_dim = frag->dim.value_or(inherited_dim);
  bool current_italic = frag->italic.value_or(inherited_italic);
  bool current_underlined = frag->underlined.value_or(inherited_underlined);
  bool current_underlined_double =
      frag->underlined_double.value_or(inherited_underlined_double);
  bool current_strikethrough =
      frag->strikethrough.value_or(inherited_strikethrough);
  bool current_overlined = frag->overlined.value_or(inherited_overlined);
  bool current_blink = frag->blink.value_or(inherited_blink);

  // 0. Draw Background
  if (frag->background_color) {
    Color new_bg = frag->background_color.value();
    new_bg.a = static_cast<uint8_t>(new_bg.a * current_opacity);
    if (new_bg.a > 0) {
      for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
          int tex_x = abs_x + x;
          int tex_y = abs_y + y;
          if (tex_x >= 0 && tex_x < texture.width() && tex_y >= 0 &&
              tex_y < texture.height() && clip.Contains(tex_x, tex_y)) {
            auto& cell = texture[tex_x, tex_y];
            resolve_cell(cell);
            if (new_bg.a == 255) {
              // An opaque background hides whatever was painted beneath it,
              // glyphs included: a dialog box must not show the text it
              // covers through its blank cells.
              const bool was_continuation = cell.is_continuation;
              // Except, for a cell of a table with collapsed borders, the
              // border lines under its own: they are shared, and its border
              // joins them.
              const bool shared_border =
                  frag->merge_borders &&
                  (x == 0 || y == 0 || x == w - 1 || y == h - 1) &&
                  BoxArms(cell.character).first;
              std::string kept;
              if (shared_border) {
                kept = std::move(cell.character);
              }
              cell = Cell();
              ForgetGlyph(texture, tex_x, tex_y);
              cell.background_color = new_bg;
              cell.foreground_color = new_bg;
              if (shared_border) {
                cell.character = std::move(kept);
              }
              // A wide glyph cut in half by the box's edge would still be
              // drawn two cells wide, over the box or past it: blank the
              // half left outside.
              if (was_continuation && tex_x > 0) {
                texture[tex_x - 1, tex_y].character = " ";
              }
              if (tex_x + 1 < texture.width() &&
                  texture[tex_x + 1, tex_y].is_continuation) {
                auto& next = texture[tex_x + 1, tex_y];
                next.is_continuation = false;
                next.character = " ";
              }
              continue;
            }
            cell.background_color = Blend(new_bg, cell.background_color);
            cell.foreground_color = Blend(new_bg, cell.foreground_color);
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

    // inset and outset light one pair of sides and shade the other, each half
    // way from its own color to white or black. The terminal's default
    // foreground cannot be mixed, so it stands in as a mid grey.
    if (frag->border_style == BorderStyle::Inset ||
        frag->border_style == BorderStyle::Outset) {
      auto shade = [](Color c, int toward) {
        if (c.a == 0) {
          c = Color::RGB(170, 170, 170);
        }
        return Color::RGBA((c.r + toward) / 2, (c.g + toward) / 2,
                           (c.b + toward) / 2, c.a);
      };
      const bool outset = frag->border_style == BorderStyle::Outset;
      const int lit = outset ? 255 : 0;
      const int shaded = outset ? 0 : 255;
      top_border_color = shade(top_border_color, lit);
      left_border_color = shade(left_border_color, lit);
      bottom_border_color = shade(bottom_border_color, shaded);
      right_border_color = shade(right_border_color, shaded);
    }

    top_border_color.a =
        static_cast<uint8_t>(top_border_color.a * current_opacity);
    right_border_color.a =
        static_cast<uint8_t>(right_border_color.a * current_opacity);
    bottom_border_color.a =
        static_cast<uint8_t>(bottom_border_color.a * current_opacity);
    left_border_color.a =
        static_cast<uint8_t>(left_border_color.a * current_opacity);

    const auto& data = GetBorderData(frag->border_style);

    // One cell of the charset: the glyph and where it draws its ground.
    struct Glyph {
      const char* c;
      Location location;
    };
    auto glyph = [&data](int row, int col) -> Glyph {
      return {data.charset[row][col],
              data.locations ? data.locations[row][col] : Location::Inner};
    };

    auto set_char = [&](int x, int y, Glyph g, const Color& border_color) {
      const char* c = g.c;
      if (c == nullptr || *c == '\0' || *c == ' ') {
        return;
      }
      if (x >= 0 && x < texture.width() && y >= 0 && y < texture.height() &&
          clip.Contains(x, y)) {
        auto& cell = texture[x, y];
        resolve_cell(cell);
        if (frag->merge_borders) {
          c = MergeBorderGlyph(cell.character, c);
        }
        cell.character = c;
        cell.link = 0;
        ForgetGlyph(texture, x, y);

        // The glyph is always drawn in the border color. What varies is which
        // background fills the rest of the cell -- the element's own, or its
        // parent's -- and whether the two are swapped.
        //
        // Inner keeps whatever step 0 laid down, which is the element's own
        // background composited over everything beneath it. Outer replaces it
        // with the parent's, which is what makes `tall`'s right edge and
        // `inner`'s whole ring read as sitting outside the element.
        const bool outer = g.location == Location::Outer ||
                           g.location == Location::ReverseOuter;
        const Color field =
            outer ? parent_background_color : cell.background_color;

        cell.foreground_color = Blend(border_color, field);
        cell.background_color = field;

        const bool reversed = g.location == Location::ReverseOuter ||
                              g.location == Location::ReverseInner;
        if (reversed) {
          // Swap here when the field is a real color, exactly as Textual does.
          // Anything painted over this cell later then composites against the
          // colors it will actually be drawn in, and terminals that render
          // reverse video with a palette of their own are kept out of it.
          //
          // A transparent field cannot be swapped here: it has no foreground
          // spelling. ESC[39m is the default *foreground*, so putting it in
          // the glyph's slot painted a white block down the left of every
          // uncolored `tall` box. Ask the terminal to do the swap instead, and
          // it performs it against its own background -- the color we cannot
          // name. Setting a screen background is what makes this branch the
          // rare one.
          if (field.a == 255) {
            std::swap(cell.foreground_color, cell.background_color);
          } else {
            cell.inverted = true;
          }
        }
      }
    };

    bool has_top = frag->border_width.top > 0;
    bool has_right = frag->border_width.right > 0;
    bool has_bottom = frag->border_width.bottom > 0;
    bool has_left = frag->border_width.left > 0;

    // A corner only gets a corner glyph when both of its adjacent sides are
    // present; with just one adjacent side, that side's own line character
    // extends flush to the edge instead (matching how a single `border-top`/
    // `-right`/`-bottom`/`-left` renders as a plain line, not a partial box).
    // When both sides are present, the corner glyph keeps the *vertical*
    // (left/right, `b_color`) side's color, matching the pre-existing
    // behavior for ordinary 4-sided borders (verified by "Individual Border
    // Colors": corners take the color of the vertical border).
    auto draw_corner = [&](int x, int y, bool side_a, bool side_b, Glyph corner,
                           Glyph a, const Color& a_color, Glyph b,
                           const Color& b_color) {
      if (side_a && side_b) {
        set_char(x, y, corner, b_color);
      } else if (side_a) {
        set_char(x, y, a, a_color);
      } else if (side_b) {
        set_char(x, y, b, b_color);
      }
    };

    // Corners
    draw_corner(abs_x, abs_y, has_top, has_left, glyph(0, 0), glyph(0, 1),
                top_border_color, glyph(1, 0), left_border_color);  // tl
    draw_corner(abs_x + w - 1, abs_y, has_top, has_right, glyph(0, 2),
                glyph(0, 1), top_border_color, glyph(1, 2),
                right_border_color);  // tr
    draw_corner(abs_x, abs_y + h - 1, has_bottom, has_left, glyph(2, 0),
                glyph(2, 1), bottom_border_color, glyph(1, 0),
                left_border_color);  // bl
    draw_corner(abs_x + w - 1, abs_y + h - 1, has_bottom, has_right,
                glyph(2, 2), glyph(2, 1), bottom_border_color, glyph(1, 2),
                right_border_color);  // br

    // Top/Bottom
    for (int i = 1; i < w - 1; ++i) {
      if (has_top) {
        set_char(abs_x + i, abs_y, glyph(0, 1), top_border_color);  // t
      }
      if (has_bottom) {
        set_char(abs_x + i, abs_y + h - 1, glyph(2, 1),
                 bottom_border_color);  // b
      }
    }

    // Left/Right
    for (int i = 1; i < h - 1; ++i) {
      if (has_left) {
        set_char(abs_x, abs_y + i, glyph(1, 0), left_border_color);  // l
      }
      if (has_right) {
        set_char(abs_x + w - 1, abs_y + i, glyph(1, 2),
                 right_border_color);  // r
      }
    }
  }

  // 2. Draw Text
  if (frag->is_text) {
    // Only while a selection is shown or recorded: whose text this is.
    const Element* selection_node =
        g_selection && frag->text_node && TextSelectable(frag->text_node)
            ? frag->text_node
            : nullptr;
    const uint16_t link = g_link_elements.load(std::memory_order_relaxed) > 0
                              ? texture.AddLink(ExternalLink(frag->dom_node))
                              : 0;
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
        resolve_cell(cell);
        cell.character = std::string(g.text);
        cell.link = link;
        bool highlighted = false;
        if (g_selection) {
          TextPosition glyph;
          if (selection_node) {
            glyph = {selection_node,
                     frag->text_offset +
                         static_cast<uint32_t>(g.text.data() -
                                               frag->text_content.data()),
                     frag->after_wrap && cell_x == 0};
          }
          if (g_selection->glyphs) {
            (*g_selection->glyphs)[x + (y * texture.width())] = glyph;
          }
          highlighted = glyph.node && g_selection->selected &&
                        g_selection->selected(glyph);
        }
        Color fg = current_foreground_color;
        fg.a = static_cast<uint8_t>(fg.a * current_opacity);
        if (highlighted) {
          // In the colors ::selection gives, else in reverse video.
          const GeneratedContents* colors = SelectionColors(selection_node);
          if (!colors) {
            cell.inverted = true;
          } else {
            if (colors->selection_background) {
              cell.background_color =
                  Blend(*colors->selection_background, cell.background_color);
            }
            fg = colors->selection_foreground.value_or(fg);
          }
        }
        Color bg = cell.background_color;
        cell.foreground_color = Blend(fg, bg);
        cell.bold = current_bold;
        cell.dim = current_dim;
        cell.italic = current_italic;
        cell.underlined = current_underlined;
        cell.underlined_double = current_underlined_double;
        cell.strikethrough = current_strikethrough;
        cell.overlined = current_overlined;
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

  // Each side's border, which the scrollbars sit inside.
  Spacing border;
  if (frag->has_border && frag->border_style != BorderStyle::None) {
    border = frag->border_width;
  }

  if (draw_v_scrollbar) {
    int scrollbar_x = abs_x + w - border.right - 1;
    int track_y_start = abs_y + border.top;
    int track_h = h - border.Vert() - (draw_h_scrollbar ? 1 : 0);

    if (track_h > 0 && scrollbar_x >= 0 && scrollbar_x < texture.width()) {
      int scroll_height = frag->dom_node->scroll_height();
      int padding_vert = frag->dom_node->style.padding.Vert();
      int viewport_h = std::max(1, h - border.Vert() - padding_vert);

      int thumb_h_eighths =
          (viewport_h * track_h * 8) / std::max(1, scroll_height);
      thumb_h_eighths = std::max(8, thumb_h_eighths);
      thumb_h_eighths = std::min(track_h * 8, thumb_h_eighths);

      int max_scroll = scroll_height - h;
      int thumb_y_eighths =
          (max_scroll > 0)
              ? static_cast<int>(std::round(frag->visual_scroll_y *
                                            ((track_h * 8) - thumb_h_eighths) /
                                            max_scroll))
              : 0;

      Color thumb_bg =
          (frag->dom_node && frag->dom_node->style.has_scrollbar_color_thumb)
              ? frag->dom_node->style.scrollbar_color_thumb
              : kDefaultScrollbarThumbColor;
      Color track_bg =
          (frag->dom_node && frag->dom_node->style.has_scrollbar_color_track)
              ? frag->dom_node->style.scrollbar_color_track
              : kDefaultScrollbarTrackColor;

      thumb_bg.a = static_cast<uint8_t>(thumb_bg.a * current_opacity);
      track_bg.a = static_cast<uint8_t>(track_bg.a * current_opacity);

      const char* lower_blocks[] = {
          " ",
          " ",  // U+2581
          "▂",  // U+2582
          "▃",  // U+2583
          "▄",  // U+2584
          "▅",  // U+2585
          "▆",  // U+2586
          "▇",  // U+2587
          "█"   // U+2588
      };

      for (int i = 0; i < track_h; ++i) {
        int y = track_y_start + i;
        if (y >= 0 && y < texture.height() && clip.Contains(scrollbar_x, y)) {
          auto& cell = texture[scrollbar_x, y];
          resolve_cell(cell);
          cell.link = 0;
          ForgetGlyph(texture, scrollbar_x, y);

          int cell_start = i * 8;
          int cell_end = (i + 1) * 8;

          int start_eighth = std::max(cell_start, thumb_y_eighths);
          int end_eighth =
              std::min(cell_end, thumb_y_eighths + thumb_h_eighths);

          if (start_eighth >= end_eighth) {
            cell.character = " ";
            cell.background_color = Blend(track_bg, cell.background_color);
          } else {
            int t_start = start_eighth - cell_start;
            int t_end = end_eighth - cell_start;

            if (t_start == 0 && t_end == 8) {
              cell.character = " ";
              cell.background_color = Blend(thumb_bg, cell.background_color);
            } else if (t_start > 0 && t_end == 8) {
              int thumb_len = 8 - t_start;
              cell.character = lower_blocks[thumb_len];
              const Color original_bg = cell.background_color;
              cell.background_color = Blend(track_bg, original_bg);
              cell.foreground_color = Blend(thumb_bg, original_bg);
            } else if (t_start == 0 && t_end < 8) {
              int track_len = 8 - t_end;
              cell.character = lower_blocks[track_len];
              const Color original_bg = cell.background_color;
              cell.background_color = Blend(thumb_bg, original_bg);
              cell.foreground_color = Blend(track_bg, original_bg);
            } else {
              cell.character = " ";
              cell.background_color = Blend(thumb_bg, cell.background_color);
            }
          }
        }
      }
    }
  }

  if (draw_h_scrollbar) {
    int scrollbar_y = abs_y + h - border.bottom - 1;
    int track_x_start = abs_x + border.left;
    int track_w = w - border.Horiz() - (draw_v_scrollbar ? 1 : 0);

    if (track_w > 0 && scrollbar_y >= 0 && scrollbar_y < texture.height()) {
      int scroll_width = frag->dom_node->scroll_width();
      int padding_horiz = frag->dom_node->style.padding.Horiz();
      int viewport_w = std::max(1, w - border.Horiz() - padding_horiz);

      int thumb_w_eighths =
          (viewport_w * track_w * 8) / std::max(1, scroll_width);
      thumb_w_eighths = std::max(8, thumb_w_eighths);
      thumb_w_eighths = std::min(track_w * 8, thumb_w_eighths);

      int max_scroll = scroll_width - w;
      int thumb_x_eighths =
          (max_scroll > 0)
              ? static_cast<int>(std::round(frag->visual_scroll_x *
                                            ((track_w * 8) - thumb_w_eighths) /
                                            max_scroll))
              : 0;

      Color thumb_bg =
          (frag->dom_node && frag->dom_node->style.has_scrollbar_color_thumb)
              ? frag->dom_node->style.scrollbar_color_thumb
              : kDefaultScrollbarThumbColor;
      Color track_bg =
          (frag->dom_node && frag->dom_node->style.has_scrollbar_color_track)
              ? frag->dom_node->style.scrollbar_color_track
              : kDefaultScrollbarTrackColor;

      thumb_bg.a = static_cast<uint8_t>(thumb_bg.a * current_opacity);
      track_bg.a = static_cast<uint8_t>(track_bg.a * current_opacity);

      const char* left_blocks[] = {
          " ",
          "▏",  // U+258F
          "▎",  // U+258E
          "▍",  // U+258D
          "▌",  // U+258C
          "▋",  // U+258B
          "▊",  // U+258A
          "▉",  // U+2589
          "█"   // U+2588
      };

      for (int i = 0; i < track_w; ++i) {
        int x = track_x_start + i;
        if (x >= 0 && x < texture.width() && clip.Contains(x, scrollbar_y)) {
          auto& cell = texture[x, scrollbar_y];
          resolve_cell(cell);
          cell.link = 0;
          ForgetGlyph(texture, x, scrollbar_y);

          int cell_start = i * 8;
          int cell_end = (i + 1) * 8;

          int start_eighth = std::max(cell_start, thumb_x_eighths);
          int end_eighth =
              std::min(cell_end, thumb_x_eighths + thumb_w_eighths);

          if (start_eighth >= end_eighth) {
            cell.character = " ";
            cell.background_color = Blend(track_bg, cell.background_color);
          } else {
            int t_start = start_eighth - cell_start;
            int t_end = end_eighth - cell_start;

            if (t_start == 0 && t_end == 8) {
              cell.character = " ";
              cell.background_color = Blend(thumb_bg, cell.background_color);
            } else if (t_start > 0 && t_end == 8) {
              cell.character = left_blocks[t_start];
              const Color original_bg = cell.background_color;
              cell.background_color = Blend(thumb_bg, original_bg);
              cell.foreground_color = Blend(track_bg, original_bg);
            } else if (t_start == 0 && t_end < 8) {
              cell.character = left_blocks[t_end];
              const Color original_bg = cell.background_color;
              cell.background_color = Blend(track_bg, original_bg);
              cell.foreground_color = Blend(thumb_bg, original_bg);
            } else {
              cell.character = " ";
              cell.background_color = Blend(thumb_bg, cell.background_color);
            }
          }
        }
      }
    }
  }

  // 4. Recurse
  ClipRect child_clip = clip;
  int scroll_x_offset = 0;
  int scroll_y_offset = 0;
  int border_l = 0, border_r = 0, border_t = 0, border_b = 0;
  int padding_l = 0, padding_r = 0, padding_t = 0, padding_b = 0;

  if (frag->dom_node) {
    padding_l = frag->dom_node->style.padding.left;
    padding_r = frag->dom_node->style.padding.right;
    padding_t = frag->dom_node->style.padding.top;
    padding_b = frag->dom_node->style.padding.bottom;
  }

  if (frag->has_border && frag->border_style != BorderStyle::None) {
    border_l = frag->border_width.left;
    border_r = frag->border_width.right;
    border_t = frag->border_width.top;
    border_b = frag->border_width.bottom;
  }

  if (frag->clips_descendants) {
    scroll_x_offset = frag->scroll_x;
    scroll_y_offset = frag->scroll_y;

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
  std::stable_sort(
      sorted_children.begin(), sorted_children.end(),
      [](const PhysicalFragment::ChildLink& a,
         const PhysicalFragment::ChildLink& b) {
        bool a_pos = (a.fragment && a.fragment->dom_node &&
                      a.fragment->dom_node->style.PaintsAsPositioned());
        bool b_pos = (b.fragment && b.fragment->dom_node &&
                      b.fragment->dom_node->style.PaintsAsPositioned());
        int az = (a.fragment && a.fragment->dom_node)
                     ? a.fragment->dom_node->style.z_index.value_or(0)
                     : 0;
        int bz = (b.fragment && b.fragment->dom_node)
                     ? b.fragment->dom_node->style.z_index.value_or(0)
                     : 0;
        if (az != bz) {
          return az < bz;
        }
        if (a_pos != b_pos) {
          return !a_pos && b_pos;
        }
        return false;
      });

  int next_accum_scroll_x = accum_scroll_x + scroll_x_offset;
  int next_accum_scroll_y = accum_scroll_y + scroll_y_offset;

  int next_viewport_x = viewport_x;
  int next_viewport_y = viewport_y;
  int next_viewport_w = viewport_w;
  int next_viewport_h = viewport_h;
  if (frag->clips_descendants) {
    next_viewport_x = abs_x + border_l + padding_l;
    next_viewport_y = abs_y + border_t + padding_t;
    next_viewport_w = w - border_l - border_r - padding_l - padding_r;
    next_viewport_h = h - border_t - border_b - padding_t - padding_b;
  }

  for (auto& child : sorted_children) {
    bool is_fixed =
        (child.fragment && child.fragment->dom_node &&
         child.fragment->dom_node->style.position == PositionType::Fixed);

    int child_off_x = abs_x + child.x;
    int child_off_y = abs_y + child.y;
    int child_accum_scroll_x = next_accum_scroll_x;
    int child_accum_scroll_y = next_accum_scroll_y;
    ClipRect child_clip_to_pass = child_clip;

    bool is_sticky =
        (child.fragment && child.fragment->dom_node &&
         child.fragment->dom_node->style.position == PositionType::Sticky);

    if (is_fixed) {
      child_off_x += accum_scroll_x;
      child_off_y += accum_scroll_y;
      child_accum_scroll_x = 0;
      child_accum_scroll_y = 0;
      child_clip_to_pass = ClipRect{0, 0, texture.width(), texture.height()};
    } else {
      child_off_x -= scroll_x_offset;
      child_off_y -= scroll_y_offset;

      if (is_sticky) {
        StickyContext sticky;
        sticky.parent_clips = frag->clips_descendants;
        sticky.viewport_x =
            frag->clips_descendants ? next_viewport_x : viewport_x;
        sticky.viewport_y =
            frag->clips_descendants ? next_viewport_y : viewport_y;
        sticky.viewport_width =
            frag->clips_descendants ? next_viewport_w : viewport_w;
        sticky.viewport_height =
            frag->clips_descendants ? next_viewport_h : viewport_h;
        sticky.parent_x = abs_x;
        sticky.parent_y = abs_y;
        sticky.parent_width = w;
        sticky.parent_height = h;
        sticky.parent_border_left = border_l;
        sticky.parent_border_top = border_t;
        sticky.parent_border_right = border_r;
        sticky.parent_border_bottom = border_b;
        sticky.parent_padding_left = padding_l;
        sticky.parent_padding_top = padding_t;
        sticky.parent_padding_right = padding_r;
        sticky.parent_padding_bottom = padding_b;
        ApplyStickyOffset(child.fragment->dom_node->style,
                          child.fragment->width, child.fragment->height, sticky,
                          child_off_x, child_off_y);
      }
    }

    auto paint_child = [=, &texture, fragment = child.fragment] {
      PaintImpl(
          fragment, texture, child_off_x, child_off_y, child_accum_scroll_x,
          child_accum_scroll_y, is_fixed ? viewport_x : next_viewport_x,
          is_fixed ? viewport_y : next_viewport_y,
          is_fixed ? viewport_w : next_viewport_w,
          is_fixed ? viewport_h : next_viewport_h, current_foreground_color,
          current_background_color, current_bold, current_dim, current_italic,
          current_underlined, current_underlined_double, current_strikethrough,
          current_overlined, current_blink, child_clip_to_pass,
          current_opacity);
    };
    if (IsRaised(child.fragment)) {
      g_raised.back().push_back(
          {child.fragment->dom_node->style.z_index.value_or(0), paint_child});
    } else {
      paint_child();
    }
  }

  // 5. Outline, drawn once the whole tree is painted.
  if (frag->dom_node && !frag->is_text &&
      frag->dom_node->style.outline_style != BorderStyle::None) {
    QueueOutline(frag->dom_node->style, abs_x, abs_y, w, h,
                 current_foreground_color, current_opacity, clip);
  }

  // 6. The raised boxes of the stacking context this box starts.
  if (stacking_context) {
    std::vector<RaisedPaint> raised = std::move(g_raised.back());
    g_raised.pop_back();
    std::ranges::stable_sort(raised, {}, &RaisedPaint::z_index);
    for (const RaisedPaint& box : raised) {
      box.paint();
    }
  }
}
}  // namespace

void Paint(const PhysicalFragment* frag,
           Texture& texture,
           int off_x,
           int off_y,
           Color screen_background,
           const PaintSelection* selection,
           Color screen_foreground) {
  g_pending_outlines.clear();
  g_selection = selection;
  if (selection && selection->glyphs) {
    selection->glyphs->assign(
        static_cast<size_t>(texture.width()) * texture.height(), {});
  }
  // The ground every cell starts from, so that a translucent color painted
  // straight onto the screen blends with it rather than with nothing.
  if (screen_background.a != 0) {
    for (int y = 0; y < texture.height(); ++y) {
      for (int x = 0; x < texture.width(); ++x) {
        texture[x, y].background_color = screen_background;
      }
    }
  }
  PaintImpl(frag, texture, off_x, off_y, 0, 0, off_x, off_y, texture.width(),
            texture.height(), screen_foreground, screen_background, false,
            false, false, false, false, false, false, false,
            ClipRect{0, 0, texture.width(), texture.height()}, 1.0f);
  // Outlines go over everything, in the order their elements were painted.
  for (const PendingOutline& outline : g_pending_outlines) {
    PaintOutline(outline, texture);
  }
  g_pending_outlines.clear();
  g_selection = nullptr;
}

void CountLinkElements(int delta) {
  g_link_elements.fetch_add(delta, std::memory_order_relaxed);
}

}  // namespace rtxui
