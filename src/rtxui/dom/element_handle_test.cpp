// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <catch2/catch_test_macros.hpp>
#include <rtxui/rtxui.hpp>
#include <string_view>

namespace {

class HandleApp : public rtxui::Component<HandleApp> {
 public:
  std::string_view view = R"html(
    <div id="list" class="scroller" data-kind="log">
      <span>a</span>
      <span>b</span>
    </div>
    <style>
      .scroller { height: 1; overflow-y: scroll; }
    </style>
  )html";
};

TEST_CASE("ElementHandle navigates the tree", "[dom][element_handle]") {
  auto app = rtxui::Ref<HandleApp>::New();
  app->Mount();

  rtxui::ElementHandle root = app->RootElement();
  REQUIRE(root);

  rtxui::ElementHandle list = app->QueryElement("#list");
  REQUIRE(list);
  CHECK(list.tag() == "div");
  CHECK(list.GetAttribute("data-kind") == "log");
  CHECK_FALSE(list.GetAttribute("missing").has_value());
  REQUIRE(list.ChildCount() == 2);
  CHECK(list.ChildAt(0).tag() == "span");
  CHECK(list.ChildAt(0).Parent().tag() == "div");
  CHECK(list.QuerySelector("span"));
}

TEST_CASE("A null ElementHandle is inert", "[dom][element_handle]") {
  auto app = rtxui::Ref<HandleApp>::New();
  app->Mount();

  rtxui::ElementHandle none = app->QueryElement("#nothing");
  CHECK_FALSE(none);
  CHECK_FALSE(none.QuerySelector("span"));
  CHECK_FALSE(none.Parent());
  CHECK(none.ChildCount() == 0);
  CHECK_FALSE(none.ChildAt(0));
  CHECK(none.tag().empty());
  CHECK_FALSE(none.GetAttribute("id").has_value());
  CHECK(none.scroll_y() == 0);
  none.SetScrollY(3);  // Must not crash.

  rtxui::ElementHandle list = app->QueryElement("#list");
  CHECK_FALSE(list.ChildAt(99));
}

TEST_CASE("ElementHandle reads and writes scroll", "[dom][element_handle]") {
  auto app = rtxui::Ref<HandleApp>::New();
  app->Mount();

  rtxui::ElementHandle list = app->QueryElement("#list");
  REQUIRE(list);
  list.SetScrollY(1);
  CHECK(list.scroll_y() == 1);
  list.SetScrollY(-5);
  CHECK(list.scroll_y() == 0);
  list.SetScrollX(-5);
  CHECK(list.scroll_x() == 0);
}

class ToggleApp : public rtxui::Component<ToggleApp> {
 public:
  bool shown = true;
  int frame = 0;
  ToggleApp() {
    Bind(shown);
    Bind(frame);
  }
  std::string_view view = R"html(
    <span>{frame}</span>
    <if condition="{shown}">
      <div id="target">x</div>
    </if>
  )html";
};

TEST_CASE("ElementHandle becomes null once a re-render destroys its element",
          "[dom][element_handle]") {
  // The handle must not keep the element alive: an element points back at its
  // component without owning it, so outliving the component would leave tag()
  // reading freed memory.
  auto app = rtxui::Ref<ToggleApp>::New();
  app->Mount();

  rtxui::ElementHandle target = app->QueryElement("#target");
  REQUIRE(target);
  rtxui::ElementHandle copy = target;

  app->shown = false;
  app->Digest();
  CHECK_FALSE(app->QueryElement("#target"));
  // Removed components are pooled until the next render, so the element may
  // still exist -- but only detached, never reachable from the tree.
  CHECK_FALSE(target.Parent());

  app->frame++;
  app->Digest();
  CHECK_FALSE(target);
  CHECK_FALSE(copy);
  CHECK(target.tag().empty());
}

TEST_CASE("ElementHandle becomes null once its component is destroyed",
          "[dom][element_handle]") {
  rtxui::ElementHandle kept;
  {
    auto app = rtxui::Ref<HandleApp>::New();
    app->Mount();
    rtxui::ElementHandle found = app->QueryElement("#list");
    rtxui::ElementHandle moved = std::move(found);
    REQUIRE(moved);
    kept = moved;
  }
  CHECK_FALSE(kept);
  CHECK(kept.ChildCount() == 0);
}

}  // namespace
