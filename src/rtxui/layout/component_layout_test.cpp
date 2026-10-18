// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "rtxui/color.hpp"
#include "rtxui/component.hpp"
#include "rtxui/component/component_internal.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/layout/layout.hpp"
#include "rtxui/layout/layout_tree_builder.hpp"
#include "rtxui/layout/style.hpp"
#include "rtxui/paint/texture.hpp"
#include "rtxui/refcounted.hpp"
#include "rtxui/testing/render.hpp"

using namespace rtxui;

namespace {

class FlexMinWidthComponent : public rtxui::Component<FlexMinWidthComponent> {
 public:
  // The container is narrower than the two items together, so the flex
  // distribution asks each to shrink to 4 -- and min-width refuses, leaving
  // each 5 wide.
  std::string_view view = R"(
    <div class="f"><div id="a">AAAAAAAA</div><div id="b">BBBBBBBB</div></div>
    <style>
      .f { display: flex; width: 8; }
      #a, #b { width: 6; min-width: 5; }
    </style>
  )";
};

TEST_CASE("A flex item that refuses to shrink is not overlapped",
          "[component][flex][layout]") {
  // The container positioned items by the size it asked for rather than the
  // size they took, so an item held at its min-width had the next one drawn
  // on top of its last cell.
  auto container = rtxui::Ref<FlexMinWidthComponent>::New();
  auto texture = RenderComponent(container, 16, 4);

  auto* first =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("#a");
  auto* second =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("#b");
  REQUIRE(first != nullptr);
  REQUIRE(second != nullptr);

  SECTION("min-width holds against the shrink") {
    CHECK(first->layout_width() == 5);
    CHECK(second->layout_width() == 5);
  }

  SECTION("and what is drawn matches those widths") {
    // The failure was silent in the layout tree and visible only on screen:
    // the widths were already right, and the second item was simply placed a
    // cell too early.
    std::string row;
    for (int x = 0; x < 10; ++x) {
      row += texture[x, 0].character;
    }
    CHECK(row == "AAAAABBBBB");
  }
}

class BlockInInlineComponent : public rtxui::Component<BlockInInlineComponent> {
 public:
  // A component's own root is inline-level, so these sit in an inline
  // formatting context -- which is the situation the widths expose. Each is
  // narrow enough that two would fit side by side if nothing broke the line.
  std::string_view view = R"(
    <div class="ib">AA</div><div class="ib">BB</div>
    <div class="blk">CC</div><div class="blk">DD</div>
    <span class="pad">EE</span><span class="pad">FF</span>
    <style>
      .ib { display: inline-block; width: 4; }
      .blk { display: block; width: 4; }
      .pad { padding-left: 1; }
    </style>
  )";
};

TEST_CASE("A block in an inline context takes its own line",
          "[component][layout]") {
  // Block-level children were placed like atomic inline boxes, so two narrow
  // ones shared a line. An auto-width block hid it: one fills the line and
  // pushes the next down anyway, so it only showed once a block had a width
  // small enough for two to fit.
  auto container = rtxui::Ref<BlockInInlineComponent>::New();
  auto texture = RenderComponent(container, 30, 10);

  auto row_text = [&](int y) {
    std::string row;
    for (int x = 0; x < texture.width(); ++x) {
      row += texture[x, y].character;
    }
    while (!row.empty() && row.back() == ' ') {
      row.pop_back();
    }
    while (!row.empty() && row.front() == ' ') {
      row.erase(row.begin());
    }
    return row;
  };
  auto row_of = [&](std::string_view needle) {
    for (int y = 0; y < texture.height(); ++y) {
      if (row_text(y).find(needle) != std::string::npos) {
        return y;
      }
    }
    return -1;
  };

  SECTION("two block children never share a line") {
    const int cc = row_of("CC");
    const int dd = row_of("DD");
    REQUIRE(cc != -1);
    REQUIRE(dd != -1);
    CHECK(cc != dd);
  }

  SECTION("inline-block children still do share one") {
    CHECK(row_of("AA") == row_of("BB"));
    CHECK(row_of("AA") != -1);
  }

  SECTION("inline children with padding still share one") {
    CHECK(row_of("EE") == row_of("FF"));
    CHECK(row_of("EE") != -1);
  }

  SECTION("a block breaks the line before it and after it") {
    // The inline-blocks above it and the spans below it are on neither of the
    // blocks' lines.
    CHECK(row_of("AA") < row_of("CC"));
    CHECK(row_of("DD") < row_of("EE"));
  }
}

