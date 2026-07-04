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

// Regression test: Vertical scrollbar with solid border.
// The scrollbar track should be drawn between the top and bottom border rows,
// not overlapping them. The border corners and horizontal edges must be
// preserved.
TEST_CASE("Paint: VScroll with solid border - no overlap",
          "[paint][border][scroll]") {
  struct VScrollBorderApp : Component<VScrollBorderApp> {
    std::string_view view = R"html(
        <div id="scrollable">
          <div>Line 1</div>
          <div>Line 2</div>
          <div>Line 3</div>
          <div>Line 4</div>
          <div>Line 5</div>
          <div>Line 6</div>
          <div>Line 7</div>
          <div>Line 8</div>
        </div>
        <style>
          self {
            display: block;
          }
          #scrollable {
            display: block;
            height: 6;
            border: solid;
            overflow-y: scroll;
          }
        </style>
      )html";

    VScrollBorderApp() { Import<div>(); }
  };

  auto app = Ref<VScrollBorderApp>::New();
  auto texture = RenderComponent(app, 12, 6);

  std::string text = GetTextLayer(texture);
  std::cout << "VScroll+Border:\n" << text << std::endl;

  // The border characters at the corners and edges must be present.
  // Top-left corner
  CHECK(texture[0, 0].character == "┌");
  // Top-right corner
  CHECK(texture[11, 0].character == "┐");
  // Bottom-left corner
  CHECK(texture[0, 5].character == "└");
  // Bottom-right corner
  CHECK(texture[11, 5].character == "┘");

  // Top border (horizontal)
  CHECK(texture[1, 0].character == "─");
  CHECK(texture[10, 0].character == "─");

  // Bottom border (horizontal)
  CHECK(texture[1, 5].character == "─");
  CHECK(texture[10, 5].character == "─");

  // Left border (vertical) - middle rows
  CHECK(texture[0, 1].character == "│");
  CHECK(texture[0, 4].character == "│");

  // Right border (vertical) - middle rows
  CHECK(texture[11, 1].character == "│");
  CHECK(texture[11, 4].character == "│");

  // The scrollbar should NOT overwrite any border character.
  // The scrollbar column should be at x=10 (one inside the right border).
  // The scrollbar track should only occupy y=1..4 (between the top/bottom
  // borders).
  for (int y = 1; y <= 4; ++y) {
    // The scrollbar cell at x=10 should not be a border character.
    CHECK(texture[10, y].character != "│");
    CHECK(texture[10, y].character != "─");
    CHECK(texture[10, y].character != "┌");
    CHECK(texture[10, y].character != "┐");
    CHECK(texture[10, y].character != "└");
    CHECK(texture[10, y].character != "┘");
  }
}

// Regression test: Horizontal scrollbar with solid border.
// The scrollbar track should be drawn between the left and right border
// columns, not overlapping them.
TEST_CASE("Paint: HScroll with solid border - no overlap",
          "[paint][border][scroll]") {
  struct HScrollBorderApp : Component<HScrollBorderApp> {
    std::string_view view = R"html(
        <div id="scrollable">
          <div class="wide">Wide content that exceeds container width!</div>
        </div>
        <style>
          self {
            display: block;
          }
          #scrollable {
            display: block;
            width: 12;
            height: 4;
            border: solid;
            overflow-x: scroll;
          }
          .wide {
            display: block;
            width: 30;
          }
        </style>
      )html";

    HScrollBorderApp() { Import<div>(); }
  };

  auto app = Ref<HScrollBorderApp>::New();
  auto texture = RenderComponent(app, 12, 4);

  std::string text = GetTextLayer(texture);
  std::cout << "HScroll+Border:\n" << text << std::endl;

  // Border characters must be preserved.
  CHECK(texture[0, 0].character == "┌");
  CHECK(texture[11, 0].character == "┐");
  CHECK(texture[0, 3].character == "└");
  CHECK(texture[11, 3].character == "┘");

  // Left and right borders on the scrollbar row (y=2) must be preserved.
  CHECK(texture[0, 2].character == "│");
  CHECK(texture[11, 2].character == "│");

  // The horizontal scrollbar should be on row y=2 (one inside the bottom
  // border). It should only occupy x=1..10 (between left/right borders).
  for (int x = 1; x <= 10; ++x) {
    CHECK(texture[x, 2].character != "│");
    CHECK(texture[x, 2].character != "─");
    CHECK(texture[x, 2].character != "┌");
    CHECK(texture[x, 2].character != "┐");
    CHECK(texture[x, 2].character != "└");
    CHECK(texture[x, 2].character != "┘");
  }
}

