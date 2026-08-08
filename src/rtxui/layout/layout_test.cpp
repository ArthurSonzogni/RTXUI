#include "rtxui/layout/layout.hpp"

#include <catch2/catch_test_macros.hpp>
#include <map>
#include <string>
#include <vector>

#include "rtxui/component/default_components_internal.hpp"
#include "rtxui/base/string.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/internal/component.hpp"
#include "rtxui/internal/refcounted.hpp"
#include "rtxui/layout/layout_tree_builder.hpp"
#include "rtxui/paint/paint.hpp"
#include "rtxui/paint/texture.hpp"

namespace rtxui {
namespace {

// Helper to extract the character layer of a texture as a grid string.
// Continuation cells (second half of a double-width grapheme) are shown as '^'.
std::string GetTextLayer(const Texture& texture) {
  std::string out;
  for (int y = 0; y < texture.height(); ++y) {
    for (int x = 0; x < texture.width(); ++x) {
      const auto& cell = const_cast<Texture&>(texture)[x, y];
      if (cell.is_continuation) {
        out += '^';  // marks the reserved column of a double-width character
      } else {
        out += cell.character.empty() ? " " : cell.character;
      }
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

  Texture texture(static_cast<uint8_t>(width), static_cast<uint8_t>(height));
  if (layout_box) {
    // Execute Layout algorithm.
    LayoutConstraints constraints;
    constraints.width = {width, MeasureMode::Exactly};
    constraints.height = {height, MeasureMode::Exactly};
    auto fragment = RunLayout({layout_box.get()}, constraints);

    // Paint the resulting fragments into a texture.
    Paint(fragment.get(), texture);
  }
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

}  // TEST_CASE "Layout: Borders"

TEST_CASE("Layout: Unicode rendering", "[layout][unicode]") {
  SECTION("Combining characters render into a single cell") {
    // é = e (U+0065) + combining acute accent (U+0301, UTF-8: \xCC\x81).
    // Both bytes must land in a single terminal cell of width 1.
    struct CombiningTest : Component<CombiningTest> {
      std::string_view Setup() {
        Import<div>();
        // Embed combining codepoint as raw UTF-8 — XML passes it through as-is.
        static const std::string html =
            "<style> .box { display: block; } </style>"
            "<div class=\"box\">cafe\xCC\x81</div>";
        return html;
      }
    };

    auto texture = RenderComponent(Ref<CombiningTest>::New(), 6, 1);
    // The combining grapheme (e + U+0301) must be stored in a single cell.
    const auto& cell3 = const_cast<Texture&>(texture)[3, 0];
    CHECK(cell3.character.size() > 1);      // multi-byte cluster in one cell
    CHECK(cell3.character == "e\xCC\x81");  // e + combining acute
    // GetTextLayer joins all cells: c a f e+\xCC\x81 _ _
    CHECK(GetTextLayer(texture) == "cafe\xCC\x81  \n");
  }

  SECTION("CJK double-width characters occupy two columns") {
    struct CjkTest : Component<CjkTest> {
      std::string_view Setup() {
        Import<div>();
        return R"html(
          <style> .box { display: block; } </style>
          <div class="box">AB</div>
        )html";
      }
    };
    // ASCII baseline: 'A' at col 0, 'B' at col 1.
    auto texture_ascii = RenderComponent(Ref<CjkTest>::New(), 4, 1);
    CHECK(GetTextLayer(texture_ascii) == "AB  \n");

    struct CjkTest2 : Component<CjkTest2> {
      std::string_view Setup() {
        Import<div>();
        return R"html(
          <style> .box { display: block; } </style>
          <div class="box">一B</div>
        )html";
      }
    };
    // '一' is double-width: occupies cols 0+1; 'B' lands at col 2.
    auto texture_cjk = RenderComponent(Ref<CjkTest2>::New(), 4, 1);
    std::string layer = GetTextLayer(texture_cjk);
    CHECK(layer == "一^B \n");

    // Verify the continuation cell flag is set
    const auto& cont = const_cast<Texture&>(texture_cjk)[1, 0];
    CHECK(cont.is_continuation);

    // Verify 'B' is at column 2
    const auto& b_cell = const_cast<Texture&>(texture_cjk)[2, 0];
    CHECK(b_cell.character == "B");
  }

  SECTION("Mixed ASCII and CJK alignment") {
    struct MixedTest : Component<MixedTest> {
      std::string_view Setup() {
        Import<div>();
        return R"html(
          <style> .box { display: block; } </style>
          <div class="box">A一B</div>
        )html";
      }
    };
    // 'A' at col 0, '一' at cols 1+2 (continuation at 2), 'B' at col 3.
    auto texture = RenderComponent(Ref<MixedTest>::New(), 5, 1);
    CHECK(GetTextLayer(texture) == "A一^B \n");
  }
}

TEST_CASE("Layout: display: none", "[layout][display]") {
  SECTION("Element with display: none takes no space") {
    struct DisplayNoneTest : Component<DisplayNoneTest> {
      std::string_view Setup() {
        Import<div>();
        Import<span>();
        return R"html(
          <style>
            .none { display: none; }
            .inline { display: inline; }
          </style>
          <div class="inline">
            <span>A</span>
            <span class="none">B</span>
            <span>C</span>
          </div>
        )html";
      }
    };
    auto texture = RenderComponent(Ref<DisplayNoneTest>::New(), 5, 1);
    CHECK(GetTextLayer(texture) == "AC   \n");
  }

  SECTION("Root element with display: none renders empty") {
    struct RootDisplayNoneTest : Component<RootDisplayNoneTest> {
      std::string_view Setup() {
        Import<div>();
        Import<span>();
        return R"html(
          <style>
            .root { display: none; }
          </style>
          <div class="root">
            <span>A</span>
          </div>
        )html";
      }
    };
    auto texture = RenderComponent(Ref<RootDisplayNoneTest>::New(), 5, 1);
    CHECK(GetTextLayer(texture) == "     \n");
  }
}

TEST_CASE("Layout: text-align", "[layout][text-align]") {
  SECTION("text-align: right with fixed width") {
    struct TestComponent : Component<TestComponent> {
      std::string_view Setup() {
        Import<div>();
        return R"html(
          <style>
            .container {
              display: block;
              width: 10;
              text-align: right;
            }
          </style>
          <div class="container">Hello</div>
        )html";
      }
    };
    auto texture = RenderComponent(Ref<TestComponent>::New(), 10, 1);
    CHECK(GetTextLayer(texture) == "     Hello\n");
  }

  SECTION("text-align: center with fixed width") {
    struct TestComponent : Component<TestComponent> {
      std::string_view Setup() {
        Import<div>();
        return R"html(
          <style>
            .container {
              display: block;
              width: 10;
              text-align: center;
            }
          </style>
          <div class="container">Hello</div>
        )html";
      }
    };
    auto texture = RenderComponent(Ref<TestComponent>::New(), 10, 1);
    // Hello is 5 cells. 10 - 5 = 5. Center shift = 5/2 = 2.
    // Result has 2 leading spaces, 5 text cells, and 3 trailing spaces.
    CHECK(GetTextLayer(texture) == "  Hello   \n");
  }

  SECTION("text-align: right with automatic wrapping") {
    struct TestComponent : Component<TestComponent> {
      std::string_view Setup() {
        Import<div>();
        return R"html(
          <style>
            .container {
              display: block;
              width: 4;
              text-align: right;
            }
          </style>
          <div class="container">A B C</div>
        )html";
      }
    };
    auto texture = RenderComponent(Ref<TestComponent>::New(), 4, 2);
    // "A B" wraps to line 1: width 3. Shift = 4 - 3 = 1 -> " A B"
    // "C" wraps to line 2: width 1. Shift = 4 - 1 = 3 -> "   C"
    CHECK(GetTextLayer(texture) ==
          " A B\n"
          "   C\n");
  }

  SECTION("text-align inheritance to nested components") {
    struct TestComponent : Component<TestComponent> {
      std::string_view Setup() {
        Import<div>();
        Import<span>();
        return R"html(
          <style>
            .container {
              display: block;
              width: 10;
              text-align: right;
            }
          </style>
          <div class="container">
            <span>A<span>B</span></span>
          </div>
        )html";
      }
    };
    auto texture = RenderComponent(Ref<TestComponent>::New(), 10, 1);
    CHECK(GetTextLayer(texture) == "        AB\n");
  }
}

TEST_CASE("Layout: white-space", "[layout][white-space]") {
  SECTION("white-space: nowrap prevents text wrapping") {
    struct TestComponent : Component<TestComponent> {
      std::string_view Setup() {
        Import<div>();
        return R"html(
          <style>
            .container {
              display: block;
              width: 4;
              white-space: nowrap;
            }
          </style>
          <div class="container">A B C</div>
        )html";
      }
    };
    auto texture = RenderComponent(Ref<TestComponent>::New(), 4, 1);
    CHECK(GetTextLayer(texture) == "A B \n");
  }

  SECTION("white-space: nowrap inheritance") {
    struct TestComponent : Component<TestComponent> {
      std::string_view Setup() {
        Import<div>();
        Import<span>();
        return R"html(
          <style>
            .container {
              display: block;
              width: 4;
              white-space: nowrap;
            }
          </style>
          <div class="container">
            <span>A B C</span>
          </div>
        )html";
      }
    };
    auto texture = RenderComponent(Ref<TestComponent>::New(), 4, 1);
    CHECK(GetTextLayer(texture) == "A B \n");
  }
}

// ─── Textarea rendering tests ───────────────────────────────────────────────

TEST_CASE("Layout: textarea single line rendering", "[layout][textarea]") {
  struct TestComponent : Component<TestComponent> {
    std::string text = "hello";
    std::string_view Setup() {
      Bind(text);
      Import<rtxui::textarea>();
      return R"html(
        <textarea class="ta" value="{text}" />
      )html";
    }
  };
  auto c = Ref<TestComponent>::New();
  c->Mount();
  c->Digest();

  auto layout_box = LayoutTreeBuilder::Build(c->Root());
  // The textarea has no border (removed). Default: width=40, height=5.
  // We render into 9x3 to observe the padding+content+blank rows.
  // Width 9: padding(1) + content(7) + padding(1)
  // Height 3: 3 of the 5 default rows visible
  Texture texture(9, 3);
  if (layout_box) {
    LayoutConstraints constraints;
    constraints.width = {9, MeasureMode::Exactly};
    constraints.height = {3, MeasureMode::Exactly};
    auto fragment = RunLayout({layout_box.get()}, constraints);
    Paint(fragment.get(), texture);
  }
  std::string layer = GetTextLayer(texture);
  INFO("Actual render: [" << layer << "]");
  // Row 0: content with padding:  " hello   " (padding-left=1, then text,
  // spaces) Row 1: blank row (fixed height, content shorter than 5 rows) Row 2:
  // blank row
  CHECK(layer ==
        " hello   \n"
        "         \n"
        "         \n");
}

TEST_CASE("Layout: textarea multiline rendering", "[layout][textarea]") {
  struct TestComponent : Component<TestComponent> {
    std::string text = "foo\nbar";
    std::string_view Setup() {
      Bind(text);
      Import<rtxui::textarea>();
      return R"html(
        <textarea class="ta" value="{text}" />
      )html";
    }
  };
  auto c = Ref<TestComponent>::New();
  c->Mount();
  c->Digest();

  auto layout_box = LayoutTreeBuilder::Build(c->Root());
  // The textarea has no border (removed). Default: width=40, height=5.
  // We render into 7x4 to observe the padding+content+blank rows.
  // Width 7: padding(1) + content(5) + padding(1)
  // Height 4: 4 of the 5 default rows visible
  Texture texture(7, 4);
  if (layout_box) {
    LayoutConstraints constraints;
    constraints.width = {7, MeasureMode::Exactly};
    constraints.height = {4, MeasureMode::Exactly};
    auto fragment = RunLayout({layout_box.get()}, constraints);
    Paint(fragment.get(), texture);
  }
  std::string layer = GetTextLayer(texture);
  INFO("Actual multiline render: [" << layer << "]");
  // Row 0: " foo   " (padding-left + cursor 'f' + "oo" + spaces)
  // Row 1: " bar   " (padding-left + "bar" + spaces)
  // Row 2: blank row
  // Row 3: blank row
  CHECK(layer ==
        " foo   \n"
        " bar   \n"
        "       \n"
        "       \n");
}

