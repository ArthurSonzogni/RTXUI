// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <string>
#include <string_view>

#include "catch2/catch_message.hpp"
#include "rtxui/component.hpp"
#include "rtxui/component/default/b/b.hpp"
#include "rtxui/component/default/div/div.hpp"
#include "rtxui/component/default/i/i.hpp"
#include "rtxui/component/default/span/span.hpp"
#include "rtxui/component/default/strong/strong.hpp"
#include "rtxui/paint/texture.hpp"
#include "rtxui/refcounted.hpp"
#include "rtxui/testing/render.hpp"

using namespace rtxui;

namespace {

class BoldTestComponent : public rtxui::Component<BoldTestComponent> {
 public:
  void InitReflection() override {
    Import<rtxui::b>();
    Import<rtxui::strong>();
    rtxui::Component<BoldTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <div>
      <b>Bold text</b>
      <strong>Strong text</strong>
    </div>
  )";
};

TEST_CASE("Bold and Strong Components cell.bold rendering",
          "[component][b][strong][paint]") {
  auto container = rtxui::Ref<BoldTestComponent>::New();
  auto texture = RenderComponent(container, 80, 24);

  bool found_bold = false;
  bool found_strong = false;
  for (int y = 0; y < texture.height(); ++y) {
    for (int x = 0; x < texture.width(); ++x) {
      const auto& cell = texture[x, y];
      if (cell.character == "B") {
        if (x + 8 < texture.width()) {
          std::string text = "";
          bool all_bold = true;
          for (int i = 0; i < 9; ++i) {
            text += texture[x + i, y].character;
            if (!texture[x + i, y].bold) {
              all_bold = false;
            }
          }
          if (text == "Bold text") {
            CHECK(all_bold);
            found_bold = true;
          }
        }
      }
      if (cell.character == "S") {
        if (x + 10 < texture.width()) {
          std::string text = "";
          bool all_bold = true;
          for (int i = 0; i < 11; ++i) {
            text += texture[x + i, y].character;
            if (!texture[x + i, y].bold) {
              all_bold = false;
            }
          }
          if (text == "Strong text") {
            CHECK(all_bold);
            found_strong = true;
          }
        }
      }
    }
  }
  CHECK(found_bold);
  CHECK(found_strong);

  std::string rendered = texture.Render();
  CHECK(rendered.find("\x1B[1m") != std::string::npos);
  CHECK(rendered.find("\x1B[22m") != std::string::npos);
}

class ItalicTestComponent : public rtxui::Component<ItalicTestComponent> {
 public:
  void InitReflection() override {
    Import<rtxui::i>();
    Import<rtxui::em>();
    rtxui::Component<ItalicTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <div>
      <i>Italic text</i>
      <em>Em text</em>
    </div>
  )";
};

TEST_CASE("Italic and Em Components cell.italic rendering",
          "[component][i][em][paint]") {
  auto container = rtxui::Ref<ItalicTestComponent>::New();
  auto texture = RenderComponent(container, 80, 24);

  auto find_italic_run = [&](const std::string& expected) {
    for (int y = 0; y < texture.height(); ++y) {
      for (int x = 0; x < texture.width(); ++x) {
        if (texture[x, y].character != expected.substr(0, 1)) {
          continue;
        }
        std::string text = "";
        bool all_italic = true;
        for (size_t c = 0; c < expected.size(); ++c) {
          text += texture[x + static_cast<int>(c), y].character;
          if (!texture[x + static_cast<int>(c), y].italic) {
            all_italic = false;
          }
        }
        if (text == expected) {
          CHECK(all_italic);
          return true;
        }
      }
    }
    return false;
  };

  CHECK(find_italic_run("Italic text"));
  CHECK(find_italic_run("Em text"));

  std::string rendered = texture.Render();
  CHECK(rendered.find("\x1B[3m") != std::string::npos);
  CHECK(rendered.find("\x1B[23m") != std::string::npos);
}

class DimTestComponent : public rtxui::Component<DimTestComponent> {
 public:
  void InitReflection() override {
    Import<rtxui::div>();
    rtxui::Component<DimTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <div>
      <div class="light">Dim text</div>
    </div>
    <style>
      .light { font-weight: lighter; }
    </style>
  )";
};

TEST_CASE("Dim text rendering via font-weight lighter",
          "[component][font-weight][paint]") {
  auto container = rtxui::Ref<DimTestComponent>::New();
  auto texture = RenderComponent(container, 80, 24);

  bool found = false;
  const std::string expected = "Dim text";
  for (int y = 0; y < texture.height() && !found; ++y) {
    for (int x = 0; x < texture.width() && !found; ++x) {
      std::string text = "";
      bool all_dim = true;
      for (size_t c = 0; c < expected.size(); ++c) {
        text += texture[x + static_cast<int>(c), y].character;
        if (!texture[x + static_cast<int>(c), y].dim) {
          all_dim = false;
        }
      }
      if (text == expected) {
        CHECK(all_dim);
        found = true;
      }
    }
  }
  CHECK(found);

  std::string rendered = texture.Render();
  CHECK(rendered.find("\x1B[2m") != std::string::npos);
}

class TextTransformTestComponent
    : public rtxui::Component<TextTransformTestComponent> {
 public:
  void InitReflection() override {
    Import<rtxui::div>();
    Import<rtxui::span>();
    rtxui::Component<TextTransformTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <div>
      <div class="up">hello wörld</div>
      <div class="down">QUIET Text</div>
      <div class="cap">two words</div>
      <div class="up"><span>inherited text</span></div>
    </div>
    <style>
      .up { text-transform: uppercase; }
      .down { text-transform: lowercase; }
      .cap { text-transform: capitalize; }
    </style>
  )";
};

TEST_CASE("Text transform rendering", "[component][text-transform][paint]") {
  auto container = rtxui::Ref<TextTransformTestComponent>::New();
  auto texture = RenderComponent(container, 80, 24);

  std::string all_text;
  for (int y = 0; y < texture.height(); ++y) {
    for (int x = 0; x < texture.width(); ++x) {
      all_text += texture[x, y].character;
    }
    all_text += "\n";
  }

  // Uppercase transforms ASCII letters; multi-byte UTF-8 passes through.
  CHECK(all_text.find("HELLO WöRLD") != std::string::npos);
  CHECK(all_text.find("quiet text") != std::string::npos);
  CHECK(all_text.find("Two Words") != std::string::npos);
  // The transform inherits into nested inline elements.
  CHECK(all_text.find("INHERITED TEXT") != std::string::npos);
}

class LetterSpacingTestComponent
    : public rtxui::Component<LetterSpacingTestComponent> {
 public:
  void InitReflection() override {
    Import<rtxui::div>();
    Import<rtxui::span>();
    rtxui::Component<LetterSpacingTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <div>
      <div class="spaced">ab cd</div>
      <div class="wide">xy</div>
      <div class="spaced"><span>ef</span></div>
    </div>
    <style>
      .spaced { letter-spacing: 1; }
      .wide { letter-spacing: 2; }
    </style>
  )";
};

TEST_CASE("Letter spacing rendering", "[component][letter-spacing][paint]") {
  auto container = rtxui::Ref<LetterSpacingTestComponent>::New();
  auto texture = RenderComponent(container, 80, 24);

  std::string all_text;
  for (int y = 0; y < texture.height(); ++y) {
    for (int x = 0; x < texture.width(); ++x) {
      all_text += texture[x, y].character;
    }
    all_text += "\n";
  }

  // One non-breaking space (U+00A0) between every pair of grapheme
  // clusters, including around the (still breakable) ASCII word gap.
  CHECK(all_text.find("a\xC2\xA0"
                      "b\xC2\xA0 \xC2\xA0"
                      "c\xC2\xA0"
                      "d") != std::string::npos);
  // letter-spacing: 2 inserts two cells.
  CHECK(all_text.find("x\xC2\xA0\xC2\xA0y") != std::string::npos);
  // The spacing inherits into nested inline elements.
  CHECK(all_text.find("e\xC2\xA0"
                      "f") != std::string::npos);
}

class LineHeightTestComponent
    : public rtxui::Component<LineHeightTestComponent> {
 public:
  void InitReflection() override {
    Import<rtxui::div>();
    rtxui::Component<LineHeightTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <div class="lh">first
second</div>
    <style>
      .lh { line-height: 2; white-space: pre; }
    </style>
  )";
};

class EllipsisTestComponent : public rtxui::Component<EllipsisTestComponent> {
 public:
  void InitReflection() override {
    Import<rtxui::div>();
    rtxui::Component<EllipsisTestComponent>::InitReflection();
  }
  // Plain blocks, which is what a <div> is and what the property did nothing
  // on: the ellipsis check reads the style of the box running the inline flow,
  // and a block's text sits in an anonymous wrapper that was not given it.
  // Wrapped in a block parent so each block gets a row of its own to assert
  // on. A component's own root is inline-level, and although a block child
  // now breaks the line there too, reading rows is clearer with a real block
  // container.
  std::string_view view = R"(
    <div id="wrap">
      <div class="e" id="over">abcdefghij</div>
      <div class="e" id="exact">abcdef</div>
      <div class="e" id="under">abc</div>
      <div class="e" id="wide">你好你好</div>
      <div class="e narrow" id="tiny">abcdef</div>
      <div class="c" id="clipped">abcdefghij</div>
    </div>
    <style>
      #wrap { display: block; }
      .e { width: 6; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
      .narrow { width: 2; }
      .c { width: 6; white-space: nowrap; overflow: hidden; text-overflow: clip; }
    </style>
  )";
};

TEST_CASE("text-overflow: ellipsis truncates a block", "[component][paint]") {
  auto container = rtxui::Ref<EllipsisTestComponent>::New();
  auto texture = RenderComponent(container, 30, 12);

  SECTION("text too wide is truncated and marked") {
    const int row = FindRow(texture, "abc.");
    REQUIRE(row != -1);
    CHECK(RowText(texture, row) == "abc...");
  }

  SECTION("text that fits is left alone") {
    CHECK(FindRow(texture, "abcdef") != -1);  // the exact-fit block, unmarked
    CHECK(FindRow(texture, "abc") != -1);     // and the short one
  }

  SECTION("truncation lands on a character boundary") {
    // Three double-width glyphs do not fit in six cells alongside the marker,
    // and half a glyph is not an option -- so one whole glyph and the marker.
    const int row = FindRow(texture, "\u4f60.");
    REQUIRE(row != -1);
    CHECK(RowText(texture, row) == "\u4f60...");
  }

  SECTION("a container too narrow for the marker shows what fits") {
    // Two cells cannot hold "..." let alone any text, so the marker itself is
    // what gets truncated. Matched on the whole row, since "abc..." contains
    // ".." too.
    bool found = false;
    for (int y = 0; y < texture.height(); ++y) {
      found = found || RowText(texture, y) == "..";
    }
    CHECK(found);
  }

  SECTION("clip is unaffected") {
    CHECK(FindRow(texture, "abcdefghij") == -1);  // clipped to the container
    const int row = FindRow(texture, "abcdef");
    REQUIRE(row != -1);
  }
}

class TabStopTestComponent : public rtxui::Component<TabStopTestComponent> {
 public:
  void InitReflection() override {
    Import<rtxui::div>();
    rtxui::Component<TabStopTestComponent>::InitReflection();
  }
  // Each line starts at a different column so the tab lands on a different
  // stop, which is what distinguishes real tab stops from a fixed run of
  // spaces. The wide-glyph line checks the column is counted in cells.
  std::string_view view =
      "<div class=\"t\">a\tb\nxy\tz\nabcdefgh\ti\n\u4f60\tw</div>"
      "<div class=\"t four\">a\tb</div>"
      "<div class=\"n\">a\tb</div>"
      "<style>"
      ".t { white-space: pre; }"
      ".four { tab-size: 4; }"
      ".n { white-space: normal; }"
      "</style>";
};

TEST_CASE("Tabs expand to tab stops", "[component][tab-size][paint]") {
  auto container = rtxui::Ref<TabStopTestComponent>::New();
  auto texture = RenderComponent(container, 40, 24);

  SECTION("a tab advances to the next multiple of tab-size") {
    // Not a fixed number of spaces: 'a' sits at column 0 so its tab fills 7,
    // while "abcdefgh" already ends on a stop so its tab fills a whole 8.
    int row = FindRow(texture, "a");
    REQUIRE(row != -1);
    CHECK(RowText(texture, row) == "a       b");
    CHECK(RowText(texture, row + 1) == "xy      z");
    CHECK(RowText(texture, row + 2) == "abcdefgh        i");
  }

  SECTION("the column is counted in cells, not codepoints") {
    // A full-width glyph occupies two cells, so the next stop is 6 away.
    int row = FindRow(texture, "\u4f60");
    REQUIRE(row != -1);
    CHECK(RowText(texture, row) == "\u4f60      w");
  }

  SECTION("tab-size moves the stops") {
    int row = FindRow(texture, "a   b");
    REQUIRE(row != -1);
    CHECK(RowText(texture, row) == "a   b");
  }

  SECTION("a tab is collapsible whitespace outside pre") {
    int row = FindRow(texture, "a b");
    REQUIRE(row != -1);
    CHECK(RowText(texture, row) == "a b");
  }

  SECTION("no literal tab survives into the painted cells") {
    // The engine measures a tab as one cell while a terminal advances to its
    // own stop, so one reaching the screen would shift the rest of the line
    // and leave the cell diff describing a screen that does not exist.
    for (int y = 0; y < texture.height(); ++y) {
      CHECK(RowText(texture, y).find('\t') == std::string::npos);
    }
  }
}

TEST_CASE("Line height rendering", "[component][line-height][paint]") {
  auto container = rtxui::Ref<LineHeightTestComponent>::New();
  auto texture = RenderComponent(container, 80, 24);

  int first_row = -1;
  int second_row = -1;
  for (int y = 0; y < texture.height(); ++y) {
    if (RowText(texture, y).find("first") != std::string::npos) {
      first_row = y;
    }
    if (RowText(texture, y).find("second") != std::string::npos) {
      second_row = y;
    }
  }
  REQUIRE(first_row != -1);
  REQUIRE(second_row != -1);
  // line-height: 2 makes each line box two rows tall: one blank row
  // separates consecutive lines.
  CHECK(second_row - first_row == 2);
}

class OverflowWrapTestComponent
    : public rtxui::Component<OverflowWrapTestComponent> {
 public:
  void InitReflection() override {
    Import<rtxui::div>();
    rtxui::Component<OverflowWrapTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <div>
      <div class="break">abcdefghijklmno</div>
      <div class="keep">abcdefghijklmno</div>
    </div>
    <style>
      .break, .keep { width: 6; }
      .keep { overflow-wrap: normal; }
    </style>
  )";
};

TEST_CASE("Overflow wrap rendering", "[component][overflow-wrap][paint]") {
  auto container = rtxui::Ref<OverflowWrapTestComponent>::New();
  auto texture = RenderComponent(container, 80, 24);

  // Default: the word emergency-breaks at the container edge, and each
  // line stays within the specified width (regression: the break used to
  // land one column too late).
  CHECK(RowText(texture, 0) == "abcdef");
  CHECK(RowText(texture, 1) == "ghijkl");
  CHECK(RowText(texture, 2) == "mno");
  // overflow-wrap: normal keeps the word intact and lets it overflow.
  CHECK(RowText(texture, 3) == "abcdefghijklmno");
}

class WordBreakTestComponent : public rtxui::Component<WordBreakTestComponent> {
 public:
  void InitReflection() override {
    Import<rtxui::div>();
    Import<rtxui::b>();
    rtxui::Component<WordBreakTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <div>
      <div class="normal">Ab <b>abcdefgh</b></div>
      <div class="break">Ab <b>abcdefgh</b></div>
    </div>
    <style>
      .normal, .break { width: 6; }
      .break { word-break: break-all; }
    </style>
  )";
};

TEST_CASE("word-break: break-all wraps mid-word",
          "[component][word-break][paint]") {
  auto container = rtxui::Ref<WordBreakTestComponent>::New();
  auto texture = RenderComponent(container, 80, 24);

  // Default: "abcdefgh" (a separate inline <b> child) doesn't fit after
  // "Ab " on line 0, so it's pushed whole to line 1, then emergency-broken
  // there like any overflowing unbreakable word.
  CHECK(RowText(texture, 0) == "Ab");
  CHECK(RowText(texture, 1) == "abcdef");
  CHECK(RowText(texture, 2) == "gh");
  // word-break: break-all fills line 0 to the edge (breaking right after
  // "Ab abc") instead of pushing "abcdefgh" whole to the next line.
  CHECK(RowText(texture, 3) == "Ab abc");
  CHECK(RowText(texture, 4) == "defgh");
}

class StyleDecorationTestComponent
    : public rtxui::Component<StyleDecorationTestComponent> {
 public:
  void InitReflection() override {
    rtxui::Component<StyleDecorationTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <box>
      <item id="u">u</item>
      <item id="ud">ud</item>
      <item id="s">s</item>
      <item id="bl">bl</item>
    </box>
    <style>
      box { display: block; }
      item { display: inline; }
      #u { text-decoration: underline; }
      #ud { text-decoration: double-underline; }
      #s { text-decoration: strikethrough; }
      #bl { text-decoration: blink; }
    </style>
  )";
};

TEST_CASE("Text Decoration rendering onto cells",
          "[component][style][paint][decoration]") {
  auto container = rtxui::Ref<StyleDecorationTestComponent>::New();
  auto texture = RenderComponent(container, 80, 24);

  bool found_u = false;
  bool found_ud = false;
  bool found_s = false;
  bool found_bl = false;

  for (int y = 0; y < texture.height(); ++y) {
    for (int x = 0; x < texture.width(); ++x) {
      const auto& cell = texture[x, y];
      if (cell.character == "u" && x + 1 < texture.width() &&
          texture[x + 1, y].character != "d") {
        CHECK(cell.underlined);
        CHECK_FALSE(cell.underlined_double);
        found_u = true;
      }
      if (cell.character == "u" && x + 1 < texture.width() &&
          texture[x + 1, y].character == "d") {
        CHECK_FALSE(cell.underlined);
        CHECK(cell.underlined_double);
        CHECK(texture[x + 1, y].underlined_double);
        found_ud = true;
      }
      if (cell.character == "s") {
        CHECK(cell.strikethrough);
        found_s = true;
      }
      if (cell.character == "b" && x + 1 < texture.width() &&
          texture[x + 1, y].character == "l") {
        CHECK(cell.blink);
        CHECK(texture[x + 1, y].blink);
        found_bl = true;
      }
    }
  }
  CHECK(found_u);
  CHECK(found_ud);
  CHECK(found_s);
  CHECK(found_bl);
}

class OverlineTestComponent : public rtxui::Component<OverlineTestComponent> {
 public:
  void InitReflection() override {
    Import<rtxui::div>();
    rtxui::Component<OverlineTestComponent>::InitReflection();
  }
  std::string_view view = R"(
    <div class="over">ab<div class="plain">cd</div></div>
    <style>
      .over { display: block; text-decoration: overline; }
      .plain { display: inline; }
    </style>
  )";
};

}  // namespace

TEST_CASE("text-decoration: overline reaches the cells and inherits",
          "[component][style][paint][decoration]") {
  auto container = rtxui::Ref<OverlineTestComponent>::New();
  container->Mount();
  auto texture = PaintComponent(*container, 20, 4);

  int found = 0;
  for (int y = 0; y < texture.height(); ++y) {
    for (int x = 0; x < texture.width(); ++x) {
      const auto& cell = texture[x, y];
      // "ab" is styled directly; "cd" only inherits it from the parent.
      if (cell.character == "a" || cell.character == "b" ||
          cell.character == "c" || cell.character == "d") {
        INFO("cell " << x << "," << y << " = " << cell.character);
        CHECK(cell.overlined);
        ++found;
      }
    }
  }
  CHECK(found == 4);
}
