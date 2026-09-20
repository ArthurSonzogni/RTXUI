// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <utility>
#include <vector>

#include "rtxui/base/string.hpp"

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

// --- StripIndent ---

TEST_CASE("StripIndent is idempotent", "[unicode][string]") {
  // Mount() relies on this: Template() strips the view once, and stripping the
  // result again used to be done anyway -- two splits into lines and a rebuilt
  // string, per component instance. Removing that second call is only correct
  // because a second pass cannot change anything.
  const auto twice = [](std::string_view text) {
    return StripIndent(StripIndent(text));
  };

  const std::string_view indented =
      "\n    <div>\n      <span>x</span>\n    </div>\n  ";
  CHECK(StripIndent(indented) == twice(indented));

  // The shapes that could make a second pass behave differently: an unindented
  // first line, blank lines among indented ones, tabs, and nothing at all.
  const std::string_view ragged = "top\n    indented\n\n        more\n";
  CHECK(StripIndent(ragged) == twice(ragged));
  const std::string_view blanks = "    a\n\n    \n    b\n";
  CHECK(StripIndent(blanks) == twice(blanks));
  CHECK(StripIndent("") == twice(""));
  CHECK(StripIndent("one line") == twice("one line"));
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

TEST_CASE("string_width.ZeroWidthFormatCharacters", "[unicode]") {
  // A terminal draws none of these, so counting one as a cell puts everything
  // after it a column to the left of where the layout believes it is, and the
  // cell diff then describes a screen that is one column out per occurrence.
  // They turn up in ordinary text: a zero-width space pasted from a web page,
  // a bidi mark inside a filename, a byte-order mark at the head of a file.
  const auto zero_width = [](uint32_t codepoint) {
    return string_width("a" + CodePointToString(codepoint) + "b");
  };
  CHECK(zero_width(0x200B) == 2);  // zero-width space
  // U+200D, the zero-width joiner, is deliberately absent: it joins the
  // characters either side into a single cluster, so it changes the count for
  // reasons of its own rather than by occupying a cell.
  CHECK(zero_width(0x200E) == 2);  // left-to-right mark
  CHECK(zero_width(0x202E) == 2);  // right-to-left override
  CHECK(zero_width(0x2060) == 2);  // word joiner
  CHECK(zero_width(0x2069) == 2);  // pop directional isolate
  CHECK(zero_width(0xFEFF) == 2);  // byte-order mark
  CHECK(zero_width(0xFFFB) == 2);  // interlinear annotation terminator

  // The neighbours of those ranges are ordinary printable characters and must
  // keep their cell -- the table is searched by bisection, so an interval that
  // is too wide or out of order fails quietly.
  CHECK(zero_width(0x200A) == 3);  // hair space
  CHECK(zero_width(0x2010) == 3);  // hyphen
  CHECK(zero_width(0x2065) == 3);  // unassigned, but not in the format set
  CHECK(zero_width(0xFEFE) == 3);
  CHECK(zero_width(0xFFFC) == 3);  // object replacement character
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

TEST_CASE("Base64Encode", "[base64]") {
  // Standard test vectors (RFC 4648).
  CHECK(Base64Encode("") == "");
  CHECK(Base64Encode("f") == "Zg==");
  CHECK(Base64Encode("fo") == "Zm8=");
  CHECK(Base64Encode("foo") == "Zm9v");
  CHECK(Base64Encode("foob") == "Zm9vYg==");
  CHECK(Base64Encode("fooba") == "Zm9vYmE=");
  CHECK(Base64Encode("foobar") == "Zm9vYmFy");
  CHECK(Base64Encode("hello world") == "aGVsbG8gd29ybGQ=");

  // Bytes with the high bit set must round-trip correctly (not sign-extend).
  std::string binary("\xFF\x00\x80", 3);
  CHECK(Base64Encode(binary) == "/wCA");
}

// --- Emoji sequences ---
//
// The width of a cluster is what layout reserves and what the cursor advances
// by, so a cluster measured differently from how the terminal draws it shifts
// everything after it on the line.

namespace {
// Sums the widths the renderer will actually use, and counts the clusters
// that text wrapping and truncation are allowed to break between.
std::pair<int, int> ClustersAndWidth(std::string_view text) {
  int clusters = 0;
  int width = 0;
  for (const Grapheme& grapheme : Graphemes(text)) {
    ++clusters;
    width += grapheme.width;
  }
  return {clusters, width};
}
}  // namespace

TEST_CASE("Grapheme.VariationSelector", "[unicode][emoji]") {
  // U+2600 is one cell as text. U+FE0F asks for the emoji presentation, which
  // terminals draw in two -- the selector clusters either way, but it used to
  // leave the width at one.
  CHECK(ClustersAndWidth("\xe2\x98\x80") == std::pair{1, 1});              // ☀
  CHECK(ClustersAndWidth("\xe2\x98\x80\xef\xb8\x8f") == std::pair{1, 2});  // ☀️

  // VS15 asks for text presentation, taking a default-emoji character back
  // down to one cell.
  CHECK(ClustersAndWidth("\xe2\x9d\xa4\xef\xb8\x8e") == std::pair{1, 1});  // ❤︎
}

TEST_CASE("Grapheme.RegionalIndicatorFlags", "[unicode][emoji]") {
  const std::string fr = "\xf0\x9f\x87\xab\xf0\x9f\x87\xb7";  // 🇫🇷
  const std::string de = "\xf0\x9f\x87\xa9\xf0\x9f\x87\xaa";  // 🇩🇪

  // One cluster, two cells. Two clusters would let wrapping split the flag in
  // half and truncation cut it to a lone letter.
  CHECK(ClustersAndWidth(fr) == std::pair{1, 2});

  // Adjacent flags stay separate rather than merging into one run.
  CHECK(ClustersAndWidth(fr + de) == std::pair{2, 4});

  // An odd one out is its own cluster: three indicators are a flag plus the
  // start of another, not a flag and a half.
  CHECK(ClustersAndWidth(fr + "\xf0\x9f\x87\xa9") == std::pair{2, 3});

  // A lone indicator is just a letter.
  CHECK(ClustersAndWidth("\xf0\x9f\x87\xab") == std::pair{1, 1});
}

TEST_CASE("Grapheme.ZeroWidthJoinerAndModifiers", "[unicode][emoji]") {
  // A ZWJ sequence is one cluster occupying the cells of one emoji.
  CHECK(ClustersAndWidth(
            "\xf0\x9f\x91\xa8\xe2\x80\x8d\xf0\x9f\x91\xa9\xe2\x80\x8d"
            "\xf0\x9f\x91\xa7") == std::pair{1, 2});  // 👨‍👩‍👧

  // A skin-tone modifier joins the emoji it follows.
  CHECK(ClustersAndWidth("\xf0\x9f\x91\x8d\xf0\x9f\x8f\xbd") ==
        std::pair{1, 2});  // 👍🏽
}

TEST_CASE("string_width agrees with the grapheme widths", "[unicode][emoji]") {
  // string_width used to count codepoints, so it reported eight cells for a
  // family emoji that occupies two. Nothing in the library called it, which
  // is the only reason that never showed up on screen.
  for (std::string_view text : {
           "Hello",
           "\xe6\xbc\xa2\xe5\xad\x97",          // 漢字
           "\xe2\x98\x80\xef\xb8\x8f",          // ☀️
           "\xf0\x9f\x87\xab\xf0\x9f\x87\xb7",  // 🇫🇷
           "\xf0\x9f\x91\xa8\xe2\x80\x8d\xf0\x9f\x91\xa9\xe2\x80\x8d\xf0\x9f"
           "\x91\xa7",
       }) {
    INFO("text: " << text);
    CHECK(string_width(text) == ClustersAndWidth(text).second);
  }
}
