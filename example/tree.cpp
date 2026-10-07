// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// A tree view.
//
// Each <tree-item> is a focusable row with a `label`, and the <tree-item>s
// nested inside it are its children, shown indented while it is `open`.
// Clicking a branch, or pressing Enter on it, toggles it; Right opens and Left
// closes the focused branch. A leaf's own `onclick` runs when it is picked.
//
// Try it: Tab into the tree, then use the arrows and Enter.
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class TreeDemo : public Component<TreeDemo> {
 public:
  bool src_open = true;
  std::string picked = "nothing";

  void Pick(std::string file) { picked = std::move(file); }

  TreeDemo() {
    Bind(src_open);
    Bind(picked);
    Bind(Pick);
  }

  std::string_view view = R"html(
      <div class="card">
        <h1>Project</h1>
        <div class="tree">
          <tree-item label="src" open="{src_open}">
            <tree-item label="main.cpp" onclick="Pick(src/main.cpp)"/>
            <tree-item label="ui">
              <tree-item label="app.cpp" onclick="Pick(src/ui/app.cpp)"/>
              <tree-item label="app.hpp" onclick="Pick(src/ui/app.hpp)"/>
            </tree-item>
          </tree-item>
          <tree-item label="tests">
            <tree-item label="app_test.cpp" onclick="Pick(tests/app_test.cpp)"/>
          </tree-item>
          <tree-item label="README.md" onclick="Pick(README.md)"/>
        </div>
        <p>Picked: {picked}</p>
      </div>

      <style>
        self {
          display: flex;
          align-items: center;
          justify-content: center;
          width: 100%;
          height: 100%;
          background-color: rgb(13, 17, 23);
          color: rgb(230, 237, 243);
        }
        .card {
          border: tall;
          border-color: rgb(48, 54, 61);
          background-color: rgb(22, 27, 34);
          padding: 1 3;
          width: 40;
        }
        h1 {
          color: rgb(88, 166, 255);
          font-weight: bold;
        }
        .tree {
          margin: 1 0;
        }
        tree-item::part(arrow) {
          color: rgb(139, 148, 158);
        }
      </style>
    )html";
};

int main() {
  auto app = Ref<TreeDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
