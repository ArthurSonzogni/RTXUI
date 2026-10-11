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
#include "rtxui/component/default/div/div.hpp"
#include "rtxui/component/default/span/span.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/layout/layout.hpp"
#include "rtxui/layout/layout_tree_builder.hpp"
#include "rtxui/layout/style.hpp"
#include "rtxui/paint/paint.hpp"
#include "rtxui/paint/texture.hpp"
#include "rtxui/refcounted.hpp"

using namespace rtxui;

namespace {

class FlexMinWidthComponent : public rtxui::Component<FlexMinWidthComponent> {
 public:
  void InitReflection() override {
    Import<rtxui::div>();
    rtxui::Component<FlexMinWidthComponent>::InitReflection();
  }
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
  container->Mount();

  auto root_box = rtxui::LayoutTreeBuilder::Build(container->Root());
  REQUIRE(root_box != nullptr);
  rtxui::LayoutConstraints viewport = {
      {16, rtxui::MeasureMode::Exactly},
      {4, rtxui::MeasureMode::Exactly},
  };
  auto root_fragment = rtxui::RunLayout({root_box.get()}, viewport);
  REQUIRE(root_fragment != nullptr);
  Texture texture(16, 4);
  rtxui::Paint(root_fragment.get(), texture);

  auto* first = container->Root()->QuerySelector("#a");
  auto* second = container->Root()->QuerySelector("#b");
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
  void InitReflection() override {
    Import<rtxui::div>();
    Import<rtxui::span>();
    rtxui::Component<BlockInInlineComponent>::InitReflection();
  }
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
  container->Mount();

  auto root_box = rtxui::LayoutTreeBuilder::Build(container->Root());
  REQUIRE(root_box != nullptr);
  rtxui::LayoutConstraints viewport = {
      {30, rtxui::MeasureMode::Exactly},
      {10, rtxui::MeasureMode::Exactly},
  };
  auto root_fragment = rtxui::RunLayout({root_box.get()}, viewport);
  REQUIRE(root_fragment != nullptr);
  Texture texture(30, 10);
  rtxui::Paint(root_fragment.get(), texture);

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

class ExtremeSizeComponent : public rtxui::Component<ExtremeSizeComponent> {
 public:
  void InitReflection() override {
    Import<rtxui::div>();
    rtxui::Component<ExtremeSizeComponent>::InitReflection();
  }
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
  container->Mount();

  auto root_box = rtxui::LayoutTreeBuilder::Build(container->Root());
  REQUIRE(root_box != nullptr);
  rtxui::LayoutConstraints viewport = {
      {40, rtxui::MeasureMode::Exactly},
      {12, rtxui::MeasureMode::Exactly},
  };
  auto root_fragment = rtxui::RunLayout({root_box.get()}, viewport);
  REQUIRE(root_fragment != nullptr);
  Texture texture(40, 12);
  rtxui::Paint(root_fragment.get(), texture);

  // Whatever the engine settles on, it must be a size rather than a wrapped
  // one: the failure this guards against produced INT_MIN.
  auto* big = container->Root()->QuerySelector("#big");
  REQUIRE(big != nullptr);
  CHECK(big->layout_width() >= 0);
  CHECK(big->layout_width() <= 1000000);
}

class MarginAutoTestComponent
    : public rtxui::Component<MarginAutoTestComponent> {
 public:
  void InitReflection() override {
    rtxui::Component<MarginAutoTestComponent>::InitReflection();
  }
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
  container->Mount();

  auto root_box = rtxui::LayoutTreeBuilder::Build(container->Root());
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
  void InitReflection() override {
    rtxui::Component<MaxHeightTestComponent>::InitReflection();
  }
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
  container->Mount();

  auto root_box = rtxui::LayoutTreeBuilder::Build(container->Root());
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

struct InlineSpanBackgroundTestComponent
    : public rtxui::Component<InlineSpanBackgroundTestComponent> {
  void InitReflection() override {
    Import<rtxui::div>();
    Import<rtxui::span>();
    rtxui::Component<InlineSpanBackgroundTestComponent>::InitReflection();
  }
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
  container->Mount();

  auto root_box = rtxui::LayoutTreeBuilder::Build(container->Root());
  REQUIRE(root_box != nullptr);
  rtxui::LayoutConstraints viewport = {
      {40, rtxui::MeasureMode::Exactly},
      {3, rtxui::MeasureMode::Exactly},
  };
  auto root_fragment = rtxui::RunLayout({root_box.get()}, viewport);
  REQUIRE(root_fragment != nullptr);

  Texture texture(40, 3);
  rtxui::Paint(root_fragment.get(), texture);

  std::string rendered = texture.Render();
  auto pos = rendered.find("highlighted");
  REQUIRE(pos != std::string::npos);

  // Find which (x, y) cell that character lands on by re-deriving it from
  // the element's own layout position, since `rendered` embeds ANSI codes.
  auto* span = container->Root()->QuerySelector(".hl");
  REQUIRE(span != nullptr);
  int x = span->absolute_x();
  int y = span->absolute_y();
  CHECK(texture[x, y].background_color == Color::RGB(38, 79, 120));
}
