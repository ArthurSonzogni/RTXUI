#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "src/component/component.hpp"
#include "src/component/default_components.hpp"
#include "src/core/refcounted.hpp"
#include "src/dom/element.hpp"
#include "src/dom/text_element.hpp"
#include "src/layout/layout.hpp"
#include "src/layout/layout_tree_builder.hpp"
#include "src/layout/style.hpp"
#include "src/paint/color.hpp"
#include "src/paint/paint.hpp"
#include "src/paint/texture.hpp"

using namespace rtxui;

class Styled : public Component<Styled> {
 public:
  std::string_view Setup() override {
    Import<rtxui::div>();
    return R"html(
      <div>Begin</div>
      <slot></slot>
      <div class="end">End</div>

      <style>
        self {
          display: inline block;
          color: white;
          background-color: rgb(70, 70, 70);
          border: tall;
          border-color: black;
          width: 50%;
          padding-left: 1;
        }
        div {
          color: yellow;
          background-color: rgb(120, 120, 120);
          border: tall;
          padding-left: 1;
          border-color: rgb(100, 100, 100);
          border-color-top: rgb(200, 200, 200);
          border-color-bottom: rgb(50, 50, 50);
        }
        .end {
          border-color: rgb(70,70,70);
          border-color-bottom: rgb(200, 200, 200);
          border-color-top: rgb(50, 50, 50);
        }
      </style>
    )html";
  }
};

class App : public Component<App> {
 public:
  // --- Transparent State ---
  RTXUI_STATE(int, count);

  // --- Actions ---
  void Increment() {
    count++;
    this->Digest();
  }

  std::string_view Setup() override {
    Import<rtxui::div>();
    Import<rtxui::h1>();
    Import<rtxui::button>();
    return R"html(
      <div class="header">
        <h1>RTXUI Reflection Demo</h1>
        <button onclick="Increment">Clicks: {count}</button>
      </div>

      <div id="flex">
        <div id="red">
          This is a red box.
        </div>
        <div id="green">
          This is a green box.
        </div>
        <div id="blue">
          This is a blue box.
        </div>
      </div>
      Au revoir!

      <style>
        self {
          color: white;
          background-color: rgb(30, 30, 30);
          width: 100%;
          border: tall;
        }
        .header {
          padding: 1;
          border-bottom: tall;
          border-color: gray;
        }
        #flex {
          display: flex;
          width: 100%;
          gap: 1;
          border: tall;
          margin: 1;
          background-color: rgb(50, 50, 50);
          color: black;
          border-color: black;
        }
        #red {
          width: 10;
          background-color: red;
          color: white;
          border-color: black;
          flex-grow: 1;
          padding-left: 1;
          border: tall;
        }
        #green {
          background-color: green;
          color: white;
          border-color: black;
          flex-grow: 2;
          padding-left: 1;
          border: tall;
        }
        #blue {
          background-color: blue;
          border-color: black;
          padding-left: 1;
          color: white;
          flex-grow: 1;
          border: tall;
        }
      </style>
    )html";
  }
};

int main() {
  auto app = Ref<App>::New();
  app->Mount();
  auto root = app->Root();

  // 2. Build Box Tree
  auto root_box = LayoutTreeBuilder::Build(root);

  for (int width = 80; width <= 80; width += 15) {
    // 3. Layout
    LayoutConstraints viewport = {
        {width, MeasureMode::Exactly},
        {30, MeasureMode::Exactly},
    };
    auto root_fragment = RunLayout({root_box.get()}, viewport);

    // 4. Paint
    Texture texture(width, 30);
    Paint(root_fragment.get(), texture);

    std::cout << texture.Render() << std::endl;
  }

  return 0;
}
