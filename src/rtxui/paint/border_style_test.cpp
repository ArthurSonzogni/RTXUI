#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "rtxui/component/default_components_internal.hpp"
#include "rtxui/base/string.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/internal/component.hpp"
#include "rtxui/internal/refcounted.hpp"
#include "rtxui/layout/layout.hpp"
#include "rtxui/layout/layout_tree_builder.hpp"
#include "rtxui/paint/paint.hpp"
#include "rtxui/paint/texture.hpp"

namespace rtxui {
namespace {

// Helper to extract the character layer of a texture as a grid string.
std::string GetTextLayer(const Texture& texture) {
  std::string out;
  for (int y = 0; y < texture.height(); ++y) {
    for (int x = 0; x < texture.width(); ++x) {
      const auto& cell = const_cast<Texture&>(texture).operator[](x, y);
      out += cell.character.empty() ? " " : cell.character;
    }
    out += "\n";
  }
  return out;
}

// Helper to extract a color layer (background or foreground) as a grid of
// characters.
std::string GetColorLayer(const Texture& texture,
                          bool background,
                          const std::map<Color, char>& color_map) {
  std::string out;
  for (int y = 0; y < texture.height(); ++y) {
    for (int x = 0; x < texture.width(); ++x) {
      const auto& cell = const_cast<Texture&>(texture)[x, y];
      Color c = background ? cell.background_color : cell.foreground_color;
      if (c.a == 0) {
        out += ".";  // Use '.' for transparent/unset colors
      } else {
        auto it = color_map.find(c);
        out += (it != color_map.end()) ? it->second : '?';
      }
    }
    out += "\n";
  }
  return out;
}

// Process the raw string for comparison.
std::string CheckGrid(const std::vector<std::string>& expected) {
  std::string result;
  for (const auto& line : expected) {
    result += line + "\n";
  }
  return result;
}

// Renders a component into a texture of a fixed size.
Texture RenderComponent(Ref<ComponentBase> component, int width, int height) {
  component->Mount();
  auto layout_box = LayoutTreeBuilder::Build(component->Root());

  Texture texture(static_cast<uint8_t>(width), static_cast<uint8_t>(height));
  // Initialize with spaces
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      texture.operator[](x, y).character = " ";
    }
  }

  if (layout_box) {
    LayoutConstraints constraints;
    constraints.width = {width, MeasureMode::Exactly};
    constraints.height = {height, MeasureMode::Exactly};
    auto fragment = RunLayout({layout_box.get()}, constraints);
    Paint(fragment.get(), texture);
  }
  return texture;
}

}  // namespace

// --- TEST CASES ---

