#include "rtxui/layout/layout.hpp"

#include <catch2/catch_test_macros.hpp>
#include <map>
#include <string>
#include <vector>

#include "rtxui/component/default_components_internal.hpp"
#include "rtxui/core/string.hpp"
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
  // Row 0: content with padding:  " hello   " (padding-left=1, then text, spaces)
  // Row 1: blank row (fixed height, content shorter than 5 rows)
  // Row 2: blank row
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

TEST_CASE("Layout: Flexbox Grow Cumulative Distribution", "[layout][flex][grow]") {
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
    if (c == 'R') count_r++;
    else if (c == 'G') count_g++;
    else if (c == 'B') count_b++;
  }

  // Under cumulative allocation, the total width must be exactly 40.
  CHECK(count_r + count_g + count_b == 40);
}

TEST_CASE("Layout: Text node in flexbox row regression", "[layout][flex][regression]") {
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

TEST_CASE("Layout: Flexbox Shrink Cumulative Distribution", "[layout][flex][shrink]") {
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
    if (c == 'R') count_r++;
    else if (c == 'G') count_g++;
    else if (c == 'B') count_b++;
  }

  // Under cumulative allocation, total width of flex row must shrink from 60 (20+20+20) to exactly 40.
  CHECK(count_r + count_g + count_b == 40);
}

TEST_CASE("Layout: Flexbox grow/shrink remainder allocated to final child", "[layout][flex][remainder]") {
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
    // The final child must receive the remainder, making the sizes exactly 3, 3, 4.
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

}  // namespace rtxui

