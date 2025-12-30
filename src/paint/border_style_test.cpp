#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "component/component.hpp"
#include "component/default_components.hpp"
#include "core/refcounted.hpp"
#include "core/string.hpp"
#include "dom/element.hpp"
#include "layout/layout.hpp"
#include "layout/layout_tree_builder.hpp"
#include "paint/paint.hpp"
#include "paint/texture.hpp"

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

  LayoutConstraints constraints;
  constraints.width = {width, MeasureMode::Exactly};
  constraints.height = {height, MeasureMode::Exactly};
  auto fragment = RunLayout({layout_box.get()}, constraints);

  Texture texture(static_cast<uint8_t>(width), static_cast<uint8_t>(height));
  // Initialize with spaces
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      texture.operator[](x, y).character = " ";
    }
  }
  Paint(fragment.get(), texture);
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

}  // namespace rtxui