TEST_CASE("Paint: 4x3 Border Grid Component", "[paint][border]") {
  struct BorderGridApp : Component<BorderGridApp> {
    std::string_view Setup() {
      Import<div>();
      // We define a 4x3 grid using flexbox wrapping.
      // Each item is exactly 1/3 of the width and 1/4 of the height.
      return R"html(
        <style>
          self {
            display: flow;
            background-color: black;
          }
          .cell {
            display: inline;
            margin: 1;
            background-color: rgb(100, 100, 100);
          }
          .ascii  { border: ascii; }
          .blank  { border: blank; }
          .dashed { border: dashed; }
          .double { border: double; }
          .hkey   { border: hkey; }
          .heavy  { border: heavy; }
          .inner  { border: inner; }
          .outer  { border: outer; }
          .panel  { border: panel; }
          .round  { border: round; }
          .solid  { border: solid; }
          .tall   { border: tall; }
        </style>
        <div class="cell ascii">ascii</div>
        <div class="cell blank">blank</div>
        <div class="cell dashed">dashed</div>
        <div class="cell double">double</div>
        <div class="cell hkey">hkey</div>
        <div class="cell heavy">heavy</div>
        <div class="cell inner">inner</div>
        <div class="cell outer">outer</div>
        <div class="cell panel">panel</div>
        <div class="cell round">round</div>
        <div class="cell solid">solid</div>
        <div class="cell tall">tall</div>
      )html";
    }
  };

  auto texture = RenderComponent(Ref<BorderGridApp>::New(), 40, 15);
  std::cout << texture.Render() << std::endl;

  // Check the text layer for correct border characters.
  {
    std::string output = GetTextLayer(texture);
    std::vector<std::string> expected = {
        "                                        ",
        " +-----+           ┏╍╍╍╍╍╍┓  ╔══════╗   ",
        " |ascii|   blank   ╏dashed╏  ║double║   ",
        " +-----+           ┗╍╍╍╍╍╍┛  ╚══════╝   ",
        "                                        ",
        "                                        ",
        " ▔▔▔▔▔▔  ┏━━━━━┓  ▗▄▄▄▄▄▖  ▛▀▀▀▀▀▜      ",
        "  hkey   ┃heavy┃  ▐inner▌  ▌outer▐      ",
        " ▁▁▁▁▁▁  ┗━━━━━┛  ▝▀▀▀▀▀▘  ▙▄▄▄▄▄▟      ",
        "                                        ",
        "                                        ",
        " ▊█████▎  ╭─────╮  ┌─────┐  ▊▔▔▔▔▎      ",
        " ▊panel▎  │round│  │solid│  ▊tall▎      ",
        " ▊▁▁▁▁▁▎  ╰─────╯  └─────┘  ▊▁▁▁▁▎      ",
        "                                        ",
    };
    CHECK(output == CheckGrid(expected));
  }

  // Check the background color layer for correct coloring.
  {
    std::map<Color, char> color_map = {
        {Color::RGB(0, 0, 0), '0'},
        {Color::RGB(100, 100, 100), '1'},
        {Color::RGB(255, 255, 255), '2'},
    };

    std::string output;
    for (int y = 0; y < texture.height(); ++y) {
      for (int x = 0; x < texture.width(); ++x) {
        const auto& cell = texture.operator[](x, y);
        Color bg = cell.background_color;
        if (color_map.find(bg) != color_map.end()) {
          output += color_map[bg];
        } else {
          output += ' ';
        }
      }
      output += "\n";
    }

    std::vector<std::string> expected = {
        "0000000000000000000000000000000000000000",
        "0111111100111111100111111110011111111000",
        "0111111100111111100111111110011111111000",
        "0111111100111111100111111110011111111000",
        "0000000000000000000000000000000000000000",
        "0000000000000000000000000000000000000000",
        "0111111001111111000000000001111111000000",
        "0111111001111111000111110001111111000000",
        "0111111001111111000000000001111111000000",
        "0000000000000000000000000000000000000000",
        "0000000000000000000000000000000000000000",
        "0211111000111111100111111100211110000000",
        "0211111000111111100111111100211110000000",
        "0211111000111111100111111100211110000000",
        "0000000000000000000000000000000000000000",
    };

    CHECK(output == CheckGrid(expected));
  }
}

