// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

#include "rtxui/core/string.hpp"

// --- EatCodePoint ---

TEST_CASE("EatCodePoint.ASCII", "[unicode]") {
  std::string_view input = "Hello";
  size_t end = 0;
  uint32_t cp = 0;

  REQUIRE(EatCodePoint(input, 0, &end, &cp));
  CHECK(cp == 'H');
  CHECK(end == 1);

  REQUIRE(EatCodePoint(input, 1, &end, &cp));
  CHECK(cp == 'e');
  CHECK(end == 2);
}

TEST_CASE("EatCodePoint.TwoByte", "[unicode]") {
  // é is U+00E9, encoded as 0xC3 0xA9
  std::string_view input = "é";
  size_t end = 0;
  uint32_t cp = 0;

  REQUIRE(EatCodePoint(input, 0, &end, &cp));
  CHECK(cp == 0x00E9);
  CHECK(end == 2);
}

TEST_CASE("EatCodePoint.ThreeByte", "[unicode]") {
  // ₿ is U+20BF, encoded as 0xE2 0x82 0xBF
  std::string_view input = "₿";
  size_t end = 0;
  uint32_t cp = 0;

  REQUIRE(EatCodePoint(input, 0, &end, &cp));
  CHECK(cp == 0x20BF);
  CHECK(end == 3);
}

TEST_CASE("EatCodePoint.FourByte", "[unicode]") {
  // 😀 is U+1F600, encoded as 0xF0 0x9F 0x98 0x80
  std::string_view input = "😀";
  size_t end = 0;
  uint32_t cp = 0;

  REQUIRE(EatCodePoint(input, 0, &end, &cp));
  CHECK(cp == 0x1F600);
  CHECK(end == 4);
}

TEST_CASE("EatCodePoint.OutOfBounds", "[unicode]") {
  std::string_view input = "A";
  size_t end = 0;
  uint32_t cp = 0;

  CHECK_FALSE(EatCodePoint(input, 5, &end, &cp));
}

// --- CodePointToString ---

TEST_CASE("CodePointToString.ASCII", "[unicode]") {
  CHECK(CodePointToString('A') == "A");
  CHECK(CodePointToString('z') == "z");
}

TEST_CASE("CodePointToString.TwoByte", "[unicode]") {
  CHECK(CodePointToString(0x00E9) == "é");
}

TEST_CASE("CodePointToString.ThreeByte", "[unicode]") {
  CHECK(CodePointToString(0x20BF) == "₿");
}

TEST_CASE("CodePointToString.FourByte", "[unicode]") {
  CHECK(CodePointToString(0x1F600) == "😀");
}

// --- IsCombining / IsFullWidth / IsControl ---

TEST_CASE("IsControl", "[unicode]") {
  CHECK(IsControl(0));     // NUL
  CHECK(IsControl(1));     // SOH
  CHECK(IsControl(0x7F));  // DEL
  CHECK_FALSE(IsControl('A'));
  CHECK_FALSE(IsControl(0x00E9));  // é
  // Newline is NOT a control character (by design in FTXUI/RTXUI)
  CHECK_FALSE(IsControl(10));
}

TEST_CASE("IsCombining", "[unicode]") {
  // U+0301 is a combining acute accent
  CHECK(IsCombining(0x0301));
  CHECK_FALSE(IsCombining('A'));
  CHECK_FALSE(IsCombining(0x1F600));  // emoji
}

TEST_CASE("IsFullWidth", "[unicode]") {
  // CJK Unified Ideograph (U+4E00 一)
  CHECK(IsFullWidth(0x4E00));
  // Hangul Syllable (U+AC00 가)
  CHECK(IsFullWidth(0xAC00));
  // Regular ASCII
  CHECK_FALSE(IsFullWidth('A'));
  CHECK_FALSE(IsFullWidth(0x00E9));
}

// --- string_width ---

TEST_CASE("string_width.ASCII", "[unicode]") {
  CHECK(string_width("Hello") == 5);
  CHECK(string_width("") == 0);
  CHECK(string_width("A") == 1);
}

