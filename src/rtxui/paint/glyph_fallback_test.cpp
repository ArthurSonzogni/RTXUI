#include "rtxui/paint/glyph_fallback.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "catch2/catch_message.hpp"
#include "catch2/catch_test_macros.hpp"
#include "rtxui/component.hpp"
#include "rtxui/paint/texture.hpp"
#include "rtxui/refcounted.hpp"
#include "rtxui/screen.hpp"
#include "rtxui/terminal/screen_internal.hpp"
#include "rtxui/terminal/terminal_device.hpp"

namespace rtxui {
namespace {

std::string Replaced(const GlyphFallback& fallback, std::string glyph) {
  Texture texture(1, 1);
  texture[0, 0].character = std::move(glyph);
  fallback.Apply(texture);
  return texture[0, 0].character;
}

TEST_CASE("GlyphFallback.ReplacesTheGlyphsTheTerminalLacks", "[paint]") {
  // Consolas lacks the eighth blocks of `border: tall`, not the half blocks.
  const GlyphFallback fallback(U"▊▎▔▁");
  CHECK(Replaced(fallback, "▊") == "▌");
  CHECK(Replaced(fallback, "▎") == "▌");
  CHECK(Replaced(fallback, "▔") == "▀");
  CHECK(Replaced(fallback, "▁") == "▄");
  CHECK(Replaced(fallback, "▌") == "▌");
  CHECK(Replaced(fallback, "a") == "a");
}

TEST_CASE("GlyphFallback.TakesTheFirstReplacementTheTerminalHas", "[paint]") {
  // NSimSun lacks ▀ too.
  CHECK(Replaced(GlyphFallback(U"▔▀"), "▔") == "-");
  CHECK(Replaced(GlyphFallback(U"╭"), "╭") == "┌");
  CHECK(Replaced(GlyphFallback(U"╭┌"), "╭") == "+");
}

TEST_CASE("GlyphFallback.ReplacesNothingWhenNothingIsMissing", "[paint]") {
  CHECK(GlyphFallback().empty());
  CHECK(GlyphFallback(U"").empty());
  CHECK(Replaced(GlyphFallback(), "▊") == "▊");
}

TEST_CASE("GlyphFallback.BasicKeepsWhatWindowsFontsHave", "[paint]") {
  const GlyphFallback fallback(GlyphFallback::BasicMissing());
  CHECK(Replaced(fallback, "─") == "─");
  CHECK(Replaced(fallback, "▀") == "▀");
  CHECK(Replaced(fallback, "▊") == "▌");
  CHECK(Replaced(fallback, "╭") == "┌");
  CHECK(Replaced(fallback, "⡏") == "┌");
  CHECK(Replaced(fallback, "☑") == "■");
}

class EveryGlyph : public Component<EveryGlyph> {
 public:
  std::string_view view = R"(
    <div class="page">
      <div style="border: ascii">a</div>
      <div style="border: blank">a</div>
      <div style="border: dashed">a</div>
      <div style="border: double">a</div>
      <div style="border: heavy">a</div>
      <div style="border: hkey">a</div>
      <div style="border: inner">a</div>
      <div style="border: outer">a</div>
      <div style="border: panel">a</div>
      <div style="border: round">a</div>
      <div style="border: solid">a</div>
      <div style="border: tall">a</div>
      <div style="border: thick">a</div>
      <div style="border: vkey">a</div>
      <div style="border: wide">a</div>
      <div style="border: dotted">a</div>
      <div style="border: dotted-fine">a</div>
      <div style="border: double-horizontal">a</div>
      <div style="border: double-vertical">a</div>
      <div style="border: shadow">a</div>
      <div style="border: shade-light">a</div>
      <div style="border: shade-medium">a</div>
      <div style="border: shade-dark">a</div>
      <div style="border: block">a</div>
      <div style="border: heavy-dashed">a</div>
      <div style="border: inset">a</div>
      <div style="border: outset">a</div>
      <checkbox checked="true">a</checkbox>
      <checkbox>a</checkbox>
      <radio name="r" checked="true">a</radio>
      <radio name="r">a</radio>
      <slider/>
      <select><option>a</option></select>
      <details><summary>a</summary>b</details>
      <ul><li>a</li></ul>
      <progress value="5" max="10"/>
      <hr/>
      <tree-item label="a" open="true"><tree-item label="b"/></tree-item>
      <tree-item label="c"><tree-item label="d"/></tree-item>
      <div class="scroll">1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16</div>
    </div>
    <style>
      .page { display: flex; flex-wrap: wrap; gap: 1; }
      .scroll { width: 5; height: 3; overflow: scroll; }
    </style>
  )";
};

// Every glyph RTXUI draws on its own has a replacement, down to ASCII: a
// glyph added without one would reach a terminal lacking it.
TEST_CASE("GlyphFallback.ReachesAsciiFromEveryGlyphRtxuiDraws", "[paint]") {
  auto device = std::make_shared<MockTerminalDevice>();
  device->SetMissingGlyphs(GlyphFallback::Glyphs());
  Screen screen = ScreenInternals::Make(Ref<EveryGlyph>::New(), device);

  const std::string& output = device->GetOutput();
  REQUIRE(output.find("a") != std::string::npos);
  const auto first = std::ranges::find_if(
      output, [](char c) { return static_cast<unsigned char>(c) >= 0x80; });
  INFO(std::string(first, std::min(first + 12, output.end())));
  CHECK(first == output.end());
}

TEST_CASE("GlyphFallback.ScreenAsksTheTerminalWhatItLacks", "[paint]") {
  class Tall : public Component<Tall> {
   public:
    std::string_view view = R"(<div style="border: tall">a</div>)";
  };
  auto device = std::make_shared<MockTerminalDevice>();
  device->SetMissingGlyphs(U"▊▎▔▁");
  Screen screen = ScreenInternals::Make(Ref<Tall>::New(), device);

  const std::string& output = device->GetOutput();
  CHECK(output.find("▌") != std::string::npos);
  CHECK(output.find("▀") != std::string::npos);
  CHECK(output.find("▊") == std::string::npos);
  CHECK(output.find("▔") == std::string::npos);
}

}  // namespace
}  // namespace rtxui
