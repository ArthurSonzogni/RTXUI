#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <string>
#include <vector>

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

namespace {

// A minimal textarea host, mirroring the default component's own
// height/width so the scrollbar column position (x = width - 1) is known.
struct TextareaScrollbarApp : Component<TextareaScrollbarApp> {
  std::string text;
  void InitReflection() override {
    Bind(text);
    Import<textarea>();
    Component<TextareaScrollbarApp>::InitReflection();
  }
  std::string_view view = R"html(<textarea value="{text}" />)html";
};

// Unlike the top-level RenderComponent() helper (Mount() only), textarea's
// rendered spans (left_unselected, right_unselected, ...) are only
// populated by Digest()/DigestShared() -- Mount() alone leaves them at
// their default-constructed empty strings, which would make the textarea
// appear to have no content at all regardless of `text`.
Texture RenderTextarea(Ref<TextareaScrollbarApp> app, int width, int height) {
  app->Mount();
  app->Digest();
  auto layout_box = LayoutTreeBuilder::Build(app->Root());

  Texture texture(static_cast<uint8_t>(width), static_cast<uint8_t>(height));
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      texture[x, y].character = " ";
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

std::vector<Color> ScrollbarColumn(Texture& texture, int x, int height) {
  std::vector<Color> colors;
  for (int y = 0; y < height; ++y) {
    colors.push_back(texture[x, y].background_color);
  }
  return colors;
}

int CountDistinct(const std::vector<Color>& colors) {
  std::vector<Color> distinct;
  for (const auto& c : colors) {
    if (std::find(distinct.begin(), distinct.end(), c) == distinct.end()) {
      distinct.push_back(c);
    }
  }
  return static_cast<int>(distinct.size());
}

}  // namespace

// Regression test for: textarea used to hardcode scrollbar-width: none,
// hiding the scrollbar entirely regardless of overflow. Default height is
// 5 rows (see textarea.cpp), default width 40 (scrollbar column x=39).
TEST_CASE("Textarea shows a full-height scrollbar thumb when content fits",
          "[paint][scrollbar][textarea]") {
  auto app = Ref<TextareaScrollbarApp>::New();
  app->text = "line one\nline two";  // 2 lines, well within height 5.
  auto texture = RenderTextarea(app, 40, 5);

  // A thumb spanning the whole track looks uniform: every row in the
  // scrollbar column shares the same (thumb) color, with no track color
  // visible anywhere since there's nothing to scroll.
  auto column = ScrollbarColumn(texture, 39, 5);
  CHECK(CountDistinct(column) == 1);
}

TEST_CASE("Textarea shrinks the scrollbar thumb when content overflows",
          "[paint][scrollbar][textarea]") {
  auto app = Ref<TextareaScrollbarApp>::New();
  app->text = "l1\nl2\nl3\nl4\nl5\nl6\nl7\nl8\nl9\nl10\nl11\nl12";  // 12 lines
                                                                    // in a
                                                                    // 5-row
                                                                    // viewport.
  auto texture = RenderTextarea(app, 40, 5);

  // The thumb only covers part of the track now, so the track color must
  // also be visible somewhere in the column: more than one distinct color.
  auto column = ScrollbarColumn(texture, 39, 5);
  CHECK(CountDistinct(column) > 1);
}

TEST_CASE("Textarea scrollbar thumb position tracks the scroll offset",
          "[paint][scrollbar][textarea]") {
  auto app = Ref<TextareaScrollbarApp>::New();
  app->text = "l1\nl2\nl3\nl4\nl5\nl6\nl7\nl8\nl9\nl10\nl11\nl12";
  app->Mount();
  app->Digest();

  auto* ta_el = app->Root()->QuerySelector("textarea");
  REQUIRE(ta_el != nullptr);

  auto layout_box = LayoutTreeBuilder::Build(app->Root());
  REQUIRE(layout_box);

  auto render_at_scroll = [&](int scroll_y) {
    ta_el->set_scroll_y(scroll_y);
    Texture texture(40, 5);
    for (int y = 0; y < 5; ++y) {
      for (int x = 0; x < 40; ++x) {
        texture[x, y].character = " ";
      }
    }
    LayoutConstraints constraints;
    constraints.width = {40, MeasureMode::Exactly};
    constraints.height = {5, MeasureMode::Exactly};
    auto fragment = RunLayout({layout_box.get()}, constraints);
    Paint(fragment.get(), texture);
    return ScrollbarColumn(texture, 39, 5);
  };

  auto column_top = render_at_scroll(0);
  auto column_bottom = render_at_scroll(7);  // scroll_height(12) - height(5)

  // The thumb moved from the top of the track to the bottom, so the
  // per-row colors at the two scroll positions must differ somewhere.
  CHECK(column_top != column_bottom);
}

}  // namespace rtxui
