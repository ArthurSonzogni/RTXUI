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

RTXUI_COMPONENT(Styled) {
  Import<rtxui::div>();
  return R"html(
    <div>Begin</div>
    <slot></slot>
    <div>End</div>

    <style>
      self {
        display: inline block;
        color: white;
        background-color: black;
        border: 1;
        width: 30%;
      }
      div {
        color: yellow;
        background-color: blue;
        padding: 1;
        border: 1;
        margin: 1;
      }
    </style>
  )html";
}

RTXUI_COMPONENT(App) {
  Import<rtxui::div>();
  Import<Styled>();
  return R"html(
    Lorem ipsum dolor sit amet, consectetur adipiscing elit - Bonjour!
    Bonjour ipsum dolor sit amet, consectetur adipiscing elit.
    Au revoir ipsum dolor sit amet, consectetur adipiscing elit.


    <Styled>
      This is a slot content inside the Styled component.
    </Styled>

    <Styled id="large-styled">
      This is a slot content inside the Styled component.
      This is a slot content inside the Styled component.
      This is a slot content inside the Styled component.
      This is a slot content inside the Styled component.
      This is a slot content inside the Styled component.
    </Styled>

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
        background-color: black;
        width: 100%;
        border: 1;
      }

      #large-styled {
        background-color: red;
      }
     
      #flex {
        display: flex;
        width: 100%;
        gap: 1;
        padding: 1;
        border: 1;
        margin: 1;
        background-color: yellow;
        color: black;
      }
      #red {
        width: 10;
        background-color: red;
        color: white;
        flex-grow: 1;
        padding: 1;
        border: 1;
      }
      #green {
        background-color: green;
        color: white;
        flex-grow: 2;
        padding: 1;
        border: 1;
      }
      #blue {
        background-color: blue;
        color: white;
        flex-grow: 1;
        padding: 1;
        border: 1;
      }
    </style>
  )html";
}

int main() {
  auto app = Ref<App>::New();
  app->Mount();
  auto root = app->Root();

  // Print the root DOM element
  std::cout << "[Info] Root DOM Element:" << std::endl;
  std::cout << root->Print() << std::endl;

  // 2. Build Box Tree
  std::cout << "[Step 1] Constructing Layout Tree..." << std::endl;
  auto root_box = LayoutTreeBuilder::Build(root);

  // Print the root Layout Box Tree
  std::cout << "[Info] Root Layout Box Tree:" << std::endl;
  std::cout << root_box->Print() << std::endl;

  for (int width = 10; width <= 60; width += 5) {
    // 3. Layout
    std::cout << "[Step 2] Running Layout Algorithms..." << std::endl;
    LayoutConstraints viewport = {
        {width, MeasureMode::Exactly},
        {30, MeasureMode::Exactly},
    };
    auto root_fragment = RunLayout({root_box.get()}, viewport);

    // 4. Paint
    // Initialize with space characters
    Texture texture(width, 30);
    Paint(root_fragment.get(), texture);

    std::cout << texture.Render() << std::endl;
  }

  return 0;
}
