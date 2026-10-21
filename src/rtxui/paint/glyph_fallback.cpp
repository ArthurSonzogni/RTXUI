// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/paint/glyph_fallback.hpp"

#include <algorithm>
#include <string>
#include <string_view>

#include "rtxui/base/string.hpp"
#include "rtxui/paint/cell.hpp"
#include "rtxui/paint/texture.hpp"

namespace rtxui {

namespace {

struct Entry {
  char32_t glyph;
  // Tried in order; the last is ASCII, which every terminal draws.
  std::u32string_view replacements;
};

// Every glyph RTXUI draws on its own. A block replacing another keeps its
// side: the left column of `border: tall` is a ▊ drawn reversed, so only its
// right quarter shows the border color, and ▌ keeps a strip there where █
// would hide it.
constexpr Entry kEntries[] = {
    // Light and double box drawing.
    {U'─', U"-"},
    {U'│', U"|"},
    {U'┌', U"+"},
    {U'┐', U"+"},
    {U'└', U"+"},
    {U'┘', U"+"},
    {U'├', U"+"},
    {U'┤', U"+"},
    {U'┬', U"+"},
    {U'┴', U"+"},
    {U'┼', U"+"},
    {U'═', U"="},
    {U'║', U"|"},
    {U'╒', U"+"},
    {U'╓', U"+"},
    {U'╔', U"+"},
    {U'╕', U"+"},
    {U'╖', U"+"},
    {U'╗', U"+"},
    {U'╘', U"+"},
    {U'╙', U"+"},
    {U'╚', U"+"},
    {U'╛', U"+"},
    {U'╜', U"+"},
    {U'╝', U"+"},
    {U'╠', U"+"},
    {U'╣', U"+"},
    {U'╦', U"+"},
    {U'╩', U"+"},
    {U'╬', U"+"},
    // Heavy, rounded and dashed: their light, square, solid kin.
    {U'━', U"─-"},
    {U'┃', U"│|"},
    {U'┍', U"┌+"},
    {U'┎', U"┌+"},
    {U'┏', U"┌+"},
    {U'┑', U"┐+"},
    {U'┒', U"┐+"},
    {U'┓', U"┐+"},
    {U'┕', U"└+"},
    {U'┖', U"└+"},
    {U'┗', U"└+"},
    {U'┙', U"┘+"},
    {U'┚', U"┘+"},
    {U'┛', U"┘+"},
    {U'┣', U"├+"},
    {U'┫', U"┤+"},
    {U'┳', U"┬+"},
    {U'┻', U"┴+"},
    {U'╋', U"┼+"},
    {U'╭', U"┌+"},
    {U'╮', U"┐+"},
    {U'╯', U"┘+"},
    {U'╰', U"└+"},
    {U'┈', U"─-"},
    {U'┊', U"│|"},
    {U'╌', U"─-"},
    {U'╍', U"━─-"},
    {U'╎', U"│|"},
    {U'╏', U"┃│|"},
    // Blocks and shades.
    {U'█', U"#"},
    {U'▀', U"▔-"},
    {U'▄', U"▁_"},
    {U'▌', U"▎|"},
    {U'▐', U"▕|"},
    {U'░', U"▒:"},
    {U'▒', U"▓░:"},
    {U'▓', U"▒#"},
    // Eighths: the nearest half or whole block.
    {U'▔', U"▀-"},
    {U'▁', U"▄_"},
    {U'▂', U"▄_"},
    {U'▃', U"▄_"},
    {U'▅', U"▄_"},
    {U'▆', U"█▄#"},
    {U'▇', U"█#"},
    {U'▉', U"█#"},
    {U'▊', U"▌|"},
    {U'▋', U"▌|"},
    {U'▍', U"▌|"},
    {U'▎', U"▌|"},
    {U'▏', U"▎▌|"},
    {U'▕', U"▐|"},
    // Quadrants: the half block along the edge they draw.
    {U'▖', U"▄."},
    {U'▗', U"▄."},
    {U'▘', U"▀'"},
    {U'▝', U"▀'"},
    {U'▙', U"█#"},
    {U'▛', U"█#"},
    {U'▜', U"█#"},
    {U'▟', U"█#"},
    // Braille, of `border: dotted-fine`.
    {U'⡏', U"┌+"},
    {U'⢹', U"┐+"},
    {U'⣇', U"└+"},
    {U'⣸', U"┘+"},
    {U'⠉', U"─-"},
    {U'⣀', U"─-"},
    {U'⡇', U"│|"},
    {U'⢸', U"│|"},
    // The marks of <checkbox>, <radio>, <li>, <select>, <details>,
    // <tree-item> and <slider>.
    {U'☐', U"□o"},
    {U'☑', U"■x"},
    {U'•', U"*"},
    {U'■', U"#"},
    {U'○', U"o"},
    {U'◉', U"●*"},
    {U'●', U"*"},
    {U'▴', U"▲^"},
    {U'▾', U"▼v"},
    {U'▸', U"►>"},
    {U'▶', U"►>"},
};

// The glyphs of WGL4 among Glyphs(): the repertoire Windows fonts cover, all
// but ● and □ in Lucida Console.
constexpr std::u32string_view kBasic =
    U"─│┌┐└┘├┤┬┴┼═║╒╓╔╕╖╗╘╙╚╛╜╝╠╣╦╩╬█▀▄▌▐░▒▓•■○●▲▼►□";

}  // namespace

GlyphFallback::GlyphFallback(std::u32string_view missing) {
  auto drawn = [&](char32_t glyph) {
    return missing.find(glyph) == std::u32string_view::npos;
  };
  for (const Entry& entry : kEntries) {
    if (drawn(entry.glyph)) {
      continue;
    }
    const auto replacement = std::ranges::find_if(
        entry.replacements,
        [&](char32_t glyph) { return glyph < 0x80 || drawn(glyph); });
    replacements_.emplace_back(CodePointToString(entry.glyph),
                               CodePointToString(*replacement));
  }
}

std::u32string GlyphFallback::Glyphs() {
  std::u32string glyphs;
  for (const Entry& entry : kEntries) {
    for (char32_t glyph :
         std::u32string(1, entry.glyph) + std::u32string(entry.replacements)) {
      if (glyph >= 0x80 && glyphs.find(glyph) == std::u32string::npos) {
        glyphs += glyph;
      }
    }
  }
  return glyphs;
}

std::u32string GlyphFallback::BasicMissing() {
  std::u32string missing;
  for (char32_t glyph : Glyphs()) {
    if (kBasic.find(glyph) == std::u32string_view::npos) {
      missing += glyph;
    }
  }
  return missing;
}

void GlyphFallback::Apply(Texture& texture) const {
  if (replacements_.empty()) {
    return;
  }
  for (int y = 0; y < texture.height(); ++y) {
    for (int x = 0; x < texture.width(); ++x) {
      Cell& cell = texture[x, y];
      // Every glyph replaced takes 3 bytes of UTF-8.
      if (cell.character.size() != 3) {
        continue;
      }
      for (const auto& [glyph, replacement] : replacements_) {
        if (cell.character == glyph) {
          cell.character = replacement;
          break;
        }
      }
    }
  }
}

}  // namespace rtxui
