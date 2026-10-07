// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/tree_item/tree_item.hpp"

#include <catch2/catch_test_macros.hpp>
#include <string>

#include "rtxui/component.hpp"
#include "rtxui/headless.hpp"

namespace rtxui {
namespace {

class Files : public Component<Files> {
 public:
  bool src_open = false;
  std::string picked;
  void Pick(std::string name) { picked = std::move(name); }
  Files() {
    Bind(src_open);
    Bind(picked);
    Bind(Pick);
  }
  std::string_view view = R"html(
    <div>
      <tree-item label="src" open="{src_open}">
        <tree-item label="main.cpp" onclick="Pick(main.cpp)"/>
        <tree-item label="util">
          <tree-item label="a.cpp"/>
        </tree-item>
      </tree-item>
      <tree-item label="README.md" onclick="Pick(README.md)"/>
    </div>
  )html";
};

TEST_CASE("A tree shows its open branches", "[component][tree]") {
  auto app = Ref<Files>::New();
  HeadlessScreen screen(app, 20, 5);
  CHECK(screen.Text() == "▸ src\n  README.md\n\n\n\n");

  app->src_open = true;
  screen.Input("");
  CHECK(screen.Text() == "▾ src\n    main.cpp\n  ▸ util\n  README.md\n\n");
}

TEST_CASE("Keys open and close the focused branch", "[component][tree]") {
  auto app = Ref<Files>::New();
  HeadlessScreen screen(app, 20, 5);
  screen.Input("\t");    // Focus src.
  screen.Input("\r");    // Enter toggles it open...
  CHECK(app->src_open);  // ...and the bound flag follows.
  CHECK(screen.Text() == "▾ src\n    main.cpp\n  ▸ util\n  README.md\n\n");

  screen.Input("\x1b[D");  // Left closes it.
  CHECK_FALSE(app->src_open);
  CHECK(screen.Text() == "▸ src\n  README.md\n\n\n\n");

  screen.Input("\x1b[C");  // Right opens it again.
  CHECK(app->src_open);
}

TEST_CASE("Clicking a branch toggles it; a leaf runs its own onclick",
          "[component][tree]") {
  auto app = Ref<Files>::New();
  HeadlessScreen screen(app, 20, 5);
  screen.Click(3, 0);
  CHECK(app->src_open);
  CHECK(screen.Text() == "▾ src\n    main.cpp\n  ▸ util\n  README.md\n\n");

  // A nested branch, by its arrow.
  screen.Click(2, 2);
  CHECK(screen.Text() ==
        "▾ src\n    main.cpp\n  ▾ util\n      a.cpp\n  README.md\n");

  screen.Click(5, 1);
  CHECK(app->picked == "main.cpp");
  // Picking re-rendered the application; the open branches stay open.
  CHECK(screen.Text() ==
        "▾ src\n    main.cpp\n  ▾ util\n      a.cpp\n  README.md\n");
  screen.Click(5, 4);
  CHECK(app->picked == "README.md");
}

}  // namespace
}  // namespace rtxui