TEST_CASE("string_width.MultiByte", "[unicode]") {
  CHECK(string_width("é") == 1);  // 2 bytes, 1 cell
  CHECK(string_width("₿") == 1);  // 3 bytes, 1 cell
}

TEST_CASE("string_width.FullWidth", "[unicode]") {
  // 一 (U+4E00) is a full-width character, takes 2 cells
  CHECK(string_width("一") == 2);
  CHECK(string_width("一二") == 4);
}

TEST_CASE("string_width.CombiningCharacters", "[unicode]") {
  // e + combining acute accent (U+0301) = 1 cell
  std::string combining = "e";
  combining += CodePointToString(0x0301);
  CHECK(string_width(combining) == 1);
}

// --- Utf8ToGlyphs ---

TEST_CASE("Utf8ToGlyphs.ASCII", "[unicode]") {
  auto glyphs = Utf8ToGlyphs("abc");
  REQUIRE(glyphs.size() == 3);
  CHECK(glyphs[0] == "a");
  CHECK(glyphs[1] == "b");
  CHECK(glyphs[2] == "c");
}

TEST_CASE("Utf8ToGlyphs.MultiByte", "[unicode]") {
  auto glyphs = Utf8ToGlyphs("aéb");
  REQUIRE(glyphs.size() == 3);
  CHECK(glyphs[0] == "a");
  CHECK(glyphs[1] == "é");
  CHECK(glyphs[2] == "b");
}

TEST_CASE("Utf8ToGlyphs.FullWidth", "[unicode]") {
  // Full-width characters produce a glyph + an empty placeholder
  auto glyphs = Utf8ToGlyphs("一");
  REQUIRE(glyphs.size() == 2);
  CHECK(glyphs[0] == "一");
  CHECK(glyphs[1] == "");
}

TEST_CASE("Utf8ToGlyphs.Combining", "[unicode]") {
  // e + combining acute accent should merge into one glyph
  std::string input = "e";
  input += CodePointToString(0x0301);
  input += "x";

  auto glyphs = Utf8ToGlyphs(input);
  REQUIRE(glyphs.size() == 2);
  CHECK(glyphs[0] == input.substr(0, 3));  // e + combining
  CHECK(glyphs[1] == "x");
}

// --- GlyphCount ---

TEST_CASE("GlyphCount.ASCII", "[unicode]") {
  CHECK(GlyphCount("Hello") == 5);
  CHECK(GlyphCount("") == 0);
}

TEST_CASE("GlyphCount.MultiByte", "[unicode]") {
  CHECK(GlyphCount("aéb") == 3);
}

TEST_CASE("GlyphCount.CombiningMerges", "[unicode]") {
  std::string input = "e";
  input += CodePointToString(0x0301);  // combining acute
  CHECK(GlyphCount(input) == 1);
}

// --- GlyphNext / GlyphPrevious / GlyphIterate ---

TEST_CASE("GlyphNext.ASCII", "[unicode]") {
  std::string_view input = "abc";
  CHECK(GlyphNext(input, 0) == 1);
  CHECK(GlyphNext(input, 1) == 2);
  CHECK(GlyphNext(input, 2) == 3);
}

TEST_CASE("GlyphNext.MultiByte", "[unicode]") {
  std::string_view input = "aéb";
  CHECK(GlyphNext(input, 0) == 1);  // skip 'a'
  CHECK(GlyphNext(input, 1) == 3);  // skip 'é' (2 bytes)
  CHECK(GlyphNext(input, 3) == 4);  // skip 'b'
}

TEST_CASE("GlyphPrevious.ASCII", "[unicode]") {
  std::string_view input = "abc";
  CHECK(GlyphPrevious(input, 3) == 2);
  CHECK(GlyphPrevious(input, 2) == 1);
  CHECK(GlyphPrevious(input, 1) == 0);
  CHECK(GlyphPrevious(input, 0) == 0);
}

TEST_CASE("GlyphIterate.Forward", "[unicode]") {
  std::string_view input = "aéb";
  CHECK(GlyphIterate(input, 0) == 0);
  CHECK(GlyphIterate(input, 1) == 1);
  CHECK(GlyphIterate(input, 2) == 3);
  CHECK(GlyphIterate(input, 3) == 4);
}