class TrailingBlockComponent : public rtxui::Component<TrailingBlockComponent> {
 public:
  // The span, a flex item, lays its content out inline, and that content ends
  // with a block. The line the block leaves open after it holds nothing, the
  // spaces before </span> collapsing away: it used to take a row of its own,
  // a blank one before "after". A component's element, inline by default,
  // holding a block did the same: <slider> in a flex row.
  std::string_view view = R"(
    <div class="row">
      <span><div>block</div>
      </span>
    </div>
    <div>after</div>
    <style>
      .row { display: flex; }
    </style>
  )";
};

TEST_CASE("An inline box ending with a block takes no line after it",
          "[component][layout]") {
  auto container = rtxui::Ref<TrailingBlockComponent>::New();
  auto texture = RenderComponent(container, 10, 4);
  auto row = [&](int y) {
    std::string text;
    for (int x = 0; x < texture.width(); ++x) {
      text += texture[x, y].character.empty() ? " " : texture[x, y].character;
    }
    return text.substr(0, text.find_last_not_of(' ') + 1);
  };
  CHECK(row(0) == "block");
  CHECK(row(1) == "after");
}

class BlockInInlineFlexItemComponent
    : public rtxui::Component<BlockInInlineFlexItemComponent> {
 public:
  // The <span> holding a <div> is what a component's root holding its
  // template is: an inline box around a block. As an item of the column, it
  // made the column claim the whole row.
  std::string_view view = R"(
    <div class="row">
      <div id="side">SIDE</div>
      <div class="column"><p>main</p><span><div>inner</div></span></div>
    </div>
    <style>
      .row { display: flex; }
      #side { width: 12; }
      .column { flex-grow: 1; display: flex; flex-direction: column; }
    </style>
  )";
};

TEST_CASE("A block in an inline box does not widen its flex container",
          "[component][flex][layout]") {
  // Any component placed in a flex column collapsed the sidebar beside it to
  // one cell: the inline box around the component's block was measured as
  // wide as all the space there was.
  auto container = rtxui::Ref<BlockInInlineFlexItemComponent>::New();
  RenderComponent(container, 40, 3);
  auto* side =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("#side");
  REQUIRE(side != nullptr);
  CHECK(side->layout_width() == 12);
}

class NarrowBlocksComponent : public rtxui::Component<NarrowBlocksComponent> {
 public:
  std::string_view view = R"(
    <div id="first">one</div>
    <div id="second">two</div>
    <style>
      div { width: 5; }
    </style>
  )";
};

class NarrowBlocksPageComponent
    : public rtxui::Component<NarrowBlocksPageComponent> {
 public:
  NarrowBlocksPageComponent() { Import<NarrowBlocksComponent>(); }
  std::string_view view = R"(<div><NarrowBlocksComponent /></div>)";
};

TEST_CASE("Blocks in a component stack, however narrow",
          "[component][layout]") {
  // The component's root is an inline box around its template. Blocks in it
  // sat side by side whenever they were narrow enough to fit on one line.
  auto container = rtxui::Ref<NarrowBlocksPageComponent>::New();
  RenderComponent(container, 40, 4);
  auto* root = rtxui::ComponentInternals::Root(*container);
  auto* first = root->QuerySelector("#first");
  auto* second = root->QuerySelector("#second");
  REQUIRE(first != nullptr);
  REQUIRE(second != nullptr);
  CHECK(first->absolute_x() == 0);
  CHECK(first->absolute_y() == 0);
  CHECK(second->absolute_x() == 0);
  CHECK(second->absolute_y() == 1);
}

class WrappedFixedComponent : public rtxui::Component<WrappedFixedComponent> {
 public:
  // The inline-block does not fit after the text, so it wraps to the next
  // line, carrying a fixed box that belongs at the top-left of the screen.
  std::string_view view = R"(
    <div class="line">some text <div class="chip"><div id="fixed"></div>x</div></div>
    <style>
      .line { width: 20; }
      .chip { display: inline-block; width: 15; }
      #fixed { position: fixed; top: 0; left: 0; width: 3; height: 1; }
    </style>
  )";
};

TEST_CASE("A box that wraps lays out its fixed descendants where it lands",
          "[component][layout]") {
  auto container = rtxui::Ref<WrappedFixedComponent>::New();
  RenderComponent(container, 40, 4);
  auto* fixed =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("#fixed");
  REQUIRE(fixed != nullptr);
  CHECK(fixed->absolute_x() == 0);
  CHECK(fixed->absolute_y() == 0);
}

