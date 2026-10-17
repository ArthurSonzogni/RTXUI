// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// An interactive flexbox playground.
//
// Every flex property is driven from the UI: change flex-direction,
// justify-content, align-items, and each item's grow/shrink/basis, and watch
// the boxes redistribute.
#include <rtxui/rtxui.hpp>
#include <string>
#include <string_view>
#include <vector>

using namespace rtxui;

class LayoutFlexDemo : public Component<LayoutFlexDemo> {
 public:
  std::string direction = "row";
  std::string wrap = "wrap";
  std::string justify = "flex-start";
  std::string align = "stretch";
  std::string gap = "0";
  std::string grow = "0";
  std::string shrink = "1";
  std::string basis = "auto";
  int width = 40;
  int height = 12;
  std::vector<std::string> boxes = {"one", "two", "three", "four"};

  void AddBox() {
    const char* names[] = {"one", "two",   "three", "four", "five",
                           "six", "seven", "eight", "nine", "ten"};
    boxes.push_back(names[boxes.size() % 10]);
  }
  void RemoveBox() {
    if (!boxes.empty()) {
      boxes.pop_back();
    }
  }

  std::string_view view = R"html(
      <div class="card">
        <h1>Flexbox playground</h1>
        <p>Each select sets one property: of the container, or of every box in it.</p>

        <div class="split">
          <div class="fields">
          <span>flex-direction</span>
          <select value="{direction}">
            <option value="row">row</option>
            <option value="row-reverse">row-reverse</option>
            <option value="column">column</option>
            <option value="column-reverse">column-reverse</option>
          </select>
          <span>flex-wrap</span>
          <select value="{wrap}">
            <option value="nowrap">nowrap</option>
            <option value="wrap">wrap</option>
            <option value="wrap-reverse">wrap-reverse</option>
          </select>
          <span>justify-content</span>
          <select value="{justify}">
            <option value="flex-start">flex-start</option>
            <option value="center">center</option>
            <option value="flex-end">flex-end</option>
            <option value="space-between">space-between</option>
            <option value="space-around">space-around</option>
            <option value="space-evenly">space-evenly</option>
          </select>
          <span>align-items</span>
          <select value="{align}">
            <option value="stretch">stretch</option>
            <option value="flex-start">flex-start</option>
            <option value="center">center</option>
            <option value="flex-end">flex-end</option>
          </select>
          <span>gap</span>
          <select value="{gap}">
            <option value="0">0</option>
            <option value="1">1</option>
            <option value="2">2</option>
            <option value="1 2">1 2</option>
          </select>
          <span>flex-grow</span>
          <select value="{grow}">
            <option value="0">0</option>
            <option value="1">1</option>
            <option value="2">2</option>
          </select>
          <span>flex-shrink</span>
          <select value="{shrink}">
            <option value="1">1</option>
            <option value="0">0</option>
            <option value="2">2</option>
          </select>
          <span>flex-basis</span>
          <select value="{basis}">
            <option value="auto">auto</option>
            <option value="0">0</option>
            <option value="10">10</option>
          </select>
          </div>

          <div class="preview">
            <div class="sizes">
              <span>width</span>
              <slider value="{width}" min="10" max="40" width="20" />
              <span>height</span>
              <slider value="{height}" min="3" max="16" width="20" />
            </div>
            <div class="container">
              <for each="{boxes}" as="box">
                <div class="box">{box}</div>
              </for>
            </div>
            <div class="buttons">
              <button onclick="AddBox">Add a box</button>
              <button onclick="RemoveBox">Remove one</button>
            </div>
          </div>
        </div>
      </div>

      <style>
        self {
          --border: rgb(48, 54, 61);
          --muted: rgb(139, 148, 158);
          --accent: rgb(88, 166, 255);

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
          border-color: var(--border);
          background-color: rgb(22, 27, 34);
          padding: 1 3;
          width: 86;
        }
        h1 {
          color: var(--accent);
        }
        p {
          color: var(--muted);
        }
        .split {
          display: flex;
          gap: 3;
        }
        .fields {
          display: grid;
          grid-template-columns: 17 16;
          row-gap: 1;
          flex-shrink: 0;
        }
        .fields span, .sizes span {
          color: var(--muted);
        }
        select:focus, slider:focus {
          color: var(--accent);
        }
        .sizes {
          display: grid;
          grid-template-columns: 8 auto;
        }
        .container {
          display: flex;
          flex-direction: {direction};
          flex-wrap: {wrap};
          justify-content: {justify};
          align-items: {align};
          gap: {gap};
          width: {width};
          height: {height};
          margin: 1 0;
          overflow: hidden;
          border: round;
          border-color: var(--border);
        }
        .box {
          flex-grow: {grow};
          flex-shrink: {shrink};
          flex-basis: {basis};
          border: tall;
          border-color: var(--accent);
          background-color: rgb(22, 45, 77);
          padding: 0 1;
        }
        .buttons {
          display: flex;
          gap: 1;
        }
        button {
          border: tall;
          border-color: var(--border);
          background-color: rgb(13, 17, 23);
          padding: 0 1;
        }
        button:hover, button:focus {
          border-color: var(--accent);
          color: var(--accent);
        }
      </style>
    )html";

  LayoutFlexDemo() {
    Bind(direction);
    Bind(wrap);
    Bind(justify);
    Bind(align);
    Bind(gap);
    Bind(grow);
    Bind(shrink);
    Bind(basis);
    Bind(width);
    Bind(height);
    Bind(boxes);
    Bind(AddBox);
    Bind(RemoveBox);
  }
};

int main() {
  auto app = Ref<LayoutFlexDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