TEST_CASE("Layout: position absolute and relative", "[layout][position]") {
  struct TestComponent : Component<TestComponent> {
    std::string_view Setup() {
      return R"html(
        <div class="relative-parent">
          <div class="static-child">A</div>
          <div class="absolute-child">B</div>
        </div>
        <style>
          .relative-parent {
            position: relative;
            width: 5;
            height: 3;
            background-color: #000;
          }
          .static-child {
            width: 1;
            height: 1;
          }
          .absolute-child {
            position: absolute;
            top: 1;
            left: 2;
            width: 1;
            height: 1;
          }
        </style>
      )html";
    }
  };
  auto c = Ref<TestComponent>::New();
  c->Mount();
  c->Digest();

  auto layout_box = LayoutTreeBuilder::Build(c->Root());
  Texture texture(5, 3);
  for (int y = 0; y < 3; ++y) {
    for (int x = 0; x < 5; ++x) {
      texture[x, y].character = " ";
    }
  }
  if (layout_box) {
    LayoutConstraints constraints;
    constraints.width = {5, MeasureMode::Exactly};
    constraints.height = {3, MeasureMode::Exactly};
    auto fragment = RunLayout({layout_box.get()}, constraints);
    Paint(fragment.get(), texture);
  }
  std::string layer = GetTextLayer(texture);
  INFO("Position render:\n" << layer);
  CHECK(layer ==
        "A    \n"
        "  B  \n"
        "     \n");
}

TEST_CASE("Layout: z-index stacking", "[layout][z-index]") {
  struct TestComponent : Component<TestComponent> {
    std::string_view Setup() {
      return R"html(
        <div class="parent">
          <div class="absolute-child1">X</div>
          <div class="absolute-child2">Y</div>
        </div>
        <style>
          .parent {
            position: relative;
            width: 1;
            height: 1;
          }
          .absolute-child1 {
            position: absolute;
            top: 0;
            left: 0;
            width: 1;
            height: 1;
            z-index: 10;
          }
          .absolute-child2 {
            position: absolute;
            top: 0;
            left: 0;
            width: 1;
            height: 1;
            z-index: 5;
          }
        </style>
      )html";
    }
  };
  auto c = Ref<TestComponent>::New();
  c->Mount();
  c->Digest();

  auto layout_box = LayoutTreeBuilder::Build(c->Root());
  Texture texture(1, 1);
  texture[0, 0].character = " ";
  if (layout_box) {
    LayoutConstraints constraints;
    constraints.width = {1, MeasureMode::Exactly};
    constraints.height = {1, MeasureMode::Exactly};
    auto fragment = RunLayout({layout_box.get()}, constraints);
    Paint(fragment.get(), texture);
  }
  std::string layer = GetTextLayer(texture);
  INFO("Z-index render:\n" << layer);
  CHECK(layer == "X\n");
}

TEST_CASE("Layout: position fixed does not scroll", "[layout][fixed][scroll]") {
  struct TestComponent : Component<TestComponent> {
    std::string_view view = R"html(
      <div class="scrollable">
        <div class="spacer"></div>
        <div class="fixed-element">F</div>
      </div>
      <style>
        .scrollable {
          display: block;
          width: 5;
          height: 3;
          overflow-y: scroll;
        }
        .spacer {
          width: 5;
          height: 10;
        }
        .fixed-element {
          position: fixed;
          top: 1;
          left: 1;
          width: 1;
          height: 1;
        }
      </style>
    )html";
  };

  auto c = Ref<TestComponent>::New();
  c->Mount();
  c->Digest();

  // Scroll the scrollable container down by 2 cells
  auto* scrollable_element = c->Root()->QuerySelector(".scrollable");
  REQUIRE(scrollable_element != nullptr);
  scrollable_element->set_scroll_y(2);

  // Render
  auto layout_box = LayoutTreeBuilder::Build(c->Root());
  Texture texture(5, 3);
  for (int y = 0; y < 3; ++y) {
    for (int x = 0; x < 5; ++x) {
      texture[x, y].character = " ";
    }
  }
  if (layout_box) {
    LayoutConstraints constraints;
    constraints.width = {5, MeasureMode::Exactly};
    constraints.height = {3, MeasureMode::Exactly};
    auto fragment = RunLayout({layout_box.get()}, constraints);
    Paint(fragment.get(), texture);
  }

  std::string layer = GetTextLayer(texture);
  INFO("Scroll fixed render:\n" << layer);

  // The fixed element must remain at top:1, left:1 (which is x=1, y=1)
  // regardless of the parent scroll position.
  CHECK(texture[1, 1].character == "F");
}

TEST_CASE("Layout: position sticky layout and scrolling",
          "[layout][sticky][scroll]") {
  struct TestComponent : Component<TestComponent> {
    std::string_view view = R"html(
      <div class="scrollable">
        <div class="container">
          <div class="sticky-element">S</div>
          <div class="spacer">.</div>
        </div>
        <div class="extra-spacer"></div>
      </div>
      <style>
        .scrollable {
          display: block;
          width: 5;
          height: 5;
          overflow-y: scroll;
        }
        .container {
          display: block;
          width: 5;
          height: 10;
        }
        .sticky-element {
          position: sticky;
          top: 1;
          left: 0;
          width: 1;
          height: 1;
          z-index: 1;
        }
        .spacer {
          display: block;
          width: 5;
          height: 9;
        }
        .extra-spacer {
          display: block;
          width: 5;
          height: 10;
        }
      </style>
    )html";
  };

  {
    // 1. scroll_y = 0: normal_y is 0, but min_y is 1 (viewport_top 0 + top 1).
    // Sticky element should stick to y = 1.
    auto c = Ref<TestComponent>::New();
    c->Mount();
    c->Digest();

    auto* sticky_el = c->Root()->QuerySelector(".sticky-element");
    REQUIRE(sticky_el != nullptr);
    INFO("sticky_el position style: " << (int)sticky_el->style.position);
    CHECK(sticky_el->style.position == PositionType::Sticky);

    auto layout_box = LayoutTreeBuilder::Build(c->Root());
    Texture texture(5, 5);
    for (int y = 0; y < 5; ++y) {
      for (int x = 0; x < 5; ++x) {
        texture[x, y].character = " ";
      }
    }
    if (layout_box) {
      LayoutConstraints constraints;
      constraints.width = {5, MeasureMode::Exactly};
      constraints.height = {5, MeasureMode::Exactly};
      auto fragment = RunLayout({layout_box.get()}, constraints);
      Paint(fragment.get(), texture);
    }
    std::string layer = GetTextLayer(texture);
    INFO("Scroll 0 render:\n" << layer);
    CHECK(texture[0, 1].character == "S");
  }

  {
    // 2. scroll_y = 3: normal_y is -3, min_y is 1 (viewport_top 0 + top 1).
    // Sticky element should stick to y = 1.
    auto c = Ref<TestComponent>::New();
    c->Mount();
    c->Digest();

    auto* scrollable_element = c->Root()->QuerySelector(".scrollable");
    REQUIRE(scrollable_element != nullptr);
    scrollable_element->set_scroll_y(3);

    auto layout_box = LayoutTreeBuilder::Build(c->Root());
    Texture texture(5, 5);
    for (int y = 0; y < 5; ++y) {
      for (int x = 0; x < 5; ++x) {
        texture[x, y].character = " ";
      }
    }
    if (layout_box) {
      LayoutConstraints constraints;
      constraints.width = {5, MeasureMode::Exactly};
      constraints.height = {5, MeasureMode::Exactly};
      auto fragment = RunLayout({layout_box.get()}, constraints);
      Paint(fragment.get(), texture);
    }
    CHECK(texture[0, 1].character == "S");
  }

  {
    // 3. scroll_y = 9: normal_y is -9, min_y is 1.
    // However, parent container bottom is at 10 - 9 = 1.
    // The sticky element is height 1, so max_y limit is 1 - 1 = 0.
    // So std::min(1, 0) = 0. It should be at y = 0.
    auto c = Ref<TestComponent>::New();
    c->Mount();
    c->Digest();

    auto* scrollable_element = c->Root()->QuerySelector(".scrollable");
    REQUIRE(scrollable_element != nullptr);
    scrollable_element->set_scroll_y(9);

    auto layout_box = LayoutTreeBuilder::Build(c->Root());
    Texture texture(5, 5);
    for (int y = 0; y < 5; ++y) {
      for (int x = 0; x < 5; ++x) {
        texture[x, y].character = " ";
      }
    }
    if (layout_box) {
      LayoutConstraints constraints;
      constraints.width = {5, MeasureMode::Exactly};
      constraints.height = {5, MeasureMode::Exactly};
      auto fragment = RunLayout({layout_box.get()}, constraints);
      Paint(fragment.get(), texture);
    }
    std::string layer = GetTextLayer(texture);
    INFO("Scroll 9 render:\n" << layer);
    CHECK(texture[0, 0].character == "S");
  }
}

TEST_CASE("Layout: position sticky direct child of scroll container",
          "[layout][sticky][scroll]") {
  struct StickyDirectComponent : Component<StickyDirectComponent> {
    void InitReflection() override {
      Import<div>();
      Component<StickyDirectComponent>::InitReflection();
    }
    std::string_view view = R"html(
      <div class="scrollable">
        <div class="sticky-element">S</div>
        <div class="spacer">.</div>
      </div>
      <style>
        .scrollable {
          display: block;
          width: 5;
          height: 5;
          overflow-y: scroll;
        }
        .sticky-element {
          position: sticky;
          top: 0;
          left: 0;
          width: 1;
          height: 1;
          z-index: 1;
        }
        .spacer {
          display: block;
          width: 5;
          height: 20;
        }
      </style>
    )html";
  };

  auto c = Ref<StickyDirectComponent>::New();
  c->Mount();
  c->Digest();

  auto* scrollable_element = c->Root()->QuerySelector(".scrollable");
  REQUIRE(scrollable_element != nullptr);
  scrollable_element->set_scroll_y(10);

  auto layout_box = LayoutTreeBuilder::Build(c->Root());
  Texture texture(5, 5);
  for (int y = 0; y < 5; ++y) {
    for (int x = 0; x < 5; ++x) {
      texture[x, y].character = " ";
    }
  }
  if (layout_box) {
    LayoutConstraints constraints;
    constraints.width = {5, MeasureMode::Exactly};
    constraints.height = {5, MeasureMode::Exactly};
    auto fragment = RunLayout({layout_box.get()}, constraints);
    Paint(fragment.get(), texture);
  }
  std::string layer = GetTextLayer(texture);
  INFO("Scroll 10 direct render:\n" << layer);
  CHECK(texture[0, 0].character == "S");
}

TEST_CASE("Layout: Flexbox Grow Cumulative Distribution",
          "[layout][flex][grow]") {
  struct FlexGrowTest : Component<FlexGrowTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
        <style>
          .row { display: flex; flex-direction: row; }
          .item1 { flex-grow: 1.0; height: 1; background-color: rgb(255, 0, 0); }
          .item2 { flex-grow: 2.5; height: 1; background-color: rgb(0, 255, 0); }
          .item3 { flex-grow: 1.0; height: 1; background-color: rgb(0, 0, 255); }
        </style>
        <div class="row">
          <div class="item1">1</div>
          <div class="item2">2</div>
          <div class="item3">3</div>
        </div>
      )html";
    }
  };

  auto texture = RenderComponent(Ref<FlexGrowTest>::New(), 40, 1);

  std::map<Color, char> colors = {
      {Color::RGB(255, 0, 0), 'R'},
      {Color::RGB(0, 255, 0), 'G'},
      {Color::RGB(0, 0, 255), 'B'},
  };

  std::string color_layer = GetColorLayer(texture, true, colors);
  int count_r = 0;
  int count_g = 0;
  int count_b = 0;
  for (char c : color_layer) {
    if (c == 'R') {
      count_r++;
    } else if (c == 'G') {
      count_g++;
    } else if (c == 'B') {
      count_b++;
    }
  }

  // Under cumulative allocation, the total width must be exactly 40.
  CHECK(count_r + count_g + count_b == 40);
}