// Regression test: Both scrollbars with solid border.
// Neither scrollbar should overwrite border characters, and they should not
// overlap each other.
TEST_CASE("Paint: Both scrollbars with solid border - no overlap",
          "[paint][border][scroll]") {
  struct BothScrollBorderApp : Component<BothScrollBorderApp> {
    std::string_view view = R"html(
        <div id="scrollable">
          <div class="wide-tall">
            <div>Line 1 - very long content that needs horizontal scroll</div>
            <div>Line 2</div>
            <div>Line 3</div>
            <div>Line 4</div>
            <div>Line 5</div>
            <div>Line 6</div>
            <div>Line 7</div>
            <div>Line 8</div>
          </div>
        </div>
        <style>
          self {
            display: block;
          }
          #scrollable {
            display: block;
            width: 14;
            height: 8;
            border: solid;
            overflow-x: scroll;
            overflow-y: scroll;
          }
          .wide-tall {
            display: block;
            width: 30;
          }
        </style>
      )html";

    BothScrollBorderApp() { Import<div>(); }
  };

  auto app = Ref<BothScrollBorderApp>::New();
  auto texture = RenderComponent(app, 14, 8);

  std::string text = GetTextLayer(texture);
  std::cout << "BothScroll+Border:\n" << text << std::endl;

  // All four corners must be preserved.
  CHECK(texture[0, 0].character == "┌");
  CHECK(texture[13, 0].character == "┐");
  CHECK(texture[0, 7].character == "└");
  CHECK(texture[13, 7].character == "┘");

  // Top and bottom borders must be intact.
  for (int x = 1; x <= 12; ++x) {
    CHECK(texture[x, 0].character == "─");
    CHECK(texture[x, 7].character == "─");
  }

  // Left and right borders must be intact.
  for (int y = 1; y <= 6; ++y) {
    CHECK(texture[0, y].character == "│");
    CHECK(texture[13, y].character == "│");
  }
}

// Regression test: Vertical scrollbar with round border.
// Border characters must be preserved (round uses ╭╮╯╰│─).
TEST_CASE("Paint: VScroll with round border - no overlap",
          "[paint][border][scroll]") {
  struct VScrollRoundBorderApp : Component<VScrollRoundBorderApp> {
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
            height: 5;
            border: round;
            overflow-y: scroll;
          }
        </style>
      )html";

    VScrollRoundBorderApp() { Import<div>(); }
  };

  auto app = Ref<VScrollRoundBorderApp>::New();
  auto texture = RenderComponent(app, 10, 5);

  std::string text = GetTextLayer(texture);
  std::cout << "VScroll+RoundBorder:\n" << text << std::endl;

  // Round border corners must be preserved.
  CHECK(texture[0, 0].character == "╭");
  CHECK(texture[9, 0].character == "╮");
  CHECK(texture[0, 4].character == "╰");
  CHECK(texture[9, 4].character == "╯");

  // Right border must be intact on all middle rows.
  for (int y = 1; y <= 3; ++y) {
    CHECK(texture[9, y].character == "│");
  }
}

// Regression test: Vertical scrollbar positioned just inside border.
// Verify the scrollbar track avoids the top and bottom border rows.
TEST_CASE("Paint: VScroll track avoids border rows",
          "[paint][border][scroll]") {
  struct VScrollTrackApp : Component<VScrollTrackApp> {
    std::string_view view = R"html(
        <div id="scrollable">
          <div>Line 1</div>
          <div>Line 2</div>
          <div>Line 3</div>
          <div>Line 4</div>
          <div>Line 5</div>
          <div>Line 6</div>
          <div>Line 7</div>
          <div>Line 8</div>
          <div>Line 9</div>
          <div>Line 10</div>
        </div>
        <style>
          self {
            display: block;
          }
          #scrollable {
            display: block;
            height: 6;
            border: solid;
            overflow-y: scroll;
          }
        </style>
      )html";

    VScrollTrackApp() { Import<div>(); }
  };

  auto app = Ref<VScrollTrackApp>::New();
  auto texture = RenderComponent(app, 12, 6);

  // The scrollbar column is at x=10 (inside the right border at x=11).
  // Top border row (y=0): should be a border horizontal char "─", NOT a
  // scrollbar character.
  CHECK(texture[10, 0].character == "─");

  // Bottom border row (y=5): should be a border horizontal char "─".
  CHECK(texture[10, 5].character == "─");

  // The scrollbar track should occupy only y=1..4.
  // These cells should have scrollbar content (track or thumb background),
  // not border characters.
  for (int y = 1; y <= 4; ++y) {
    CHECK(texture[10, y].character != "─");
    CHECK(texture[10, y].character != "│");
  }
}