TEST_CASE("Paint: Remaining 12 Border Grid Component", "[paint][border]") {
  struct RemainingGridApp : Component<RemainingGridApp> {
    std::string_view view = R"html(
        <style>
          self {
            display: flow;
            background-color: black;
          }
          .cell {
            display: inline;
            margin: 1;
            background-color: rgb(100, 100, 100);
          }
          .thick             { border: thick; }
          .vkey              { border: vkey; }
          .wide              { border: wide; }
          .none              { border: none; }
          .dotted            { border: dotted; }
          .double-horizontal { border: double-horizontal; }
          .double-vertical   { border: double-vertical; }
          .shadow            { border: shadow; }
          .shade-light       { border: shade-light; }
          .shade-medium      { border: shade-medium; }
          .shade-dark        { border: shade-dark; }
          .squiggle          { border: squiggle; }
        </style>
        <div class="cell thick">thick</div>
        <div class="cell vkey">vkey</div>
        <div class="cell wide">wide</div>
        <div class="cell none">none</div>
        <div class="cell dotted">dotted</div>
        <div class="cell double-horizontal">doubleh</div>
        <div class="cell double-vertical">doublev</div>
        <div class="cell shadow">shadow</div>
        <div class="cell shade-light">light</div>
        <div class="cell shade-medium">medium</div>
        <div class="cell shade-dark">dark</div>
        <div class="cell squiggle">squigg</div>
      )html";

    RemainingGridApp() { Import<div>(); }
  };

  auto texture = RenderComponent(Ref<RemainingGridApp>::New(), 50, 22);

  // Check the text layer for correct border characters.
  {
    std::string output = GetTextLayer(texture);
    std::vector<std::string> expected = {
        "                                                  ",
        " █▀▀▀▀▀█  ▏    ▕  ▁▁▁▁▁▁  none  ········          ",
        " █thick█  ▏vkey▕  ▎wide▊        ·dotted·          ",
        " █▄▄▄▄▄█  ▏    ▕  ▔▔▔▔▔▔        ········          ",
        "                                                  ",
        "                                                  ",
        " ╒═══════╕  ╓───────╖  ░░░░░░░▓  ░░░░░░░          ",
        " │doubleh│  ║doublev║  ░shadow▓  ░light░          ",
        " ╘═══════╛  ╙───────╜  ░▓▓▓▓▓▓▓  ░░░░░░░          ",
        "                                                  ",
        "                                                  ",
        " ▒▒▒▒▒▒▒▒  ▓▓▓▓▓▓  ~~~~~~~~                       ",
        " ▒medium▒  ▓dark▓  ~squigg~                       ",
        " ▒▒▒▒▒▒▒▒  ▓▓▓▓▓▓  ~~~~~~~~                       ",
        "                                                  ",
        "                                                  ",
        "                                                  ",
        "                                                  ",
        "                                                  ",
        "                                                  ",
        "                                                  ",
        "                                                  ",
    };
    // Fix the U+2556 character manually if needed (it matches exactly U+2556)
    CHECK(output == CheckGrid(expected));
  }

  // Check the background color layer for correct coloring.
  {
    std::map<Color, char> color_map = {
        {Color::RGB(0, 0, 0), '0'},
        {Color::RGB(100, 100, 100), '1'},
        {Color::RGB(255, 255, 255), '2'},
    };

    std::string output;
    for (int y = 0; y < texture.height(); ++y) {
      for (int x = 0; x < texture.width(); ++x) {
        const auto& cell = texture.operator[](x, y);
        Color bg = cell.background_color;
        if (color_map.find(bg) != color_map.end()) {
          output += color_map[bg];
        } else {
          output += ' ';
        }
      }
      output += "\n";
    }

    std::vector<std::string> expected = {
        "00000000000000000000000000000000000000000000000000",
        "01111111001111110000000000111100111111110000000000",
        "01111111001111110011111200000000111111110000000000",
        "01111111001111110000000000000000111111110000000000",
        "00000000000000000000000000000000000000000000000000",
        "00000000000000000000000000000000000000000000000000",
        "01111111110011111111100111111110011111110000000000",
        "01111111110011111111100111111110011111110000000000",
        "01111111110011111111100111111110011111110000000000",
        "00000000000000000000000000000000000000000000000000",
        "00000000000000000000000000000000000000000000000000",
        "01111111100111111001111111100000000000000000000000",
        "01111111100111111001111111100000000000000000000000",
        "01111111100111111001111111100000000000000000000000",
        "00000000000000000000000000000000000000000000000000",
        "00000000000000000000000000000000000000000000000000",
        "00000000000000000000000000000000000000000000000000",
        "00000000000000000000000000000000000000000000000000",
        "00000000000000000000000000000000000000000000000000",
        "00000000000000000000000000000000000000000000000000",
        "00000000000000000000000000000000000000000000000000",
        "00000000000000000000000000000000000000000000000000",
    };

    CHECK(output == CheckGrid(expected));
  }
}

