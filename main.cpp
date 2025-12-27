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

RTXUI_COMPONENT(Fixed) {
  Import<rtxui::p>();
  return R"html(
    Fixed

    <style>
      self {
        background-color: yellow;
        border-width: 1;
        width: 10;
      }
    </style>
  )html";
}

RTXUI_COMPONENT(Grow) {
  Import<rtxui::p>();
  return R"html(
    Grow
    <style>
      self {
        background-color: green;
        padding: 1;
        flex-grow: 1;
      }
    </style>
  )html";
}

RTXUI_COMPONENT(Percent) {
  Import<rtxui::p>();
  return R"html(
    20%
    <style>
      self {
        background-color: blue;
        width: 20%;
        margin: 1;
      }
    </style>
  )html";
}

RTXUI_COMPONENT(FlexContainer) {
  Import<rtxui::div>();
  Import<Fixed>();
  Import<Grow>();
  Import<Percent>();
  return R"html(
    <Fixed />
    <Grow />
    <Percent />

    <style>
      self {
        display: flex;
        flex-direction: row;
        foreground-color: black;
        width: 100%;
        height: 10;
      }
    </style>
  )html";
}

RTXUI_COMPONENT(RedText) {
  Import<rtxui::span>();
  return R"html(
    This is an inline box demo.

    <style>
      self {
        background-color: red;
      }
    </style>
  )html";
}

RTXUI_COMPONENT(GreenText) {
  Import<rtxui::span>();
  return R"html(
    It should wrap properly within

    <style>
      self {
        background-color: green;
      }
    </style>
  )html";
}

RTXUI_COMPONENT(BlueText) {
  Import<rtxui::span>();
  return R"html(
    the given width constraints.
    <style>
      self {
        background-color: blue;
      }
    </style>
  )html";
}

RTXUI_COMPONENT(App) {
  Import<rtxui::div>();
  Import<FlexContainer>();
  Import<RedText>();
  Import<GreenText>();
  Import<BlueText>();
  Import<rtxui::span>();
  return R"html(
    <FlexContainer />
    <RedText />
    <GreenText />
    <BlueText />

    <style>
      self {
        display: block;
        border-width: 1;
        foreground-color: white;
        background-color: black;
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

  for (int width = 10; width <= 60; width += 10) {
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
