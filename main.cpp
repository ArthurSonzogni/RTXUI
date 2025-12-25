#include <iostream>
#include <memory>
#include <string>
#include <vector>

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

int main() {
  // ----------------------------
  // Flexbox Demo:
  // Row container with 3 items:
  // 1. Fixed width
  // 2. Grow (Takes remaining space)
  // 3. Percent Width
  // ----------------------------

  auto root = Ref<Element>::New();
  root->style.display = Display::Block;
  root->style.border = {1, 1, 1, 1};
  root->style.foreground_color = Color::RGB(255, 255, 255);

  auto flex_container = Ref<Element>::New();
  flex_container->style.display = Display::Flex;
  flex_container->style.flex_direction = Direction::Row;
  flex_container->style.width = Length::Pct(100);
  flex_container->style.height = Length::Cells(10);

  // Item 1: Fixed
  auto item1 = Ref<Element>::New();
  item1->style.display = Display::Block;
  item1->style.width = Length::Cells(10);
  item1->style.background_color = Color::RGB(255, 255, 0);
  auto text1 = Ref<TextElement>::New("Fixed");
  text1->style.foreground_color = Color::RGB(0, 0, 0);
  item1->AddChild(text1);

  // Item 2: Grow
  auto item2 = Ref<Element>::New();
  item2->style.display = Display::Block;
  item2->style.flex_grow = 1;
  item2->style.background_color = Color::RGB(0, 255, 0);
  item2->AddChild(Ref<TextElement>::New("Grow"));

  // Item 3: Percent
  auto item3 = Ref<Element>::New();
  item3->style.display = Display::Block;
  item3->style.width = Length::Pct(20);
  item3->style.background_color = Color::RGB(0, 0, 255);
  item3->AddChild(Ref<TextElement>::New("20%"));

  flex_container->AddChild(item1);
  flex_container->AddChild(item2);
  flex_container->AddChild(item3);

  root->AddChild(flex_container);

  auto text_1 = Ref<TextElement>::New("This is an inline box demo.");
  auto text_2 = Ref<TextElement>::New(" ");
  auto text_3 = Ref<TextElement>::New("It should wrap properly within");
  auto text_4 = Ref<TextElement>::New(" ");
  auto text_5 = Ref<TextElement>::New("the given width constraints.");
  text_1->style.background_color = Color::RGB(255, 0, 0);
  text_3->style.background_color = Color::RGB(0, 255, 0);
  text_5->style.background_color = Color::RGB(0, 0, 255);

  root->AddChild(text_1);
  root->AddChild(text_2);
  root->AddChild(text_3);
  root->AddChild(text_4);
  root->AddChild(text_5);

  // 2. Build Box Tree
  std::cout << "[Step 1] Constructing Layout Tree..." << std::endl;
  auto root_box = LayoutTreeBuilder::Build(root.get());

  for(int width = 10; width <= 60; width += 10) {
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