TEST_CASE("Layout: Text node in flexbox row regression",
          "[layout][flex][regression]") {
  SECTION("Wrapped text inside span does not throw") {
    struct FlexTextSpanTest : Component<FlexTextSpanTest> {
      std::string_view Setup() {
        Import<div>();
        Import<span>();
        return R"html(
          <style>
            .row { display: flex; flex-direction: row; }
          </style>
          <div class="row">
            <span>Valid text</span>
          </div>
        )html";
      }
    };
    CHECK_NOTHROW(RenderComponent(Ref<FlexTextSpanTest>::New(), 20, 1));
  }

  SECTION("Direct text child of flexbox does not throw and auto-wraps") {
    struct FlexDirectTextTest : Component<FlexDirectTextTest> {
      std::string_view Setup() {
        Import<div>();
        return R"html(
          <style>
            .row { display: flex; flex-direction: row; }
          </style>
          <div class="row">
            Valid direct text
          </div>
        )html";
      }
    };
    CHECK_NOTHROW(RenderComponent(Ref<FlexDirectTextTest>::New(), 20, 1));
  }
}

TEST_CASE("Layout: Flexbox Shrink Cumulative Distribution",
          "[layout][flex][shrink]") {
  struct FlexShrinkTest : Component<FlexShrinkTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
        <style>
          .row { display: flex; flex-direction: row; }
          .item1 { width: 20; flex-shrink: 1.0; height: 1; background-color: rgb(255, 0, 0); }
          .item2 { width: 20; flex-shrink: 2.0; height: 1; background-color: rgb(0, 255, 0); }
          .item3 { width: 20; flex-shrink: 1.0; height: 1; background-color: rgb(0, 0, 255); }
        </style>
        <div class="row">
          <div class="item1">1</div>
          <div class="item2">2</div>
          <div class="item3">3</div>
        </div>
      )html";
    }
  };

  auto texture = RenderComponent(Ref<FlexShrinkTest>::New(), 40, 1);

  std::map<Color, char> colors = {
      {Color::RGB(255, 0, 0), 'R'},
      {Color::RGB(0, 255, 0), 'G'},
      {Color::RGB(0, 0, 255), 'B'},
  };

  std::string color_layer = GetColorLayer(texture, true, colors);
  int count_r = 0;
  int count_g = 0;
  int count_b = 0;
  for (char c : color_layer) {
    if (c == 'R') {
      count_r++;
    } else if (c == 'G') {
      count_g++;
    } else if (c == 'B') {
      count_b++;
    }
  }

  // Under cumulative allocation, total width of flex row must shrink from 60
  // (20+20+20) to exactly 40.
  CHECK(count_r + count_g + count_b == 40);
}

TEST_CASE("Layout: Flexbox grow/shrink remainder allocated to final child",
          "[layout][flex][remainder]") {
  SECTION("Grow remainder allocation") {
    struct FlexRemainderGrowTest : Component<FlexRemainderGrowTest> {
      std::string_view Setup() {
        Import<div>();
        return R"html(
          <style>
            .row { display: flex; flex-direction: row; }
            .item1 { flex-grow: 1.0; height: 1; }
            .item2 { flex-grow: 1.0; height: 1; }
            .item3 { flex-grow: 1.0; height: 1; }
          </style>
          <div class="row">
            <div class="item1">1</div>
            <div class="item2">2</div>
            <div class="item3">3</div>
          </div>
        )html";
      }
    };

    auto app = Ref<FlexRemainderGrowTest>::New();
    auto texture = RenderComponent(app, 10, 1);
    auto* item1 = app->Root()->QuerySelector(".item1");
    auto* item2 = app->Root()->QuerySelector(".item2");
    auto* item3 = app->Root()->QuerySelector(".item3");
    REQUIRE(item1 != nullptr);
    REQUIRE(item2 != nullptr);
    REQUIRE(item3 != nullptr);

    // 10 free space cannot be divided evenly by 3 grow values (3.33 each).
    // The final child must receive the remainder, making the sizes exactly 3,
    // 3, 4.
    CHECK(item1->layout_width() == 3);
    CHECK(item2->layout_width() == 3);
    CHECK(item3->layout_width() == 4);
  }
}

TEST_CASE("Layout: Table Grid Rendering", "[layout][table]") {
  struct TableTest : Component<TableTest> {
    std::string_view Setup() override {
      return R"html(
        <table>
          <tr>
            <td>col1</td>
            <td>column2</td>
          </tr>
          <tr>
            <td>val1</td>
            <td>val2</td>
          </tr>
        </table>
      )html";
    }
  };

  auto app = Ref<TableTest>::New();
  auto texture = RenderComponent(app, 30, 2);

  auto* table = app->Root()->QuerySelector("table");
  REQUIRE(table != nullptr);
  CHECK(table->layout_width() > 0);
  CHECK(table->layout_height() == 2);

  std::string layout_text = GetTextLayer(texture);
  CHECK(layout_text.find("col1") != std::string::npos);
  CHECK(layout_text.find("column2") != std::string::npos);
  CHECK(layout_text.find("val1") != std::string::npos);
  CHECK(layout_text.find("val2") != std::string::npos);
}

// Regression test: LayoutTable applied max-width to its own box but never
// min-width - same gap class as the grid-container and inline-block fixes
// above, found by the same call-site audit.
TEST_CASE("Layout: min-width is enforced on a table", "[layout][table][min-width]") {
  struct T : Component<T> {
    std::string_view Setup() override {
      return R"html(
        <table style="min-width: 20;">
          <tr><td>X</td></tr>
        </table>
      )html";
    }
  };
  auto app = Ref<T>::New();
  RenderComponent(app, 30, 2);
  auto* table = app->Root()->QuerySelector("table");
  auto* cell = app->Root()->QuerySelector("td");
  REQUIRE(table != nullptr);
  REQUIRE(cell != nullptr);
  CHECK(table->layout_width() == 20);
  // The lone column must stretch to fill the widened table, not stay at its
  // 1-cell preferred width.
  CHECK(cell->layout_width() == 20);
}

// Regression test: when a table has more columns than available width, each
// column is floored at a 1-cell minimum and the sum is never shrunk back
// down or reported outward, so a wrapping `overflow: scroll` container (the
// standard way to make a table horizontally scrollable) previously
// underestimated how far it needed to let the user scroll - LayoutTable
// never called set_scroll_width/set_scroll_height at all, leaving them at
// their default of 0. The table's own reported width intentionally stays at
// its explicit/constrained value (matching how every other overflow case in
// this engine works: an explicit size is honored, content overflows it) -
// only scroll_width/scroll_height need to reflect the true extent.
TEST_CASE("Layout: a table wider than its box reports its true scroll_width",
          "[layout][table][scroll]") {
  struct T : Component<T> {
    void InitReflection() override {
      Import<div>();
      Component<T>::InitReflection();
    }
    std::string_view Setup() override {
      return R"html(
        <div class="wrap" style="width: 5; overflow-x: scroll;">
          <table>
            <tr>
              <td>1</td><td>2</td><td>3</td><td>4</td><td>5</td>
              <td>6</td><td>7</td><td>8</td><td>9</td><td>10</td>
            </tr>
          </table>
        </div>
      )html";
    }
  };
  auto app = Ref<T>::New();
  RenderComponent(app, 40, 5);

  auto* table = app->Root()->QuerySelector("table");
  auto* wrap = app->Root()->QuerySelector(".wrap");
  REQUIRE(table != nullptr);
  REQUIRE(wrap != nullptr);

  // 10 columns can't all fit in a width-5 box even at their 1-cell floor.
  CHECK(table->scroll_width() > table->layout_width());
  // The wrapping scrollable container must see the same true extent, not
  // just the table's own (smaller) reported width.
  CHECK(wrap->scroll_width() == table->scroll_width());
}

// Regression test: LayoutTable ignored `height`/min-height/max-height
// entirely - its own box height was always purely content-driven (sum of
// row heights), found by the same call-site audit as the tests above.
TEST_CASE("Layout: height/min-height/max-height are honored on a table",
          "[layout][table][height]") {
  SECTION("explicit height grows a 1-row table, stretching the row") {
    struct T : Component<T> {
      std::string_view Setup() override {
        return R"html(
          <table style="height: 10;">
            <tr><td>X</td></tr>
          </table>
        )html";
      }
    };
    auto app = Ref<T>::New();
    RenderComponent(app, 10, 20);
    auto* table = app->Root()->QuerySelector("table");
    auto* cell = app->Root()->QuerySelector("td");
    REQUIRE(table != nullptr);
    REQUIRE(cell != nullptr);
    CHECK(table->layout_height() == 10);
    CHECK(cell->layout_height() == 10);
  }

  SECTION("min-height grows the table the same way") {
    struct T : Component<T> {
      std::string_view Setup() override {
        return R"html(
          <table style="min-height: 10;">
            <tr><td>X</td></tr>
          </table>
        )html";
      }
    };
    auto app = Ref<T>::New();
    RenderComponent(app, 10, 20);
    auto* table = app->Root()->QuerySelector("table");
    REQUIRE(table != nullptr);
    CHECK(table->layout_height() == 10);
  }

  SECTION("max-height caps a taller table without touching its rows") {
    struct T : Component<T> {
      std::string_view Setup() override {
        return R"html(
          <table style="max-height: 2;">
            <tr><td>one</td></tr>
            <tr><td>two</td></tr>
            <tr><td>three</td></tr>
          </table>
        )html";
      }
    };
    auto app = Ref<T>::New();
    RenderComponent(app, 10, 20);
    auto* table = app->Root()->QuerySelector("table");
    REQUIRE(table != nullptr);
    CHECK(table->layout_height() == 2);
  }
}

TEST_CASE("Layout: <br> forces a line break in inline flow", "[layout][br]") {
  struct BrTest : Component<BrTest> {
    std::string_view Setup() override {
      return R"html(
        <div>one<br />two<br />three</div>
      )html";
    }
  };

  auto app = Ref<BrTest>::New();
  auto texture = RenderComponent(app, 10, 3);

  auto* div = app->Root()->QuerySelector("div");
  REQUIRE(div != nullptr);
  CHECK(div->layout_height() == 3);

  std::string layout_text = GetTextLayer(texture);
  CHECK(layout_text.find("one") != std::string::npos);
  CHECK(layout_text.find("two") != std::string::npos);
  CHECK(layout_text.find("three") != std::string::npos);

  // Each segment must land on its own row, not run together as "onetwothree".
  CHECK(layout_text.find("onetwo") == std::string::npos);
  CHECK(layout_text.find("twothree") == std::string::npos);
}

TEST_CASE("Layout: box-sizing", "[layout][box-sizing]") {
  SECTION("default (border-box): width already includes padding+border") {
    struct T : Component<T> {
      std::string_view Setup() override {
        return R"html(
          <div class="box">X</div>
          <style>
            .box { width: 10; padding-left: 3; border: solid; }
          </style>
        )html";
      }
    };
    auto app = Ref<T>::New();
    RenderComponent(app, 20, 5);
    auto* box = app->Root()->QuerySelector(".box");
    REQUIRE(box != nullptr);
    CHECK(box->layout_width() == 10);
  }

  SECTION("content-box: width excludes padding+border, so the box grows") {
    struct T : Component<T> {
      std::string_view Setup() override {
        return R"html(
          <div class="box">X</div>
          <style>
            .box {
              box-sizing: content-box;
              width: 10;
              padding-left: 3;
              border: solid;
            }
          </style>
        )html";
      }
    };
    auto app = Ref<T>::New();
    RenderComponent(app, 20, 5);
    auto* box = app->Root()->QuerySelector(".box");
    REQUIRE(box != nullptr);
    // 10 content + 3 padding-left + 1 border-left + 1 border-right.
    CHECK(box->layout_width() == 15);
  }

  SECTION("content-box: min-width also adds padding+border back") {
    struct T : Component<T> {
      std::string_view Setup() override {
        return R"html(
          <div style="width: 3;">
            <div class="box">X</div>
          </div>
          <style>
            .box {
              box-sizing: content-box;
              min-width: 10;
              padding-left: 2;
              border: solid;
            }
          </style>
        )html";
      }
    };
    auto app = Ref<T>::New();
    RenderComponent(app, 20, 5);
    auto* box = app->Root()->QuerySelector(".box");
    REQUIRE(box != nullptr);
    // 10 content (min-width floor, wider than the 3-cell parent) + 2
    // padding-left + 1 + 1 border.
    CHECK(box->layout_width() == 14);
  }

  SECTION("content-box: flex-basis is a content size too") {
    struct T : Component<T> {
      std::string_view Setup() override {
        return R"html(
          <div style="display: flex;">
            <div class="item">X</div>
          </div>
          <style>
            .item {
              box-sizing: content-box;
              flex-basis: 5;
              padding-left: 2;
              border: solid;
            }
          </style>
        )html";
      }
    };
    auto app = Ref<T>::New();
    RenderComponent(app, 20, 5);
    auto* item = app->Root()->QuerySelector(".item");
    REQUIRE(item != nullptr);
    // 5 content + 2 padding-left + 1 + 1 border.
    CHECK(item->layout_width() == 9);
  }
}