class ExtremeSizeComponent : public rtxui::Component<ExtremeSizeComponent> {
 public:
  // calc() bounds each operand when it is parsed, but not their product, so
  // this reaches layout as a float far too large for the int it is narrowed
  // to. That narrowing is undefined behaviour and produced INT_MIN, which then
  // poisoned every subtraction downstream.
  std::string_view view = R"(
    <div id="big">wide</div>
    <div id="tall">tall</div>
    <div id="neg">neg</div>
    <style>
      #big { width: calc(2000000000 * 2000000000); }
      #tall { height: 2000000000; padding: 1000000000; }
      #neg { margin: -2000000000; }
    </style>
  )";
};

TEST_CASE("Extreme sizes lay out without overflowing", "[component][limits]") {
  // Reaching layout is the point: ResolveSize does its own narrowing, on a
  // path Length::Resolve never sees, so a style-level test would not cover it.
  // Under the sanitizer build this also fails outright rather than merely
  // measuring wrong, which is what makes it a guard rather than a check of
  // one number.
  auto container = rtxui::Ref<ExtremeSizeComponent>::New();
  auto texture = RenderComponent(container, 40, 12);

  // Whatever the engine settles on, it must be a size rather than a wrapped
  // one: the failure this guards against produced INT_MIN.
  auto* big =
      rtxui::ComponentInternals::Root(*container)->QuerySelector("#big");
  REQUIRE(big != nullptr);
  CHECK(big->layout_width() >= 0);
  CHECK(big->layout_width() <= 1000000);
}

class PositionedTextComponent
    : public rtxui::Component<PositionedTextComponent> {
 public:
  // `:not(.x)` reaches the text node too.
  std::string_view view = R"(
    <div class="box">text</div>
    <style>
      .box { position: relative; }
      :not(.x) { position: absolute; }
    </style>
  )";
};

TEST_CASE("Text a rule positions out of flow stays in flow",
          "[component][layout][position]") {
  // Regression (layout fuzzer): an absolutely positioned text box was laid
  // out as its own out-of-flow child, recursing until the stack ran out.
  auto container = rtxui::Ref<PositionedTextComponent>::New();
  auto texture = RenderComponent(container, 10, 2);
  CHECK(texture.Render().find("text") != std::string::npos);
}

class MarginAutoTestComponent
    : public rtxui::Component<MarginAutoTestComponent> {
 public:
  std::string_view view = R"(
    <box id="parent">
      <box id="child">centered</box>
    </box>
    <style>
      #parent {
        display: block;
        width: 80;
      }
      #child {
        display: block;
        max-width: 40;
        margin: 0 auto;
      }
    </style>
  )";
};

TEST_CASE("Block layout max-width and margin auto centering",
          "[component][layout][margin][max-width]") {
  auto container = rtxui::Ref<MarginAutoTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto root_box = rtxui::LayoutTreeBuilder::Build(
      rtxui::ComponentInternals::Root(*container));
  REQUIRE(root_box != nullptr);

  rtxui::LayoutConstraints viewport = {
      {80, rtxui::MeasureMode::Exactly},
      {24, rtxui::MeasureMode::Exactly},
  };
  auto root_fragment = rtxui::RunLayout({root_box.get()}, viewport);
  REQUIRE(root_fragment != nullptr);

  REQUIRE(root_fragment->children.size() == 1);
  auto parent_frag = root_fragment->children[0].fragment;
  REQUIRE(parent_frag->children.size() == 1);
  auto child_link = parent_frag->children[0];
  CHECK(child_link.fragment->width == 40);
  CHECK(child_link.x == 20);
}

class MaxHeightTestComponent : public rtxui::Component<MaxHeightTestComponent> {
 public:
  std::string_view view = R"(
    <box id="parent">
      <box id="child">
        <box>1</box>
        <box>2</box>
        <box>3</box>
        <box>4</box>
        <box>5</box>
      </box>
    </box>
    <style>
      box {
        display: block;
      }
      #parent {
        height: 20;
      }
      #child {
        max-height: 3;
      }
    </style>
  )";
};