TEST_CASE("Paint: Scrollbar thumb at end", "[paint][scroll]") {
  struct ScrollDemoApp : Component<ScrollDemoApp> {
    std::string_view view = R"html(
        <div id="scrollable">
          <div>Line 1</div>
          <div>Line 2</div>
          <div>Line 3</div>
          <div>Line 4</div>
          <div>Line 5</div>
          <div>Line 6</div>
        </div>
        <style>
          self {
            display: block;
          }
          #scrollable {
            display: block;
            height: 4;
            padding-top: 1;
            padding-bottom: 1;
            overflow-y: scroll;
          }
        </style>
      )html";

    ScrollDemoApp() { Import<div>(); }
  };

  auto app = Ref<ScrollDemoApp>::New();
  app->Mount();

  auto* scroll_element = app->Root()->QuerySelector("#scrollable");
  REQUIRE(scroll_element != nullptr);

  // Content = 6 items + 2 padding = 8. Height = 4. Max scroll = 8 - 4 = 4.
  scroll_element->set_scroll_y(4);

  // Render to a texture of size 10x4
  auto texture = RenderComponent(app, 10, 4);

  // Track height is 4, scrollbar column is index 9.
  // Under corrected math, the thumb must be at the very bottom (y=3).
  // Scrollbar now uses background colors: track=RGBA(80,80,80,120),
  // thumb=RGBA(200,200,200,200).
  CHECK(texture[9, 0].character == " ");
  CHECK(texture[9, 1].character == " ");
  CHECK(texture[9, 2].character == " ");
  CHECK(texture[9, 3].character == " ");
  // The thumb (y=3) should have a brighter background than the track (y=0..2).
  CHECK(texture[9, 3].background_color != texture[9, 0].background_color);
}

TEST_CASE("Paint: Horizontal Scrollbar thumb at end", "[paint][scroll]") {
  struct ScrollDemoApp : Component<ScrollDemoApp> {
    std::string_view view = R"html(
        <div id="scrollable">
          <div class="wide-item">Wide content line!</div>
        </div>
        <style>
          self {
            display: block;
          }
          #scrollable {
            display: block;
            width: 10;
            height: 4;
            overflow-x: scroll;
          }
          .wide-item {
            display: block;
            width: 20;
          }
        </style>
      )html";

    ScrollDemoApp() { Import<div>(); }
  };

  auto app = Ref<ScrollDemoApp>::New();
  app->Mount();

  auto* scroll_element = app->Root()->QuerySelector("#scrollable");
  REQUIRE(scroll_element != nullptr);

  // Content width is 20. Container width is 10. Max scroll = 20 - 10 = 10.
  scroll_element->set_scroll_x(10);

  // Render to a texture of size 10x4.
  // The scrollbar should be on the bottom row (y=3) of the scrollable
  // container.
  auto texture = RenderComponent(app, 10, 4);

  // Scrollbar row is index 3.
  // Track width is 10, scrollbar is drawn along y=3.
  // The thumb must be at the very right (x=9) when scrolled to the end.
  CHECK(texture[0, 3].character == " ");
  CHECK(texture[9, 3].character == " ");
  // The thumb (x=9) should have a brighter background than the track (x=0).
  CHECK(texture[9, 3].background_color != texture[0, 3].background_color);
}

TEST_CASE("Individual Border Colors") {
  struct IndividualBorderColorTest : Component<IndividualBorderColorTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
          <style>
            .box {
              border: ascii;
              border-color-top: red;
              border-color-right: blue;
              border-color-bottom: green;
              border-color-left: yellow;
              color: white;
              display: block;
            }
          </style>
          <div class="box">Hi</div>
        )html";
    }
  };

  auto texture = RenderComponent(Ref<IndividualBorderColorTest>::New(), 6, 3);

  std::map<Color, char> foreground_colors = {
      {Color::RGB(255, 0, 0), 'R'},      // red
      {Color::RGB(0, 0, 255), 'B'},      // blue
      {Color::RGB(0, 255, 0), 'G'},      // green
      {Color::RGB(255, 255, 0), 'Y'},    // yellow
      {Color::RGB(255, 255, 255), 'W'},  // white
  };

  // Border colors: top=Red, right=Blue, bottom=Green, left=Yellow
  // Corners take the color of the vertical border.
  CHECK(GetColorLayer(texture, false, foreground_colors) == CheckGrid({
                                                                "YRRRRB",
                                                                "YWW..B",
                                                                "YGGGGB",
                                                            }));
}