// Regression test: LayoutInlineFlow (used for inline-block, among others)
// applied max-width, min-height, and max-height, but never min-width -
// found while adding box-sizing regression tests above (a min-width test on
// an inline-block silently did nothing until this was fixed).
TEST_CASE("Layout: min-width is enforced on an inline-block", "[layout][min-width]") {
  struct T : Component<T> {
    std::string_view Setup() override {
      return R"html(
        <div class="box">X</div>
        <style>
          .box { display: inline-block; min-width: 10; }
        </style>
      )html";
    }
  };
  auto app = Ref<T>::New();
  RenderComponent(app, 20, 5);
  auto* box = app->Root()->QuerySelector(".box");
  REQUIRE(box != nullptr);
  CHECK(box->layout_width() == 10);
}

// Regression test: position:fixed elements must not be wrapped in anonymous
// inline boxes during layout tree building. Previously, if a fixed element
// appeared adjacent to an inline sibling, both were wrapped in an anonymous
// InlineFlow box. LayoutOutOfFlowChildren couldn't find the fixed element at
// the correct parent level, causing wrong viewport offset calculations and
// broken hit-testing after scrolling.
TEST_CASE("Layout: Fixed element not wrapped in anonymous inline box",
          "[layout]") {
  struct FixedNotWrappedTest : Component<FixedNotWrappedTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
        <style>
          #container {
            display: block;
            width: 20;
            height: 10;
            overflow-y: scroll;
          }
          #spacer {
            height: 8;
          }
          #inline_item {
            height: 1;
          }
          #fixed_item {
            position: fixed;
            left: 5;
            top: 2;
            width: 10;
            height: 1;
          }
        </style>
        <div id="container">
          <div id="spacer"></div>
          <div id="inline_item">Inline</div>
          <div id="fixed_item">Fixed</div>
        </div>
      )html";
    }
  };

  auto app = Ref<FixedNotWrappedTest>::New();
  app->Mount();

  auto layout_box = LayoutTreeBuilder::Build(app->Root());
  REQUIRE(layout_box != nullptr);

  LayoutConstraints constraints;
  constraints.width = {20, MeasureMode::Exactly};
  constraints.height = {10, MeasureMode::Exactly};
  auto fragment = RunLayout({layout_box.get()}, constraints);
  REQUIRE(fragment != nullptr);

  // The fixed item should be a direct child of the container fragment,
  // not nested inside an anonymous inline wrapper.
  // Find the container fragment (it has clips_descendants due to overflow).
  std::shared_ptr<PhysicalFragment> container_frag;
  std::function<void(const std::shared_ptr<PhysicalFragment>&)> find_container;
  find_container = [&](const std::shared_ptr<PhysicalFragment>& frag) {
    if (frag->dom_node && frag->dom_node->id == "container") {
      container_frag = frag;
      return;
    }
    for (const auto& child : frag->children) {
      find_container(child.fragment);
    }
  };
  find_container(fragment);
  REQUIRE(container_frag != nullptr);

  // Check that the fixed_item is a direct child of container, not nested
  // inside an anonymous wrapper.
  bool found_fixed_direct = false;
  for (const auto& child : container_frag->children) {
    if (child.fragment && child.fragment->dom_node &&
        child.fragment->dom_node->id == "fixed_item") {
      found_fixed_direct = true;
      // The fixed element should be positioned at top=2, left=5.
      CHECK(child.x == 5);
      CHECK(child.y == 2);
      break;
    }
  }
  CHECK(found_fixed_direct);

  // Verify position stays correct after scrolling and re-layout.
  auto* container_el = app->Root()->QuerySelector("#container");
  REQUIRE(container_el != nullptr);
  container_el->set_scroll_y(3);

  // Re-build layout tree and re-run layout.
  auto layout_box2 = LayoutTreeBuilder::Build(app->Root());
  auto fragment2 = RunLayout({layout_box2.get()}, constraints);
  REQUIRE(fragment2 != nullptr);

  std::shared_ptr<PhysicalFragment> container_frag2;
  std::function<void(const std::shared_ptr<PhysicalFragment>&)> find_container2;
  find_container2 = [&](const std::shared_ptr<PhysicalFragment>& frag) {
    if (frag->dom_node && frag->dom_node->id == "container") {
      container_frag2 = frag;
      return;
    }
    for (const auto& child : frag->children) {
      find_container2(child.fragment);
    }
  };
  find_container2(fragment2);
  REQUIRE(container_frag2 != nullptr);

  // After scrolling, the fixed element should still be a direct child
  // with the same position.
  bool found_fixed_after_scroll = false;
  for (const auto& child : container_frag2->children) {
    if (child.fragment && child.fragment->dom_node &&
        child.fragment->dom_node->id == "fixed_item") {
      found_fixed_after_scroll = true;
      CHECK(child.x == 5);
      CHECK(child.y == 2);
      break;
    }
  }
  CHECK(found_fixed_after_scroll);
}

TEST_CASE("Layout: Flexbox Row wrapping", "[layout][flex][wrap]") {
  struct FlexRowWrapTest : Component<FlexRowWrapTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
        <style>
          .container { display: flex; flex-direction: row; flex-wrap: wrap; width: 10; height: 3; }
          .item { width: 4; height: 1; }
          .r { background-color: rgb(255, 0, 0); }
          .g { background-color: rgb(0, 255, 0); }
          .b { background-color: rgb(0, 0, 255); }
        </style>
        <div class="container">
          <div class="item r">1</div>
          <div class="item g">2</div>
          <div class="item b">3</div>
        </div>
      )html";
    }
  };

  auto texture = RenderComponent(Ref<FlexRowWrapTest>::New(), 10, 3);
  std::map<Color, char> colors = {
      {Color::RGB(255, 0, 0), 'R'},
      {Color::RGB(0, 255, 0), 'G'},
      {Color::RGB(0, 0, 255), 'B'},
  };

  CHECK(GetColorLayer(texture, true, colors) == CheckGrid({
                                                    "RRRRGGGG..",
                                                    "..........",
                                                    "BBBB......",
                                                }));
}

TEST_CASE("Layout: Flexbox Row wrap-reverse", "[layout][flex][wrap]") {
  struct FlexRowWrapReverseTest : Component<FlexRowWrapReverseTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
        <style>
          .container { display: flex; flex-direction: row; flex-wrap: wrap-reverse; width: 10; height: 3; }
          .item { width: 4; height: 1; }
          .r { background-color: rgb(255, 0, 0); }
          .g { background-color: rgb(0, 255, 0); }
          .b { background-color: rgb(0, 0, 255); }
        </style>
        <div class="container">
          <div class="item r">1</div>
          <div class="item g">2</div>
          <div class="item b">3</div>
        </div>
      )html";
    }
  };

  auto texture = RenderComponent(Ref<FlexRowWrapReverseTest>::New(), 10, 3);
  std::map<Color, char> colors = {
      {Color::RGB(255, 0, 0), 'R'},
      {Color::RGB(0, 255, 0), 'G'},
      {Color::RGB(0, 0, 255), 'B'},
  };

  CHECK(GetColorLayer(texture, true, colors) == CheckGrid({
                                                    "BBBB......",
                                                    "RRRRGGGG..",
                                                    "..........",
                                                }));
}

TEST_CASE("Layout: Flexbox Column wrapping", "[layout][flex][wrap]") {
  struct FlexColWrapTest : Component<FlexColWrapTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
        <style>
          .container { display: flex; flex-direction: column; flex-wrap: wrap; width: 6; height: 4; }
          .item { width: 2; height: 2; }
          .r { background-color: rgb(255, 0, 0); }
          .g { background-color: rgb(0, 255, 0); }
          .b { background-color: rgb(0, 0, 255); }
        </style>
        <div class="container">
          <div class="item r">1</div>
          <div class="item g">2</div>
          <div class="item b">3</div>
        </div>
      )html";
    }
  };

  auto texture = RenderComponent(Ref<FlexColWrapTest>::New(), 6, 4);
  std::map<Color, char> colors = {
      {Color::RGB(255, 0, 0), 'R'},
      {Color::RGB(0, 255, 0), 'G'},
      {Color::RGB(0, 0, 255), 'B'},
  };

  CHECK(GetColorLayer(texture, true, colors) == CheckGrid({
                                                    "RR.BB.",
                                                    "RR.BB.",
                                                    "GG....",
                                                    "GG....",
                                                }));
}

TEST_CASE("Layout: Flexbox Column wrap-reverse", "[layout][flex][wrap]") {
  struct FlexColWrapReverseTest : Component<FlexColWrapReverseTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
        <style>
          .container { display: flex; flex-direction: column; flex-wrap: wrap-reverse; width: 6; height: 4; }
          .item { width: 2; height: 2; }
          .r { background-color: rgb(255, 0, 0); }
          .g { background-color: rgb(0, 255, 0); }
          .b { background-color: rgb(0, 0, 255); }
        </style>
        <div class="container">
          <div class="item r">1</div>
          <div class="item g">2</div>
          <div class="item b">3</div>
        </div>
      )html";
    }
  };

  auto texture = RenderComponent(Ref<FlexColWrapReverseTest>::New(), 6, 4);
  std::map<Color, char> colors = {
      {Color::RGB(255, 0, 0), 'R'},
      {Color::RGB(0, 255, 0), 'G'},
      {Color::RGB(0, 0, 255), 'B'},
  };

  CHECK(GetColorLayer(texture, true, colors) == CheckGrid({
                                                    "BB.RR.",
                                                    "BB.RR.",
                                                    "...GG.",
                                                    "...GG.",
                                                }));
}

TEST_CASE("Layout: Block children in Flex wrap container",
          "[layout][flex][block]") {
  struct BlockInFlexTest : Component<BlockInFlexTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
        <style>
          .container { display: flex; flex-direction: row; flex-wrap: wrap; width: 8; height: 2; }
          .block-item { display: block; width: 5; height: 1; background-color: rgb(255, 0, 0); }
          .block-item2 { display: block; width: 4; height: 1; background-color: rgb(0, 255, 0); }
        </style>
        <div class="container">
          <div class="block-item"></div>
          <div class="block-item2"></div>
        </div>
      )html";
    }
  };

  auto texture = RenderComponent(Ref<BlockInFlexTest>::New(), 8, 2);
  std::map<Color, char> colors = {
      {Color::RGB(255, 0, 0), 'R'},
      {Color::RGB(0, 255, 0), 'G'},
  };

  CHECK(GetColorLayer(texture, true, colors) == CheckGrid({
                                                    "RRRRR...",
                                                    "GGGG....",
                                                }));
}

