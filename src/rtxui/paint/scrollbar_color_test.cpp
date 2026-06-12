#include <catch2/catch_test_macros.hpp>
#include <string>

#include "rtxui/component/default_components_internal.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/internal/component.hpp"
#include "rtxui/internal/refcounted.hpp"
#include "rtxui/layout/layout.hpp"
#include "rtxui/layout/layout_tree_builder.hpp"
#include "rtxui/paint/color.hpp"
#include "rtxui/paint/paint.hpp"
#include "rtxui/paint/texture.hpp"

namespace rtxui {
namespace {

// Renders a component into a texture of a fixed size.
Texture RenderComponent(Ref<ComponentBase> component, int width, int height) {
  component->Mount();
  auto layout_box = LayoutTreeBuilder::Build(component->Root());

  Texture texture(static_cast<uint8_t>(width), static_cast<uint8_t>(height));
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

// Regression test for: scrollbar partial-fill Unicode characters had
// incorrect foreground/background colors because foreground_color was
// blended with the already-modified background_color instead of the
// original cell background.
//
// The fix saves the original background before modifying it, and blends
// both foreground and background against that original value.
TEST_CASE("Paint: Scrollbar partial fill color blending", "[paint][scrollbar]") {
  // A scrollable container with many children to create a small scrollbar
  // thumb that will have partial-fill characters at its edges.
  // 20 lines of content in a 10-row viewport → small thumb.
  struct ScrollbarApp : Component<ScrollbarApp> {
    std::string_view view = R"html(
        <div id="scrollable">
          <div>Line 01</div>
          <div>Line 02</div>
          <div>Line 03</div>
          <div>Line 04</div>
          <div>Line 05</div>
          <div>Line 06</div>
          <div>Line 07</div>
          <div>Line 08</div>
          <div>Line 09</div>
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
            overflow-y: scroll;
          }
        </style>
      )html";

    ScrollbarApp() { Import<div>(); }
  };

  auto app = Ref<ScrollbarApp>::New();
  app->Mount();

  auto* scroll_element = app->Root()->QuerySelector("#scrollable");
  REQUIRE(scroll_element != nullptr);

  // Scroll partway so the thumb is in the middle, maximizing the chance
  // of partial-fill characters at both thumb edges.
  scroll_element->set_scroll_y(5);

  // Render to a 20x10 texture. Scrollbar column is x=19.
  auto texture = RenderComponent(app, 20, 10);

  const Color thumb_color = Color::RGBA(200, 200, 200, 200);
  const Color track_color = Color::RGBA(80, 80, 80, 120);

  // Examine the scrollbar column (x=19) for cells with partial-fill
  // Unicode block characters.
  bool found_partial = false;
  for (int y = 0; y < 10; ++y) {
    auto& cell = texture[19, y];
    const std::string& ch = cell.character;

    // Skip fully empty (space) and fully filled (█ = U+2588) cells.
    if (ch == " " || ch == "\xe2\x96\x88") {
      continue;
    }

    found_partial = true;

    // The original cell background is transparent (Color{0,0,0,0}).
    // Both foreground and background should be blended with that same
    // transparent original independently.
    //
    // Before the fix, the foreground was incorrectly blended with the
    // already-modified background, producing a double-blend artifact.
    //
    // After the fix, Blend(color, transparent) == color for both.
    // So the cell's fg and bg should each equal one of the scrollbar
    // colors (thumb or track), NOT a blend of the two.

    // The cell is at either the top or bottom edge of the thumb:
    //   Top edge:    background=track, foreground=thumb
    //   Bottom edge: background=thumb, foreground=track
    if (cell.background_color == Blend(track_color, Color{})) {
      CHECK(cell.foreground_color == Blend(thumb_color, Color{}));
    } else if (cell.background_color == Blend(thumb_color, Color{})) {
      CHECK(cell.foreground_color == Blend(track_color, Color{}));
    } else {
      FAIL("Partial-fill scrollbar cell has unexpected background: ("
           << (int)cell.background_color.r << ","
           << (int)cell.background_color.g << ","
           << (int)cell.background_color.b << ","
           << (int)cell.background_color.a << ")");
    }
  }

  // We must have found at least one partial-fill cell for this test to
  // be meaningful.
  CHECK(found_partial);
}

}  // namespace rtxui
