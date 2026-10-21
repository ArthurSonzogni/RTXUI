// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef PAINT_GLYPH_FALLBACK_HPP_
#define PAINT_GLYPH_FALLBACK_HPP_

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "rtxui/paint/texture.hpp"

namespace rtxui {

/// Replaces the glyphs RTXUI draws on its own (borders, scrollbars, the marks
/// of the built-in tags) that the terminal cannot draw. Most terminals fall
/// back to another font for a glyph theirs lacks; the Windows console does
/// not, and draws a boxed "?" instead: Consolas has none of the eighth blocks
/// of `border: tall`. Each glyph is replaced by the closest one the terminal
/// has, down to ASCII.
class GlyphFallback {
 public:
  /// Replaces nothing.
  GlyphFallback() = default;
  /// `missing`: the glyphs among Glyphs() the terminal cannot draw.
  explicit GlyphFallback(std::u32string_view missing);

  /// The glyphs to ask the terminal about: those RTXUI draws, and those that
  /// can replace them.
  static std::u32string Glyphs();
  /// The glyphs among Glyphs() outside WGL4, the repertoire Windows fonts
  /// cover: what `RTXUI_GLYPHS=basic` takes as missing.
  static std::u32string BasicMissing();

  bool empty() const { return replacements_.empty(); }
  void Apply(Texture& texture) const;

 private:
  // Glyph, replacement, both UTF-8.
  std::vector<std::pair<std::string, std::string>> replacements_;
};

}  // namespace rtxui

#endif  // PAINT_GLYPH_FALLBACK_HPP_