TEST_CASE("Layout: Flex wrap container inside Block container",
          "[layout][flex][block]") {
  struct FlexInBlockTest : Component<FlexInBlockTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
        <style>
          .outer { display: block; width: 10; height: 4; }
          .container { display: flex; flex-direction: row; flex-wrap: wrap; width: 6; height: 2; margin-left: 2; }
          .item { width: 3; height: 1; }
          .r { background-color: rgb(255, 0, 0); }
          .g { background-color: rgb(0, 255, 0); }
          .b { background-color: rgb(0, 0, 255); }
        </style>
        <div class="outer">
          <div class="container">
            <div class="item r"></div>
            <div class="item g"></div>
            <div class="item b"></div>
          </div>
        </div>
      )html";
    }
  };

  auto texture = RenderComponent(Ref<FlexInBlockTest>::New(), 10, 4);
  std::map<Color, char> colors = {
      {Color::RGB(255, 0, 0), 'R'},
      {Color::RGB(0, 255, 0), 'G'},
      {Color::RGB(0, 0, 255), 'B'},
  };

  CHECK(GetColorLayer(texture, true, colors) == CheckGrid({
                                                    "..RRRGGG..",
                                                    "..BBB.....",
                                                    "..........",
                                                    "..........",
                                                }));
}

TEST_CASE("Layout: Flexbox Row Align Items Stretch", "[layout][flex][align]") {
  struct FlexRowStretchTest : Component<FlexRowStretchTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
        <style>
          .container { display: flex; flex-direction: row; align-items: stretch; width: 6; height: 4; }
          .item1 { width: 2; height: 2; background-color: rgb(255, 0, 0); }
          .item2 { width: 2; background-color: rgb(0, 255, 0); }
        </style>
        <div class="container">
          <div class="item1"></div>
          <div class="item2">X</div>
        </div>
      )html";
    }
  };

  auto texture = RenderComponent(Ref<FlexRowStretchTest>::New(), 6, 4);
  std::map<Color, char> colors = {
      {Color::RGB(255, 0, 0), 'R'},
      {Color::RGB(0, 255, 0), 'G'},
  };

  CHECK(GetColorLayer(texture, true, colors) == CheckGrid({
                                                    "RRGG..",
                                                    "RRGG..",
                                                    "..GG..",
                                                    "..GG..",
                                                }));
}

TEST_CASE("Layout: Flexbox Column Align Items Stretch",
          "[layout][flex][align]") {
  struct FlexColStretchTest : Component<FlexColStretchTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
        <style>
          .container { display: flex; flex-direction: column; align-items: stretch; width: 4; height: 6; }
          .item1 { height: 2; width: 2; background-color: rgb(255, 0, 0); }
          .item2 { height: 2; background-color: rgb(0, 255, 0); }
        </style>
        <div class="container">
          <div class="item1"></div>
          <div class="item2">Y</div>
        </div>
      )html";
    }
  };

  auto texture = RenderComponent(Ref<FlexColStretchTest>::New(), 4, 6);
  std::map<Color, char> colors = {
      {Color::RGB(255, 0, 0), 'R'},
      {Color::RGB(0, 255, 0), 'G'},
  };

  CHECK(GetColorLayer(texture, true, colors) == CheckGrid({
                                                    "RR..",
                                                    "RR..",
                                                    "GGGG",
                                                    "GGGG",
                                                    "....",
                                                    "....",
                                                }));
}

TEST_CASE("Layout: Flexbox Row Align Items Stretch With Wrap",
          "[layout][flex][align][wrap]") {
  struct FlexRowStretchWrapTest : Component<FlexRowStretchWrapTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
        <style>
          .container { display: flex; flex-direction: row; flex-wrap: wrap; align-items: stretch; width: 6; height: 6; }
          .item1 { width: 3; height: 2; background-color: rgb(255, 0, 0); }
          .item2 { width: 3; background-color: rgb(0, 255, 0); }
          .item3 { width: 3; background-color: rgb(0, 0, 255); }
          .item4 { width: 3; height: 1; background-color: rgb(255, 255, 0); }
        </style>
        <div class="container">
          <div class="item1"></div>
          <div class="item2">X</div>
          <div class="item3">Y</div>
          <div class="item4"></div>
        </div>
      )html";
    }
  };

  auto texture = RenderComponent(Ref<FlexRowStretchWrapTest>::New(), 6, 6);
  std::map<Color, char> colors = {
      {Color::RGB(255, 0, 0), 'R'},
      {Color::RGB(0, 255, 0), 'G'},
      {Color::RGB(0, 0, 255), 'B'},
      {Color::RGB(255, 255, 0), 'Y'},
  };

  CHECK(GetColorLayer(texture, true, colors) == CheckGrid({
                                                    "RRRGGG",
                                                    "RRRGGG",
                                                    "...GGG",
                                                    "...GGG",
                                                    "BBBYYY",
                                                    "BBB...",
                                                }));
}

TEST_CASE("Layout: Flexbox Row and Column Gap X and Y Only",
          "[layout][flex][gap]") {
  SECTION("Row layout with X-gap only") {
    struct FlexRowGapXTest : Component<FlexRowGapXTest> {
      std::string_view Setup() {
        Import<div>();
        return R"html(
          <style>
            .container { display: flex; flex-direction: row; flex-wrap: wrap; gap: 0 2; width: 8; height: 4; }
            .item1 { width: 3; height: 2; background-color: rgb(255, 0, 0); }
            .item2 { width: 3; height: 2; background-color: rgb(0, 255, 0); }
            .item3 { width: 3; height: 2; background-color: rgb(0, 0, 255); }
            .item4 { width: 3; height: 2; background-color: rgb(255, 255, 0); }
          </style>
          <div class="container">
            <div class="item1"></div>
            <div class="item2"></div>
            <div class="item3"></div>
            <div class="item4"></div>
          </div>
        )html";
      }
    };

    auto texture = RenderComponent(Ref<FlexRowGapXTest>::New(), 8, 4);
    std::map<Color, char> colors = {
        {Color::RGB(255, 0, 0), 'R'},
        {Color::RGB(0, 255, 0), 'G'},
        {Color::RGB(0, 0, 255), 'B'},
        {Color::RGB(255, 255, 0), 'Y'},
    };

    CHECK(GetColorLayer(texture, true, colors) == CheckGrid({
                                                      "RRR..GGG",
                                                      "RRR..GGG",
                                                      "BBB..YYY",
                                                      "BBB..YYY",
                                                  }));
  }

  SECTION("Row layout with Y-gap only") {
    struct FlexRowGapYTest : Component<FlexRowGapYTest> {
      std::string_view Setup() {
        Import<div>();
        return R"html(
          <style>
            .container { display: flex; flex-direction: row; flex-wrap: wrap; gap: 2 0; width: 6; height: 6; }
            .item1 { width: 3; height: 2; background-color: rgb(255, 0, 0); }
            .item2 { width: 3; height: 2; background-color: rgb(0, 255, 0); }
            .item3 { width: 3; height: 2; background-color: rgb(0, 0, 255); }
            .item4 { width: 3; height: 2; background-color: rgb(255, 255, 0); }
          </style>
          <div class="container">
            <div class="item1"></div>
            <div class="item2"></div>
            <div class="item3"></div>
            <div class="item4"></div>
          </div>
        )html";
      }
    };

    auto texture = RenderComponent(Ref<FlexRowGapYTest>::New(), 6, 6);
    std::map<Color, char> colors = {
        {Color::RGB(255, 0, 0), 'R'},
        {Color::RGB(0, 255, 0), 'G'},
        {Color::RGB(0, 0, 255), 'B'},
        {Color::RGB(255, 255, 0), 'Y'},
    };

    CHECK(GetColorLayer(texture, true, colors) == CheckGrid({
                                                      "RRRGGG",
                                                      "RRRGGG",
                                                      "......",
                                                      "......",
                                                      "BBBYYY",
                                                      "BBBYYY",
                                                  }));
  }
}

TEST_CASE("Layout: Flexbox Flex Basis and Overrides", "[layout][flex][basis]") {
  SECTION("flex-basis sets initial size") {
    struct FlexBasisTest : Component<FlexBasisTest> {
      std::string_view Setup() {
        Import<div>();
        return R"html(
          <style>
            .container { display: flex; flex-direction: row; width: 10; height: 1; }
            .item1 { flex-basis: 4; height: 1; background-color: rgb(255, 0, 0); }
            .item2 { flex-basis: 6; height: 1; background-color: rgb(0, 255, 0); }
          </style>
          <div class="container">
            <div class="item1"></div>
            <div class="item2"></div>
          </div>
        )html";
      }
    };

    auto texture = RenderComponent(Ref<FlexBasisTest>::New(), 10, 1);
    std::map<Color, char> colors = {
        {Color::RGB(255, 0, 0), 'R'},
        {Color::RGB(0, 255, 0), 'G'},
    };
    CHECK(GetColorLayer(texture, true, colors) == CheckGrid({
                                                      "RRRRGGGGGG",
                                                  }));
  }
}

TEST_CASE("Layout: Flexbox Align Self override", "[layout][flex][align-self]") {
  struct FlexAlignSelfTest : Component<FlexAlignSelfTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
        <style>
          .container { display: flex; flex-direction: row; align-items: flex-start; width: 6; height: 4; }
          .item1 { width: 3; height: 2; background-color: rgb(255, 0, 0); }
          .item2 { width: 3; height: 2; align-self: flex-end; background-color: rgb(0, 255, 0); }
        </style>
        <div class="container">
          <div class="item1"></div>
          <div class="item2"></div>
        </div>
      )html";
    }
  };

  auto texture = RenderComponent(Ref<FlexAlignSelfTest>::New(), 6, 4);
  std::map<Color, char> colors = {
      {Color::RGB(255, 0, 0), 'R'},
      {Color::RGB(0, 255, 0), 'G'},
  };
  CHECK(GetColorLayer(texture, true, colors) == CheckGrid({
                                                    "RRR...",
                                                    "RRR...",
                                                    "...GGG",
                                                    "...GGG",
                                                }));
}

TEST_CASE("Layout: Flexbox Align Content center",
          "[layout][flex][align-content]") {
  struct FlexAlignContentTest : Component<FlexAlignContentTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
        <style>
          .container { display: flex; flex-direction: row; flex-wrap: wrap; align-content: center; width: 3; height: 6; }
          .item1 { width: 3; height: 2; background-color: rgb(255, 0, 0); }
          .item2 { width: 3; height: 2; background-color: rgb(0, 255, 0); }
        </style>
        <div class="container">
          <div class="item1"></div>
          <div class="item2"></div>
        </div>
      )html";
    }
  };

  auto texture = RenderComponent(Ref<FlexAlignContentTest>::New(), 3, 6);
  std::map<Color, char> colors = {
      {Color::RGB(255, 0, 0), 'R'},
      {Color::RGB(0, 255, 0), 'G'},
  };
  CHECK(GetColorLayer(texture, true, colors) == CheckGrid({
                                                    "...",
                                                    "RRR",
                                                    "RRR",
                                                    "GGG",
                                                    "GGG",
                                                    "...",
                                                }));
}



TEST_CASE("Layout: Rowspan painting order", "[layout][table]") {
  struct RowspanPaintTest : Component<RowspanPaintTest> {
    std::string_view Setup() override {
      return R"html(
        <table>
          <tr>
            <td rowspan="2" style="background-color: rgb(0, 255, 0)">S</td>
            <td>A</td>
          </tr>
          <tr style="background-color: rgb(255, 0, 0)">
            <td>B</td>
          </tr>
        </table>
      )html";
    }
  };

  auto app = Ref<RowspanPaintTest>::New();
  auto texture = RenderComponent(app, 4, 2);

  std::map<Color, char> colors = {
      {Color::RGB(0, 255, 0), 'G'},
      {Color::RGB(255, 0, 0), 'R'},
  };
  std::string bg_grid = GetColorLayer(texture, true, colors);
  std::string text_grid = GetTextLayer(texture);
  
  INFO("BG Grid:\n" << bg_grid);
  INFO("Text Grid:\n" << text_grid);

  // Row 1
  CHECK(texture[0, 0].character == "S");
  CHECK(texture[0, 0].background_color == Color::RGB(0, 255, 0));
  
  // Row 2
  // Character is not repeated for multi-row cell, but background should span.
  CHECK(texture[0, 1].background_color == Color::RGB(0, 255, 0));
  
  CHECK(texture[1, 1].character == "B");
  CHECK(texture[1, 1].background_color == Color::RGB(255, 0, 0));
}