// Regression test: `has_border` used to be the only signal paint.cpp had -
// once true, it drew a full 4-sided box with all 4 corners regardless of
// each side's actual width, so a single `border-bottom: 1;` (all other
// sides 0) rendered a two-row box with stray corner glyphs instead of a
// flat line. PhysicalFragment now carries the per-side widths so paint.cpp
// can gate each side/corner independently.
TEST_CASE("Layout: a single border side renders as a plain line, no corners",
          "[paint][border][asymmetric]") {
  struct T : Component<T> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
          <style>
            .box {
              border-bottom: 1;
              border-style: ascii;
              border-color: red;
              width: 4;
            }
          </style>
          <div class="box">Hi</div>
        )html";
    }
  };

  auto texture = RenderComponent(Ref<T>::New(), 4, 2);
  CHECK(GetTextLayer(texture) == CheckGrid({
                                     "Hi  ",
                                     "----",
                                 }));
}

// Regression test: a corner glyph should only appear when BOTH of its
// adjacent sides are present - here only top+left are set, so only the
// top-left corner should render as "+"; the top and left lines should meet
// it flush, with nothing drawn on the (absent) right/bottom sides.
TEST_CASE("Layout: a corner glyph only appears when both adjacent sides "
          "are present",
          "[paint][border][asymmetric]") {
  struct T : Component<T> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
          <style>
            .box {
              border-top: 1;
              border-left: 1;
              border-style: ascii;
              border-color: red;
              width: 4;
            }
          </style>
          <div class="box">Hi</div>
        )html";
    }
  };

  auto texture = RenderComponent(Ref<T>::New(), 4, 2);
  CHECK(GetTextLayer(texture) == CheckGrid({
                                     "+---",
                                     "|Hi ",
                                 }));
}

TEST_CASE("Paint: Tall Border Parent Background Propagation") {
  struct TallBorderParentBgTest : Component<TallBorderParentBgTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
          <style>
            self {
              display: block;
              background-color: rgb(15, 23, 42);
            }
            .container {
              display: block;
            }
            .box {
              border: tall;
              border-color: rgb(30, 41, 59);
              display: block;
            }
          </style>
          <div class="container">
            <div class="box">Hi</div>
          </div>
        )html";
    }
  };

  auto texture = RenderComponent(Ref<TallBorderParentBgTest>::New(), 6, 3);
  Color expected_bg = Color::RGB(15, 23, 42);

  // Right border uses mode Parent -> background should be expected_bg
  CHECK(texture[5, 0].background_color == expected_bg);
  CHECK(texture[5, 1].background_color == expected_bg);
  CHECK(texture[5, 2].background_color == expected_bg);

  // Left border uses mode ReverseOuter -> foreground should be expected_bg
  CHECK(texture[0, 0].foreground_color == expected_bg);
  CHECK(texture[0, 1].foreground_color == expected_bg);
  CHECK(texture[0, 2].foreground_color == expected_bg);
}

TEST_CASE("Paint: Transparent Overlay Blending on Tall Border") {
  struct OverlayTest : Component<OverlayTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
          <style>
            .root-container {
              display: block;
              background-color: rgb(0, 0, 0);
            }
            .box {
              border: tall;
              border-color: rgb(255, 0, 0);
              display: block;
            }
            .overlay {
              position: absolute;
              top: 0;
              left: 0;
              width: 6;
              height: 3;
              background-color: rgba(0, 0, 255, 0.5);
            }
          </style>
          <div class="root-container">
            <div class="box">Hi</div>
            <div class="overlay"></div>
          </div>
        )html";
    }
  };

  auto texture = RenderComponent(Ref<OverlayTest>::New(), 6, 3);
  Color expected_blended = Blend(Color::RGBA(0, 0, 255, 127), Color::RGB(0, 0, 0));

  // The left border's foreground color should be correctly blended with the semi-transparent overlay
  CHECK(texture[0, 1].foreground_color == expected_blended);
}

}  // namespace rtxui

