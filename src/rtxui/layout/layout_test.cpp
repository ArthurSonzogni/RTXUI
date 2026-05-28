#include "rtxui/layout/layout.hpp"

#include <catch2/catch_test_macros.hpp>
#include <map>
#include <string>
#include <vector>

#include "rtxui/component/component.hpp"
#include "rtxui/component/default_components.hpp"
#include "rtxui/core/refcounted.hpp"
#include "rtxui/core/string.hpp"
#include "rtxui/dom/element.hpp"
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
      const auto& cell = const_cast<Texture&>(texture)[x, y];
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
  // Mount performs the initial Setup() and DOM tree construction.
  component->Mount();

  // Convert DOM to Layout Tree.
  auto layout_box = LayoutTreeBuilder::Build(component->Root());

  // Execute Layout algorithm.
  LayoutConstraints constraints;
  constraints.width = {width, MeasureMode::Exactly};
  constraints.height = {height, MeasureMode::Exactly};
  auto fragment = RunLayout({layout_box.get()}, constraints);

  // Paint the resulting fragments into a texture.
  Texture texture(static_cast<uint8_t>(width), static_cast<uint8_t>(height));
  Paint(fragment.get(), texture);
  return texture;
}

}  // namespace

// --- TEST CASES ---

TEST_CASE("Layout: Block-level vertical stacking", "[layout]") {
  struct BlockStackTest : Component<BlockStackTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
        <style>
          div { display: block; }
          .red { background-color: rgb(255, 0, 0); }
          .blue { background-color: rgb(0, 0, 255); }
        </style>
        <div class="red">Item 1</div>
        <div class="blue">Item 2</div>
      )html";
    }
  };

  auto texture = RenderComponent(Ref<BlockStackTest>::New(), 10, 2);

  std::map<Color, char> colors = {
      {Color::RGB(255, 0, 0), 'R'},
      {Color::RGB(0, 0, 255), 'B'},
  };

  CHECK(GetTextLayer(texture) == CheckGrid({
                                     "Item 1    ",
                                     "Item 2    ",
                                 }));

  CHECK(GetColorLayer(texture, true, colors) == CheckGrid({
                                                    "RRRRRRRRRR",
                                                    "BBBBBBBBBB",
                                                }));
}

TEST_CASE("Layout: Inline-level wrapping", "[layout]") {
  struct InlineWrapTest : Component<InlineWrapTest> {
    std::string_view Setup() {
      Import<span>();
      return R"html(<span>WordA </span><span>WordB</span>)html";
    }
  };

  auto texture = RenderComponent(Ref<InlineWrapTest>::New(), 6, 2);

  CHECK(GetTextLayer(texture) == CheckGrid({
                                     "WordA ",
                                     "WordB ",
                                 }));
}

TEST_CASE("Layout: Padding and Box Model", "[layout]") {
  struct BoxModelTest : Component<BoxModelTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
        <style>
          .container {
            background-color: red;
            padding: 1;
            display: block;
          }
          .content {
            background-color: green;
            display: block;
          }
        </style>
        <div class="container">
          <div class="content">HI</div>
        </div>
      )html";
    }
  };

  auto texture = RenderComponent(Ref<BoxModelTest>::New(), 6, 3);

  std::map<Color, char> colors = {
      {Color::RGB(255, 0, 0), 'R'},
      {Color::RGB(0, 255, 0), 'G'},
  };

  CHECK(GetColorLayer(texture, true, colors) == CheckGrid({
                                                    "RRRRRR",
                                                    "RGGGGR",
                                                    "RRRRRR",
                                                }));

  CHECK(GetTextLayer(texture) == CheckGrid({
                                     "      ",
                                     " HI   ",
                                     "      ",
                                 }));
}

TEST_CASE("Layout: Flexbox Row with Grow", "[layout]") {
  struct FlexGrowTest : Component<FlexGrowTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
        <style>
          .row { display: flex; flex-direction: row; }
          .item { flex-grow: 1; height: 1; }
          .r { background-color: rgb(255, 0, 0); }
          .g { background-color: rgb(0, 255, 0); }
          .b { background-color: rgb(0, 0, 255); }
        </style>
        <div class="row">
          <div class="item r">1</div>
          <div class="item g">2</div>
          <div class="item b">3</div>
        </div>
      )html";
    }
  };

  auto texture = RenderComponent(Ref<FlexGrowTest>::New(), 12, 1);

  std::map<Color, char> colors = {
      {Color::RGB(255, 0, 0), 'R'},
      {Color::RGB(0, 255, 0), 'G'},
      {Color::RGB(0, 0, 255), 'B'},
  };

  CHECK(GetColorLayer(texture, true, colors) == CheckGrid({
                                                    "RRRRGGGGBBBB",
                                                }));

  CHECK(GetTextLayer(texture) == CheckGrid({
                                     "1   2   3   ",
                                 }));
}

TEST_CASE("Layout: Borders", "[layout]") {
  SECTION("Simple ASCII Border") {
    struct BorderTest : Component<BorderTest> {
      std::string_view Setup() {
        Import<div>();
        return R"html(
          <style>
            .box {
              border: ascii;
              display: block;
            }
          </style>
          <div class="box">Hi</div>
        )html";
      }
    };

    auto texture = RenderComponent(Ref<BorderTest>::New(), 6, 3);

    CHECK(GetTextLayer(texture) == CheckGrid({
                                       "+----+",
                                       "|Hi  |",
                                       "+----+",
                                   }));
  }

  SECTION("Border with Colors") {
    struct ColoredBorderTest : Component<ColoredBorderTest> {
      std::string_view Setup() {
        Import<div>();
        return R"html(
          <style>
            .box {
              border: ascii;
              border-color: red;
              color: blue;
              display: block;
            }
          </style>
          <div class="box">Hi</div>
        )html";
      }
    };

    auto texture = RenderComponent(Ref<ColoredBorderTest>::New(), 6, 3);

    std::map<Color, char> foreground_colors = {
        {Color::RGB(255, 0, 0), 'R'},
        {Color::RGB(0, 0, 255), 'B'},
    };

    // Border should be Red, Text should be Blue
    CHECK(GetColorLayer(texture, false, foreground_colors) == CheckGrid({
                                                                  "RRRRRR",
                                                                  "RBB..R",
                                                                  "RRRRRR",
                                                              }));
  }

  SECTION("Rounded Border") {
    struct RoundedBorderTest : Component<RoundedBorderTest> {
      std::string_view Setup() {
        Import<div>();
        return R"html(
          <style>
            .box {
              border: rounded;
              display: block;
            }
          </style>
          <div class="box">X</div>
        )html";
      }
    };

    auto texture = RenderComponent(Ref<RoundedBorderTest>::New(), 3, 3);

    // Rounded border uses Unicode characters
    CHECK(GetTextLayer(texture) == CheckGrid({
                                       "╭─╮",
                                       "│X│",
                                       "╰─╯",
                                   }));
  }

}

}  // namespace rtxui