TEST_CASE("Layout: CSS Grid Layout basic positioning", "[layout][grid]") {
  struct GridBasicTest : Component<GridBasicTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
        <style>
          .container {
            display: grid;
            grid-template-columns: 2 4;
            grid-template-rows: 1 2;
            gap: 1;
            width: 7;
            height: 4;
          }
          .item {
            display: block;
          }
          .r { background-color: rgb(255, 0, 0); }
          .g { background-color: rgb(0, 255, 0); }
          .b { background-color: rgb(0, 0, 255); }
        </style>
        <div class="container">
          <div class="item r">1</div>
          <div class="item g">2</div>
          <div class="item b">3</div>
        </div>
      )html";
    }
  };

  auto texture = RenderComponent(Ref<GridBasicTest>::New(), 7, 4);
  std::map<Color, char> colors = {
      {Color::RGB(255, 0, 0), 'R'},
      {Color::RGB(0, 255, 0), 'G'},
      {Color::RGB(0, 0, 255), 'B'},
  };

  CHECK(GetColorLayer(texture, true, colors) == CheckGrid({
                                                    "RR.GGGG",
                                                    ".......",
                                                    "BB.....",
                                                    "BB.....",
                                                }));
}

// Regression test: LayoutGrid never applied min-width/max-width/min-height/
// max-height to its own container box - found while auditing every layout
// algorithm's size-resolution call sites for the box-sizing feature above.
TEST_CASE("Layout: min-width/max-width are enforced on a grid container",
          "[layout][grid][min-width][max-width]") {
  struct T : Component<T> {
    std::string_view Setup() override {
      return R"html(
        <div class="box">
          <div>X</div>
        </div>
        <style>
          .box { display: grid; min-width: 10; max-width: 15; }
        </style>
      )html";
    }
  };

  {
    auto app = Ref<T>::New();
    RenderComponent(app, 3, 5);
    auto* box = app->Root()->QuerySelector(".box");
    REQUIRE(box != nullptr);
    CHECK(box->layout_width() == 10);
  }
  {
    auto app = Ref<T>::New();
    RenderComponent(app, 20, 5);
    auto* box = app->Root()->QuerySelector(".box");
    REQUIRE(box != nullptr);
    CHECK(box->layout_width() == 15);
  }
}

TEST_CASE("Layout: CSS Grid Layout with fr units", "[layout][grid][fr]") {
  struct GridFrTest : Component<GridFrTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
        <style>
          .container {
            display: grid;
            grid-template-columns: 1fr 2fr;
            grid-template-rows: 1fr;
            width: 9;
            height: 2;
          }
          .item {
            display: block;
          }
          .r { background-color: rgb(255, 0, 0); }
          .g { background-color: rgb(0, 255, 0); }
        </style>
        <div class="container">
          <div class="item r"></div>
          <div class="item g"></div>
        </div>
      )html";
    }
  };

  auto texture = RenderComponent(Ref<GridFrTest>::New(), 9, 2);
  std::map<Color, char> colors = {
      {Color::RGB(255, 0, 0), 'R'},
      {Color::RGB(0, 255, 0), 'G'},
  };

  // 1fr + 2fr = 3fr. Total width = 9.
  // Col 0 = 3 wide. Col 1 = 6 wide.
  CHECK(GetColorLayer(texture, true, colors) == CheckGrid({
                                                    "RRRGGGGGG",
                                                    "RRRGGGGGG",
                                                }));
}

TEST_CASE("Layout: CSS Grid Layout with spans", "[layout][grid][span]") {
  struct GridSpanTest : Component<GridSpanTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
        <style>
          .container {
            display: grid;
            grid-template-columns: 2 2 2;
            grid-template-rows: 1 1;
            gap: 1;
            width: 8;
            height: 3;
          }
          .item {
            display: block;
          }
          .r { background-color: rgb(255, 0, 0); grid-column: span 2; }
          .g { background-color: rgb(0, 255, 0); }
          .b { background-color: rgb(0, 0, 255); grid-row: span 2; }
          .y { background-color: rgb(255, 255, 0); }
        </style>
        <div class="container">
          <div class="item r">1</div>
          <div class="item g">2</div>
          <div class="item b">3</div>
          <div class="item y">4</div>
        </div>
      )html";
    }
  };

  auto texture = RenderComponent(Ref<GridSpanTest>::New(), 8, 3);
  std::map<Color, char> colors = {
      {Color::RGB(255, 0, 0), 'R'},
      {Color::RGB(0, 255, 0), 'G'},
      {Color::RGB(0, 0, 255), 'B'},
      {Color::RGB(255, 255, 0), 'Y'},
  };

  CHECK(GetColorLayer(texture, true, colors) == CheckGrid({
                                                    "RRRRR.GG",
                                                    "........",
                                                    "BB.YY...",
                                                }));
}

TEST_CASE("Layout: CSS Grid Layout text containment", "[layout][grid][bug]") {
  struct GridTextTest : Component<GridTextTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
        <style>
          .container {
            display: grid;
            grid-template-columns: 8;
            grid-template-rows: 1fr;
            width: 8;
            height: 3;
          }
          .item {
            display: block;
            border: solid;
            padding: 1;
            background-color: rgb(255, 0, 0);
          }
        </style>
        <div class="container">
          <div class="item">AB</div>
        </div>
      )html";
    }
  };

  auto texture = RenderComponent(Ref<GridTextTest>::New(), 8, 5);
  std::string text_layer = GetTextLayer(texture);
  std::map<Color, char> colors = {
      {Color::RGB(255, 0, 0), 'R'},
  };
  std::string bg_layer = GetColorLayer(texture, true, colors);

  INFO("Text layer:\n" << text_layer);
  INFO("BG layer:\n" << bg_layer);

  // We check if the text 'AB' is drawn inside the padded area (inside the borders)
  // Grid item border takes x=0, x=7, y=0, y=4.
  // Padding takes x=1, x=6, y=1, y=3.
  // So text 'AB' should be at y=2, starting at x=2.
  CHECK(text_layer == CheckGrid({
                                    "┌──────┐",
                                    "│      │",
                                    "│ AB   │",
                                    "│      │",
                                    "└──────┘",
                                }));
}

TEST_CASE("Layout: Grid Tall Border Colors", "[layout][grid][tall]") {
  struct GridTallTest : Component<GridTallTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
        <style>
          .container {
            display: grid;
            grid-template-columns: 8;
            grid-template-rows: 3;
            width: 8;
            height: 3;
          }
          .item {
            display: block;
            border: tall;
            border-color: rgb(255, 0, 0);
            background-color: rgb(0, 255, 0);
          }
        </style>
        <div class="container">
          <div class="item"></div>
        </div>
      )html";
    }
  };

  auto texture = RenderComponent(Ref<GridTallTest>::New(), 8, 3);
  
  const auto& left_cell = texture[0, 0];
  INFO("left_cell fg: r=" << (int)left_cell.foreground_color.r 
                         << " g=" << (int)left_cell.foreground_color.g 
                         << " b=" << (int)left_cell.foreground_color.b 
                         << " a=" << (int)left_cell.foreground_color.a);
  CHECK(left_cell.character == "▊");
  CHECK(left_cell.foreground_color.a == 0);
  
  const auto& right_cell = texture[7, 0];
  INFO("right_cell bg: r=" << (int)right_cell.background_color.r 
                          << " g=" << (int)right_cell.background_color.g 
                          << " b=" << (int)right_cell.background_color.b 
                          << " a=" << (int)right_cell.background_color.a);
  CHECK(right_cell.character == "▎");
  CHECK(right_cell.background_color.a == 0);
}

TEST_CASE("Layout: position sticky pushing calendar test",
          "[layout][sticky][scroll]") {
  struct TestMonthSection : Component<TestMonthSection> {
    std::string name;
    void InitReflection() override {
      Import<div>();
      Bind(name);
      Component<TestMonthSection>::InitReflection();
    }
    std::string_view view = R"html(
      <div class="month-container">
        <div class="sticky-header">{name}</div>
        <div class="item">D</div>
        <div class="item">D</div>
      </div>
      <style>
        self { display: block; }
        .month-container { display: block; }
        .sticky-header {
          position: sticky;
          top: 0;
          height: 1;
          z-index: 1;
        }
        .item {
          display: block;
          height: 1;
        }
      </style>
    )html";
  };

  struct TestStickyDemo : Component<TestStickyDemo> {
    void InitReflection() override {
      Import<TestMonthSection>("month-section");
      Import<div>();
      Component<TestStickyDemo>::InitReflection();
    }
    std::string_view view = R"html(
      <div class="scroll-window">
        <month-section name="J" />
        <month-section name="F" />
        <div class="spacer-bottom"></div>
      </div>
      <style>
        self { display: block; }
        .scroll-window {
          display: block;
          height: 4;
          overflow-y: scroll;
        }
        .spacer-bottom {
          display: block;
          height: 10;
        }
      </style>
    )html";
  };

  auto c = Ref<TestStickyDemo>::New();
  c->Mount();
  c->Digest();

  auto* scrollable_element = c->Root()->QuerySelector(".scroll-window");
  REQUIRE(scrollable_element != nullptr);

  // 1. scroll_y = 0
  {
    scrollable_element->set_scroll_y(0);
    auto layout_box = LayoutTreeBuilder::Build(c->Root());
    Texture texture(5, 4);
    for (int y = 0; y < 4; ++y) {
      for (int x = 0; x < 5; ++x) {
        texture[x, y].character = " ";
      }
    }
    if (layout_box) {
      LayoutConstraints constraints;
      constraints.width = {5, MeasureMode::Exactly};
      constraints.height = {4, MeasureMode::Exactly};
      auto fragment = RunLayout({layout_box.get()}, constraints);
      Paint(fragment.get(), texture);
    }
    std::string layer = GetTextLayer(texture);
    INFO("Scroll 0 render:\n" << layer);
    CHECK(texture[0, 0].character == "J");
    CHECK(texture[0, 1].character == "D");
    CHECK(texture[0, 2].character == "D");
    CHECK(texture[0, 3].character == "F");
  }

  // 2. scroll_y = 1
  {
    scrollable_element->set_scroll_y(1);
    auto layout_box = LayoutTreeBuilder::Build(c->Root());
    Texture texture(5, 4);
    for (int y = 0; y < 4; ++y) {
      for (int x = 0; x < 5; ++x) {
        texture[x, y].character = " ";
      }
    }
    if (layout_box) {
      LayoutConstraints constraints;
      constraints.width = {5, MeasureMode::Exactly};
      constraints.height = {4, MeasureMode::Exactly};
      auto fragment = RunLayout({layout_box.get()}, constraints);
      Paint(fragment.get(), texture);
    }
    std::string layer = GetTextLayer(texture);
    INFO("Scroll 1 render:\n" << layer);
    CHECK(texture[0, 0].character == "J");
    CHECK(texture[0, 1].character == "D");
    CHECK(texture[0, 2].character == "F");
    CHECK(texture[0, 3].character == "D");
  }

  // 3. scroll_y = 2
  {
    scrollable_element->set_scroll_y(2);
    auto layout_box = LayoutTreeBuilder::Build(c->Root());
    Texture texture(5, 4);
    for (int y = 0; y < 4; ++y) {
      for (int x = 0; x < 5; ++x) {
        texture[x, y].character = " ";
      }
    }
    if (layout_box) {
      LayoutConstraints constraints;
      constraints.width = {5, MeasureMode::Exactly};
      constraints.height = {4, MeasureMode::Exactly};
      auto fragment = RunLayout({layout_box.get()}, constraints);
      Paint(fragment.get(), texture);
    }
    std::string layer = GetTextLayer(texture);
    INFO("Scroll 2 render:\n" << layer);
    CHECK(texture[0, 0].character == "J");
    CHECK(texture[0, 1].character == "F");
    CHECK(texture[0, 2].character == "D");
    CHECK(texture[0, 3].character == "D");
  }

  // 4. scroll_y = 3
  {
    scrollable_element->set_scroll_y(3);
    auto layout_box = LayoutTreeBuilder::Build(c->Root());
    Texture texture(5, 4);
    for (int y = 0; y < 4; ++y) {
      for (int x = 0; x < 5; ++x) {
        texture[x, y].character = " ";
      }
    }
    if (layout_box) {
      LayoutConstraints constraints;
      constraints.width = {5, MeasureMode::Exactly};
      constraints.height = {4, MeasureMode::Exactly};
      auto fragment = RunLayout({layout_box.get()}, constraints);
      Paint(fragment.get(), texture);
    }
    std::string layer = GetTextLayer(texture);
    INFO("Scroll 3 render:\n" << layer);
    CHECK(texture[0, 0].character == "F");
    CHECK(texture[0, 1].character == "D");
    CHECK(texture[0, 2].character == "D");
  }
}