TEST_CASE("Block layout max-height constraint",
          "[component][layout][max-height]") {
  auto container = rtxui::Ref<MaxHeightTestComponent>::New();
  rtxui::ComponentInternals::Mount(*container);

  auto root_box = rtxui::LayoutTreeBuilder::Build(
      rtxui::ComponentInternals::Root(*container));
  REQUIRE(root_box != nullptr);

  rtxui::LayoutConstraints viewport = {
      {80, rtxui::MeasureMode::Exactly},
      {24, rtxui::MeasureMode::Exactly},
  };
  auto root_fragment = rtxui::RunLayout({root_box.get()}, viewport);
  REQUIRE(root_fragment != nullptr);

  REQUIRE(root_fragment->children.size() == 1);
  auto parent_frag = root_fragment->children[0].fragment;
  REQUIRE(parent_frag->children.size() == 1);
  auto child_link = parent_frag->children[0];
  CHECK(child_link.fragment->height == 3);
}

class BlockLevelFlexComponent
    : public rtxui::Component<BlockLevelFlexComponent> {
 public:
  std::string_view view = R"(
    <div class="card">
      <div id="row"><span>a</span><span id="b">b</span></div>
      <div id="sized"><span>a</span></div>
    </div>
    <style>
      .card { width: 20; }
      #row { display: flex; justify-content: space-between; }
      #sized { display: flex; width: 6; }
    </style>
  )";
};

class NegativeMarginComponent
    : public rtxui::Component<NegativeMarginComponent> {
 public:
  std::string_view view = R"(
    <div>
      <div id="a">a</div>
      <div id="b">b</div>
      <div id="c">c</div>
      <div id="d">d</div>
    </div>
    <style>
      #a { margin-bottom: -1; }
      #b { margin-bottom: 2; }
      #c { margin-bottom: -1; }
    </style>
  )";
};

struct InlineSpanBackgroundTestComponent
    : public rtxui::Component<InlineSpanBackgroundTestComponent> {
  std::string_view view = R"html(
    <div>before <span class="hl">highlighted</span> after</div>
    <style>
      .hl { background-color: rgb(38, 79, 120); }
    </style>
  )html";
};

}  // namespace

TEST_CASE("Inline span background-color is applied when painted",
          "[component][layout][regression]") {
  // Regression: LayoutInlineFlow's optimization for inline elements with no
  // border/padding/margin (e.g. a plain <span>) flattens them and processes
  // their text node directly, but read the *text node's* style for
  // background-color instead of the *span's*. background-color doesn't
  // inherit, so the text node never actually carried it -- the span's own
  // background-color was silently dropped at paint time (while still
  // resolving correctly in base_style, since that part of the pipeline was
  // unaffected). This is what made textarea/input's .selection highlight
  // invisible.
  auto container = rtxui::Ref<InlineSpanBackgroundTestComponent>::New();
  auto texture = RenderComponent(container, 40, 3);

  std::string rendered = texture.Render();
  auto pos = rendered.find("highlighted");
  REQUIRE(pos != std::string::npos);

  // Find which (x, y) cell that character lands on by re-deriving it from
  // the element's own layout position, since `rendered` embeds ANSI codes.
  auto* span =
      rtxui::ComponentInternals::Root(*container)->QuerySelector(".hl");
  REQUIRE(span != nullptr);
  int x = span->absolute_x();
  int y = span->absolute_y();
  CHECK(texture[x, y].background_color == Color::RGB(38, 79, 120));
}

TEST_CASE("A block-level flex container fills its block parent",
          "[component][flex][layout][regression]") {
  // Block flow offered its flex children an AtMost width, which LayoutFlex
  // read as shrink-to-fit: the row took the width of its content, so
  // `justify-content: space-between` had no space to distribute.
  auto container = rtxui::Ref<BlockLevelFlexComponent>::New();
  RenderComponent(container, 30, 4);
  auto* root = rtxui::ComponentInternals::Root(*container);

  CHECK(root->QuerySelector("#row")->layout_width() == 20);
  CHECK(root->QuerySelector("#b")->absolute_x() == 19);
  CHECK(root->QuerySelector("#sized")->layout_width() == 6);
}

TEST_CASE("Negative vertical margins pull the next block up",
          "[component][layout][regression]") {
  // Adjacent margins collapsed to the larger of the two, so a negative one
  // was dropped. <fieldset> relies on it to set its legend in the border.
  auto container = rtxui::Ref<NegativeMarginComponent>::New();
  RenderComponent(container, 10, 6);
  auto* root = rtxui::ComponentInternals::Root(*container);

  CHECK(root->QuerySelector("#a")->absolute_y() == 0);
  CHECK(root->QuerySelector("#b")->absolute_y() == 0);  // 1 - 1.
  CHECK(root->QuerySelector("#c")->absolute_y() == 3);  // 1 + 2.
  CHECK(root->QuerySelector("#d")->absolute_y() == 3);  // 4 - 1.
}