// --- CellToGlyphIndex ---

TEST_CASE("CellToGlyphIndex.ASCII", "[unicode]") {
  auto indices = CellToGlyphIndex("abc");
  REQUIRE(indices.size() == 3);
  CHECK(indices[0] == 0);
  CHECK(indices[1] == 1);
  CHECK(indices[2] == 2);
}

TEST_CASE("CellToGlyphIndex.FullWidth", "[unicode]") {
  auto indices = CellToGlyphIndex("一x");
  // 一 takes 2 cells (both map to glyph 0), x is glyph 1
  REQUIRE(indices.size() == 3);
  CHECK(indices[0] == 0);
  CHECK(indices[1] == 0);
  CHECK(indices[2] == 1);
}

// --- Graphemes range ---

TEST_CASE("Graphemes.ASCII", "[unicode]") {
  std::vector<Grapheme> result;
  for (auto g : Graphemes("abc")) {
    result.push_back(g);
  }
  REQUIRE(result.size() == 3);
  CHECK(result[0].text == "a");
  CHECK(result[0].width == 1);
  CHECK(result[1].text == "b");
  CHECK(result[1].width == 1);
  CHECK(result[2].text == "c");
  CHECK(result[2].width == 1);
}

TEST_CASE("Graphemes.MultiByte", "[unicode]") {
  std::vector<Grapheme> result;
  for (auto g : Graphemes("aéb")) {
    result.push_back(g);
  }
  REQUIRE(result.size() == 3);
  CHECK(result[0].text == "a");
  CHECK(result[0].width == 1);
  CHECK(result[1].text == "é");
  CHECK(result[1].width == 1);
  CHECK(result[2].text == "b");
  CHECK(result[2].width == 1);
}

TEST_CASE("Graphemes.FullWidth", "[unicode]") {
  std::vector<Grapheme> result;
  for (auto g : Graphemes("一")) {
    result.push_back(g);
  }
  REQUIRE(result.size() == 1);
  CHECK(result[0].text == "一");
  CHECK(result[0].width == 2);
}

TEST_CASE("Graphemes.Combining", "[unicode]") {
  // e + combining acute accent should be a single grapheme
  std::string input = "e";
  input += CodePointToString(0x0301);

  std::vector<Grapheme> result;
  for (auto g : Graphemes(input)) {
    result.push_back(g);
  }
  REQUIRE(result.size() == 1);
  CHECK(result[0].text == input);
  CHECK(result[0].width == 1);
}

TEST_CASE("Graphemes.Empty", "[unicode]") {
  std::vector<Grapheme> result;
  for (auto g : Graphemes("")) {
    result.push_back(g);
  }
  CHECK(result.empty());
}

TEST_CASE("Graphemes.Mixed", "[unicode]") {
  // Mix of ASCII, multibyte, fullwidth, combining
  std::string input = "A";
  input += "é";   // 2-byte
  input += "一";  // fullwidth
  input += "e";
  input += CodePointToString(0x0301);  // combining

  std::vector<Grapheme> result;
  for (auto g : Graphemes(input)) {
    result.push_back(g);
  }
  REQUIRE(result.size() == 4);
  CHECK(result[0].text == "A");
  CHECK(result[0].width == 1);
  CHECK(result[1].text == "é");
  CHECK(result[1].width == 1);
  CHECK(result[2].text == "一");
  CHECK(result[2].width == 2);
  CHECK(result[3].width == 1);  // e + combining
}

TEST_CASE("Graphemes.Emoji", "[unicode]") {
  std::vector<Grapheme> result;
  for (auto g : Graphemes("😀")) {
    result.push_back(g);
  }
  REQUIRE(result.size() == 1);
  CHECK(result[0].text == "😀");
  CHECK(result[0].width == 2);  // emoji is fullwidth
}

TEST_CASE("Graphemes.RangeForWidth", "[unicode]") {
  // Verify that summing widths gives the same result as string_width
  std::string input = "Hello 一二 é😀";
  int total = 0;
  for (auto g : Graphemes(input)) {
    total += g.width;
  }
  CHECK(total == string_width(input));
}