TEST_CASE("Layout: InlineFlow respects height and min-height", "[layout][inline][height]") {
  struct TestInlineHeight : Component<TestInlineHeight> {
    void InitReflection() override {
      Import<div>();
      Component<TestInlineHeight>::InitReflection();
    }
    std::string_view view = R"html(
      <div class="container">
        <div class="inline-box">Item</div>
      </div>
      <style>
        .container {
          display: block;
        }
        .inline-box {
          display: inline-block;
          height: 3;
          width: 10;
        }
      </style>
    )html";
  };

  auto c = Ref<TestInlineHeight>::New();
  c->Mount();
  c->Digest();

  auto layout_box = LayoutTreeBuilder::Build(c->Root());
  REQUIRE(layout_box != nullptr);

  LayoutConstraints constraints;
  constraints.width = {40, MeasureMode::AtMost};
  constraints.height = {20, MeasureMode::AtMost};
  auto fragment = RunLayout({layout_box.get()}, constraints);
  REQUIRE(fragment != nullptr);

  auto* inline_box_el = c->Root()->QuerySelector(".inline-box");
  REQUIRE(inline_box_el != nullptr);
  CHECK(inline_box_el->layout_height() == 3);
  CHECK(inline_box_el->layout_width() == 10);
}

TEST_CASE("Layout: Absolute position centering via margin auto", "[layout][absolute][margin-auto]") {
  struct TestAbsoluteCentering : Component<TestAbsoluteCentering> {
    void InitReflection() override {
      Import<div>();
      Component<TestAbsoluteCentering>::InitReflection();
    }
    std::string_view view = R"html(
      <div class="container">
        <div class="child">X</div>
      </div>
      <style>
        .container {
          position: relative;
          width: 40;
          height: 20;
          display: block;
        }
        .child {
          position: absolute;
          left: 10;
          right: 10;
          top: 5;
          bottom: 5;
          width: 10;
          height: 4;
          margin: auto;
        }
      </style>
    )html";
  };

  auto c = Ref<TestAbsoluteCentering>::New();
  c->Mount();
  c->Digest();

  auto layout_box = LayoutTreeBuilder::Build(c->Root());
  REQUIRE(layout_box != nullptr);

  LayoutConstraints constraints;
  constraints.width = {40, MeasureMode::Exactly};
  constraints.height = {20, MeasureMode::Exactly};
  auto fragment = RunLayout({layout_box.get()}, constraints);
  REQUIRE(fragment != nullptr);

  Texture texture(40, 20);
  Paint(fragment.get(), texture);

  auto* child_el = c->Root()->QuerySelector(".child");
  REQUIRE(child_el != nullptr);
  CHECK(child_el->absolute_x() == 15);
  CHECK(child_el->absolute_y() == 8);
  CHECK(child_el->layout_width() == 10);
  CHECK(child_el->layout_height() == 4);
}

TEST_CASE("Layout: calc() width", "[layout][calc]") {
  struct CalcWidthTest : Component<CalcWidthTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
        <style>
          div { display: block; }
          .sized {
            width: calc(100% - 4);
            background-color: rgb(255, 0, 0);
          }
        </style>
        <div class="sized">X</div>
      )html";
    }
  };

  auto texture = RenderComponent(Ref<CalcWidthTest>::New(), 10, 1);

  std::map<Color, char> colors = {
      {Color::RGB(255, 0, 0), 'R'},
  };

  // 100% of 10 minus 4 = 6 cells wide.
  CHECK(GetColorLayer(texture, true, colors) == CheckGrid({
                                                    "RRRRRR....",
                                                }));
}

TEST_CASE("Layout: text-align justify stretches wrapped lines",
          "[layout][text-align][justify]") {
  struct JustifyTest : Component<JustifyTest> {
    std::string_view Setup() {
      Import<div>();
      return "<style>div { display: block; text-align: justify; }</style>"
             "<div>aa bb cc dd ee</div>";
    }
  };

  auto texture = RenderComponent(Ref<JustifyTest>::New(), 10, 2);

  // "aa bb cc" (8 wide) is justified to 10 by widening both gaps;
  // the last line is never justified.
  CHECK(GetTextLayer(texture) == CheckGrid({
                                     "aa  bb  cc",
                                     "dd ee     ",
                                 }));
}

TEST_CASE("Layout: justify skips hard-break lines",
          "[layout][text-align][justify]") {
  struct JustifyBreakTest : Component<JustifyBreakTest> {
    std::string_view Setup() {
      Import<div>();
      return "<style>div { display: block; text-align: justify;"
             " white-space: pre-wrap; }</style>"
             "<div>a b\ncc dd ee ff</div>";
    }
  };

  auto texture = RenderComponent(Ref<JustifyBreakTest>::New(), 8, 3);

  // "a b" ends with an explicit newline: not justified.
  // "cc dd ee" wraps: justified to 8. "ff" is the last line.
  CHECK(GetTextLayer(texture) == CheckGrid({
                                     "a b     ",
                                     "cc dd ee",
                                     "ff      ",
                                 }));
}

TEST_CASE("Layout: aspect-ratio derives height from width", "[layout][aspect-ratio]") {
  struct AspectRatioTest : Component<AspectRatioTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
        <style>
          div { display: block; }
          .tile {
            width: 8;
            aspect-ratio: 4 / 1;
            background-color: rgb(255, 0, 0);
          }
        </style>
        <div class="tile"></div>
      )html";
    }
  };

  auto texture = RenderComponent(Ref<AspectRatioTest>::New(), 10, 3);

  std::map<Color, char> colors = {
      {Color::RGB(255, 0, 0), 'R'},
  };

  // Width 8 with ratio 4:1 -> height 2.
  CHECK(GetColorLayer(texture, true, colors) == CheckGrid({
                                                    "RRRRRRRR..",
                                                    "RRRRRRRR..",
                                                    "..........",
                                                }));
}

TEST_CASE("Layout: aspect-ratio in flex context", "[layout][aspect-ratio][flex]") {
  struct FlexAspectTest : Component<FlexAspectTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
        <style>
          .row {
            display: flex;
            flex-direction: row;
            align-items: flex-start;
          }
          .tile {
            width: 6;
            aspect-ratio: 3 / 1;
            background-color: rgb(255, 0, 0);
          }
        </style>
        <div class="row"><div class="tile"></div></div>
      )html";
    }
  };

  auto texture = RenderComponent(Ref<FlexAspectTest>::New(), 8, 3);

  std::map<Color, char> colors = {
      {Color::RGB(255, 0, 0), 'R'},
  };

  // A flex item 6 wide with ratio 3:1 is 2 tall.
  CHECK(GetColorLayer(texture, true, colors) == CheckGrid({
                                                    "RRRRRR..",
                                                    "RRRRRR..",
                                                    "........",
                                                }));
}

TEST_CASE("Layout: aspect-ratio on a flex container",
          "[layout][aspect-ratio][flex]") {
  struct FlexContainerAspectTest : Component<FlexContainerAspectTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
        <style>
          .rowc {
            display: flex;
            width: 8;
            aspect-ratio: 4 / 1;
            background-color: rgb(0, 0, 255);
          }
        </style>
        <div class="rowc"></div>
      )html";
    }
  };

  auto texture = RenderComponent(Ref<FlexContainerAspectTest>::New(), 10, 3);

  std::map<Color, char> colors = {
      {Color::RGB(0, 0, 255), 'B'},
  };

  CHECK(GetColorLayer(texture, true, colors) == CheckGrid({
                                                    "BBBBBBBB..",
                                                    "BBBBBBBB..",
                                                    "..........",
                                                }));
}

TEST_CASE("Layout: aspect-ratio in grid context", "[layout][aspect-ratio][grid]") {
  struct GridAspectTest : Component<GridAspectTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
        <style>
          .grid {
            display: grid;
            grid-template-columns: 8;
          }
          .tile {
            width: 6;
            aspect-ratio: 3 / 1;
            background-color: rgb(255, 0, 0);
          }
        </style>
        <div class="grid"><div class="tile"></div></div>
      )html";
    }
  };

  auto texture = RenderComponent(Ref<GridAspectTest>::New(), 10, 3);

  std::map<Color, char> colors = {
      {Color::RGB(255, 0, 0), 'R'},
  };

  // The ratio height derives from the item's used width (6), not from its
  // 8-wide track (regression: the row used to become 3 tall).
  CHECK(GetColorLayer(texture, true, colors) == CheckGrid({
                                                    "RRRRRR....",
                                                    "RRRRRR....",
                                                    "..........",
                                                }));
}

TEST_CASE("Layout: aspect-ratio on a grid container",
          "[layout][aspect-ratio][grid]") {
  struct GridContainerAspectTest : Component<GridContainerAspectTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
        <style>
          .gridc {
            display: grid;
            grid-template-columns: 8;
            aspect-ratio: 4 / 1;
            background-color: rgb(0, 128, 0);
          }
          .item { background-color: rgb(255, 0, 0); }
        </style>
        <div class="gridc"><div class="item">A</div></div>
      )html";
    }
  };

  auto texture = RenderComponent(Ref<GridContainerAspectTest>::New(), 10, 3);

  std::map<Color, char> colors = {
      {Color::RGB(0, 128, 0), 'G'},
      {Color::RGB(255, 0, 0), 'R'},
  };

  // The container is 8 wide, so ratio 4:1 gives it 2 rows; the single
  // 1-row item leaves the second row showing the container background.
  CHECK(GetColorLayer(texture, true, colors) == CheckGrid({
                                                    "RRRRRRRR..",
                                                    "GGGGGGGG..",
                                                    "..........",
                                                }));
}

TEST_CASE("Layout: aspect-ratio derives a grid container's width from height",
          "[layout][aspect-ratio][grid]") {
  struct GridContainerAspectWidthTest
      : Component<GridContainerAspectWidthTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
        <style>
          .gridc {
            display: grid;
            grid-template-rows: 1fr;
            height: 2;
            aspect-ratio: 4 / 1;
            background-color: rgb(0, 0, 255);
          }
        </style>
        <div class="gridc"></div>
      )html";
    }
  };

  auto texture = RenderComponent(Ref<GridContainerAspectWidthTest>::New(), 10, 3);

  std::map<Color, char> colors = {
      {Color::RGB(0, 0, 255), 'B'},
  };

  // Height 2 with ratio 4:1 -> width 8, resolved before the single 1fr
  // column is sized so the column fills the derived width.
  CHECK(GetColorLayer(texture, true, colors) == CheckGrid({
                                                    "BBBBBBBB..",
                                                    "BBBBBBBB..",
                                                    "..........",
                                                }));
}

TEST_CASE("Layout: aspect-ratio derives width from height", "[layout][aspect-ratio]") {
  struct AspectRatioWidthTest : Component<AspectRatioWidthTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
        <style>
          div { display: block; }
          .tile {
            height: 2;
            aspect-ratio: 4 / 1;
            background-color: rgb(255, 0, 0);
          }
        </style>
        <div class="tile"></div>
      )html";
    }
  };

  auto texture = RenderComponent(Ref<AspectRatioWidthTest>::New(), 10, 3);

  std::map<Color, char> colors = {
      {Color::RGB(255, 0, 0), 'R'},
  };

  // Height 2 with ratio 4:1 -> width 8.
  CHECK(GetColorLayer(texture, true, colors) == CheckGrid({
                                                    "RRRRRRRR..",
                                                    "RRRRRRRR..",
                                                    "..........",
                                                }));
}