// Regression test: Horizontal scrollbar track avoids border columns.
TEST_CASE("Paint: HScroll track avoids border columns",
          "[paint][border][scroll]") {
  struct HScrollTrackApp : Component<HScrollTrackApp> {
    std::string_view view = R"html(
        <div id="scrollable">
          <div class="wide">Wide content exceeds width!</div>
        </div>
        <style>
          self {
            display: block;
          }
          #scrollable {
            display: block;
            width: 10;
            height: 4;
            border: solid;
            overflow-x: scroll;
          }
          .wide {
            display: block;
            width: 25;
          }
        </style>
      )html";

    HScrollTrackApp() { Import<div>(); }
  };

  auto app = Ref<HScrollTrackApp>::New();
  auto texture = RenderComponent(app, 10, 4);

  std::string text = GetTextLayer(texture);
  std::cout << "HScroll track avoids border columns:\n" << text << std::endl;

  // The scrollbar row is at y=2 (inside the bottom border at y=3).
  // Left border column (x=0): should be border vertical char "│".
  CHECK(texture[0, 2].character == "│");

  // Right border column (x=9): should be border vertical char "│".
  CHECK(texture[9, 2].character == "│");

  // The scrollbar track should only occupy x=1..8.
  for (int x = 1; x <= 8; ++x) {
    CHECK(texture[x, 2].character != "│");
    CHECK(texture[x, 2].character != "─");
  }
}

TEST_CASE("Paint: Anchor scrollbar thumb reaches bottom", "[paint][scroll]") {
  struct ScrollableApp : Component<ScrollableApp> {
    std::string_view view = R"xml(
        <div id="scrollable">
          <div>Line 1</div>
          <div>Line 2</div>
          <div>Line 3</div>
          <div>Line 4</div>
          <div>Line 5</div>
          <div>Line 6</div>
          <div>Line 7</div>
          <div>Line 8</div>
          <div>Line 9</div>
          <div>Line 10</div>
          <div>Line 11</div>
          <div>Line 12</div>
          <div>Line 13</div>
          <div>Line 14</div>
          <div>Line 15</div>
          <div>Line 16</div>
          <div>Line 17</div>
          <div>Line 18</div>
          <div>Line 19</div>
          <div>Line 20</div>
        </div>
        <style>
          self {
            display: block;
          }
          #scrollable {
            display: block;
            height: 10;
            border: tall;
            overflow-y: scroll;
          }
        </style>
      )xml";

    ScrollableApp() { Import<div>(); }
  };

  auto app = Ref<ScrollableApp>::New();
  app->Mount();

  auto* scroll_element = app->Root()->QuerySelector("#scrollable");
  REQUIRE(scroll_element != nullptr);

  // Layout first to resolve scroll height
  auto layout_box = LayoutTreeBuilder::Build(app->Root());
  LayoutConstraints constraints;
  constraints.width = {20, MeasureMode::Exactly};
  constraints.height = {10, MeasureMode::Exactly};
  auto fragment = RunLayout({layout_box.get()}, constraints);

  int scroll_height = scroll_element->scroll_height();
  int h = fragment->height;  // is 10
  int max_scroll = scroll_height - h;

  // Scroll to the bottom
  scroll_element->set_scroll_y(max_scroll);

  // Run layout again to update visual scroll
  fragment = RunLayout({layout_box.get()}, constraints);

  Texture texture(20, 10);
  Paint(fragment.get(), texture);

  // Print text layer for diagnostics
  std::cout << "Reaches bottom test:\n" << GetTextLayer(texture) << std::endl;

  // Let's examine the scrollbar track y range:
  // With border 'tall', the vertical scrollbar is at x = 18.
  // The top border is at y=0, bottom border at y=9.
  // Track start: y=1. Height: h - 2 = 8. So track y range is 1..8.
  // Since we are scrolled to the bottom, the bottom-most cell of the track
  // (y=8) MUST be part of the thumb, so it must have the thumb background. The
  // top-most cell of the track (y=1) should have the track background.
  CHECK(texture[18, 8].background_color != texture[18, 1].background_color);
}

}  // namespace rtxui