TEST_CASE("Layout: aspect-ratio derives width from height in a flex column",
          "[layout][aspect-ratio][flex]") {
  struct FlexColumnAspectTest : Component<FlexColumnAspectTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
        <style>
          .col {
            display: flex;
            flex-direction: column;
            height: 2;
            align-items: flex-start;
          }
          .tile {
            flex-grow: 1;
            aspect-ratio: 3 / 1;
            background-color: rgb(255, 0, 0);
          }
        </style>
        <div class="col"><div class="tile"></div></div>
      )html";
    }
  };

  auto texture = RenderComponent(Ref<FlexColumnAspectTest>::New(), 8, 3);

  std::map<Color, char> colors = {
      {Color::RGB(255, 0, 0), 'R'},
  };

  // A flex column item stretched to height 2 with ratio 3:1 is 6 wide.
  CHECK(GetColorLayer(texture, true, colors) == CheckGrid({
                                                    "RRRRRR..",
                                                    "RRRRRR..",
                                                    "........",
                                                }));
}

TEST_CASE("Layout: aspect-ratio derives a flex container's width from height",
          "[layout][aspect-ratio][flex]") {
  struct FlexContainerAspectWidthTest
      : Component<FlexContainerAspectWidthTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
        <style>
          .colc {
            display: flex;
            flex-direction: column;
            height: 2;
            aspect-ratio: 4 / 1;
            background-color: rgb(0, 0, 255);
          }
        </style>
        <div class="colc"></div>
      )html";
    }
  };

  auto texture = RenderComponent(Ref<FlexContainerAspectWidthTest>::New(), 10, 3);

  std::map<Color, char> colors = {
      {Color::RGB(0, 0, 255), 'B'},
  };

  CHECK(GetColorLayer(texture, true, colors) == CheckGrid({
                                                    "BBBBBBBB..",
                                                    "BBBBBBBB..",
                                                    "..........",
                                                }));
}

TEST_CASE("Layout: aspect-ratio transfers a stretched cross size to a flex "
          "row item's width",
          "[layout][aspect-ratio][flex]") {
  struct FlexRowStretchAspectTest : Component<FlexRowStretchAspectTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
        <style>
          .row {
            display: flex;
            flex-direction: row;
            height: 2;
            align-items: stretch;
          }
          .tile {
            aspect-ratio: 3 / 1;
            background-color: rgb(255, 0, 0);
          }
        </style>
        <div class="row"><div class="tile"></div></div>
      )html";
    }
  };

  auto texture = RenderComponent(Ref<FlexRowStretchAspectTest>::New(), 8, 3);

  std::map<Color, char> colors = {
      {Color::RGB(255, 0, 0), 'R'},
  };

  // The item is stretched to the container's height (2); ratio 3:1 must
  // transfer that into a width of 6 basis, before the flex algorithm locks
  // width in as Exactly (regression: the item used to collapse to 0 wide,
  // since block-flow's own aspect-ratio-from-height runs too late - width
  // is already forced Exactly by the time height becomes definite).
  CHECK(GetColorLayer(texture, true, colors) == CheckGrid({
                                                    "RRRRRR..",
                                                    "RRRRRR..",
                                                    "........",
                                                }));
}

TEST_CASE("Layout: min() width caps percentage", "[layout][calc][minmax]") {
  struct MinWidthTest : Component<MinWidthTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
        <style>
          div { display: block; }
          .sized {
            width: min(100%, 6);
            background-color: rgb(255, 0, 0);
          }
        </style>
        <div class="sized">X</div>
      )html";
    }
  };

  auto texture = RenderComponent(Ref<MinWidthTest>::New(), 10, 1);

  std::map<Color, char> colors = {
      {Color::RGB(255, 0, 0), 'R'},
  };

  CHECK(GetColorLayer(texture, true, colors) == CheckGrid({
                                                    "RRRRRR....",
                                                }));
}

TEST_CASE("Layout: grid justify-items and align-self", "[layout][grid][justify]") {
  struct GridJustifyTest : Component<GridJustifyTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
        <style>
          .grid {
            display: grid;
            grid-template-columns: 6 6;
            justify-items: center;
          }
          .item { display: block; width: 2; background-color: rgb(255, 0, 0); }
          .end { justify-self: end; }
        </style>
        <div class="grid">
          <div class="item">A</div>
          <div class="item end">B</div>
        </div>
      )html";
    }
  };

  auto texture = RenderComponent(Ref<GridJustifyTest>::New(), 12, 1);

  std::map<Color, char> colors = {
      {Color::RGB(255, 0, 0), 'R'},
  };

  // Column 1 (cells 0-5): 2-wide item centered -> offset 2.
  // Column 2 (cells 6-11): justify-self end -> offset 4 within the column.
  CHECK(GetColorLayer(texture, true, colors) == CheckGrid({
                                                    "..RR......RR",
                                                }));
  CHECK(GetTextLayer(texture) == CheckGrid({
                                     "  A       B ",
                                 }));
}

TEST_CASE("Layout: white-space pre-line collapses runs", "[layout][white-space]") {
  struct PreLineTest : Component<PreLineTest> {
    std::string_view Setup() {
      Import<div>();
      return "<style>div { display: block; white-space: pre-line; }</style>"
             "<div>a   b  \n   c</div>";
    }
  };

  auto texture = RenderComponent(Ref<PreLineTest>::New(), 10, 2);

  CHECK(GetTextLayer(texture) == CheckGrid({
                                     "a b       ",
                                     "c         ",
                                 }));
}

TEST_CASE("Layout: white-space pre-wrap wraps long lines", "[layout][white-space]") {
  struct PreWrapTest : Component<PreWrapTest> {
    std::string_view Setup() {
      Import<div>();
      return "<style>div { display: block; white-space: pre-wrap; }</style>"
             "<div>word1 word2</div>";
    }
  };

  auto texture = RenderComponent(Ref<PreWrapTest>::New(), 6, 2);

  CHECK(GetTextLayer(texture) == CheckGrid({
                                     "word1 ",
                                     "word2 ",
                                 }));
}

// Regression test: a shrink-to-fit flex item (no explicit width) whose own
// children are block-level (not a single text/inline node) used to report a
// bogus width of ~10000 instead of shrinking to its content. Cause: the
// intrinsic-measurement placeholder bound (10000, standing in for
// "unbounded" while a parent's own shrink-to-fit width is still unknown) was
// handed to auto-width block children as a real MeasureMode::AtMost
// constraint, which normal block layout correctly fills -- so a child would
// report "10000" as its own natural width, corrupting the parent's
// max-child-width shrink-to-fit sum. Fixed by propagating
// MeasureMode::Undefined (not AtMost) to auto-width block children whenever
// the parent itself is being shrink-to-fit measured.
TEST_CASE("Layout: flex item with auto width shrinks to its block children",
          "[layout][flex][shrink-to-fit][bug]") {
  struct ShrinkToFitTest : Component<ShrinkToFitTest> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
        <div class="row">
          <div class="side">
            <div>1</div>
            <div>2</div>
            <div>3</div>
          </div>
          <div class="main">rest</div>
        </div>
        <style>
          .row { display: flex; flex-direction: row; align-items: flex-start; }
          .side { display: block; flex-shrink: 0; }
          .main { display: block; flex-grow: 1; }
        </style>
      )html";
    }
  };

  auto container = Ref<ShrinkToFitTest>::New();
  auto texture = RenderComponent(container, 10, 3);

  auto* side_el = container->Root()->QuerySelector(".side");
  auto* main_el = container->Root()->QuerySelector(".main");
  REQUIRE(side_el != nullptr);
  REQUIRE(main_el != nullptr);
  CHECK(side_el->layout_width() == 1);
  CHECK(main_el->layout_width() == 9);

  CHECK(GetTextLayer(texture) == CheckGrid({
                                     "1rest     ",
                                     "2         ",
                                     "3         ",
                                 }));
}

// Regression test: ::part() lets an outer component style an element buried
// inside a nested component's own template (normally unreachable -- see
// IsStyledByComponent, which gates every other selector bucket to elements
// a component either rendered itself or directly instantiated). Two bugs
// were found and fixed getting this to actually apply:
// 1. every XML tag is itself a Component (see the tag registry in
//    component.cpp), so `element->component()` gives the immediate
//    per-tag wrapper, not the outer instantiation boundary -- matching
//    has to walk up the DOM parent chain via owner_component() instead.
// 2. ResolveStylesRecursive skips descending into a nested component's
//    subtree entirely when it has no slotted content, since normally
//    nothing outer could ever reach in there -- an optimization that
//    silently broke ::part() once it made that reachable.
TEST_CASE("Layout: ::part() lets an outer component style a nested "
          "component's internals",
          "[layout][style][part]") {
  struct InnerWithPart : Component<InnerWithPart> {
    std::string_view Setup() {
      Import<div>();
      return R"html(
        <div class="label" part="gizmo">X</div>
        <style>
          self { display: block; }
          .label { color: rgb(0, 255, 0); }
        </style>
      )html";
    }
  };

  struct OuterWithPartRule : Component<OuterWithPartRule> {
    std::string_view Setup() {
      Import<InnerWithPart>();
      return R"html(
        <InnerWithPart class="thing" />
        <style>
          .thing::part(gizmo) { color: rgb(255, 0, 0); }
        </style>
      )html";
    }
  };

  auto texture = RenderComponent(Ref<OuterWithPartRule>::New(), 1, 1);

  std::map<Color, char> colors = {
      {Color::RGB(255, 0, 0), 'R'},
      {Color::RGB(0, 255, 0), 'G'},
  };
  // Red (the outer ::part() override), not green (Inner's own default).
  CHECK(GetColorLayer(texture, false, colors) == CheckGrid({"R"}));
}

// Regression test: LayoutInlineFlow (used for inline-block, among other
// things) never called set_scroll_width/set_scroll_height at all, unlike
// LayoutBlockFlow/LayoutFlex which both fully support being a scroll
// container. An inline-block with a fixed width, white-space:nowrap text
// wider than it, and overflow-x:scroll had no way to reach the overflowing
// text: scroll_width defaulted to 0, so ClampScrollX always clamped to 0.
TEST_CASE("Layout: an inline-block reports scroll_width for overflowing "
          "nowrap text",
          "[layout][inline-block][scroll]") {
  struct T : Component<T> {
    void InitReflection() override {
      Import<div>();
      Component<T>::InitReflection();
    }
    std::string_view Setup() override {
      return R"html(
        <div class="ib" style="display: inline-block; width: 3; overflow-x: scroll; white-space: nowrap;">HelloWorldLongText</div>
      )html";
    }
  };
  auto app = Ref<T>::New();
  RenderComponent(app, 40, 5);
  auto* ib = app->Root()->QuerySelector(".ib");
  REQUIRE(ib != nullptr);

  CHECK(ib->layout_width() == 3);
  // "HelloWorldLongText" is 18 cells - the box's own reported width stays
  // at the explicit 3, but scroll_width must expose the true extent.
  CHECK(ib->scroll_width() >= 18);
}

// Regression test: LayoutGrid (like LayoutTable and LayoutInlineFlow above)
// never called set_scroll_width/set_scroll_height, unlike LayoutBlockFlow
// and LayoutFlex. A grid with explicit column tracks wider than its
// max-width had no way to expose the true content extent.
TEST_CASE("Layout: a grid container reports scroll_width when its tracks "
          "overflow max-width",
          "[layout][grid][scroll]") {
  struct T : Component<T> {
    std::string_view Setup() override {
      return R"html(
        <div class="grid" style="display: grid; grid-template-columns: 5 5 5 5; max-width: 5;">
          <div>a</div><div>b</div><div>c</div><div>d</div>
        </div>
      )html";
    }
  };
  auto app = Ref<T>::New();
  RenderComponent(app, 40, 5);
  auto* grid = app->Root()->QuerySelector(".grid");
  REQUIRE(grid != nullptr);

  // max-width clamps the box's own reported size...
  CHECK(grid->layout_width() == 5);
  // ...but the 4 explicit 5-cell columns (20 total) must still be reachable
  // via scroll_width.
  CHECK(grid->scroll_width() == 20);
}

}  // namespace rtxui
