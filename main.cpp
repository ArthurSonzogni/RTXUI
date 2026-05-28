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
#include "src/terminal/screen.hpp"

using namespace rtxui;

class LabeledBox : public Component<LabeledBox> {
 public:
  struct Props {
    std::string title = "Box";
  } props;

  LabeledBox() {
    Bind(props.title);
  }

  std::string_view Setup() override {
    Import<rtxui::div>();
    return R"html(
      <div class="box">
        <div class="title">{title}</div>
        <div class="content">
          <slot></slot>
        </div>
      </div>

      <style>
        self {
          display: block;
          flex-grow: 1;
        }
        .box {
          border: tall;
          border-color: rgb(150, 150, 150);
          padding-left: 1;
          display: block;
        }
        .title {
          font-weight: bold;
          color: yellow;
          margin-bottom: 1;
        }
        .content {
          display: block;
        }
      </style>
    )html";
  }
};

class App : public Component<App> {
 public:
  // --- Transparent State ---
  int count = 0;

  App() {
    Bind(count);
    BindComputed(double_clicks);
    Import("Increment", [this]() { Increment(); });
    Import("Decrement", [this]() { Decrement(); });
  }

  // --- Actions ---
  void Increment() {
    count++;
  }

  void Decrement() {
    count--;
  }

  // --- Computed ---
  int double_clicks() const {
    return count * 2;
  }

  bool OnEvent(Event event) override {
    if (event == Event::a() || event == Event::Keyboard::From(' ')) {
      Increment();
      return true;
    }
    return Component<App>::OnEvent(event);
  }

  std::string_view Setup() override {
    Import<rtxui::div>();
    Import<rtxui::h1>();
    Import<rtxui::button>();
    Import<LabeledBox>();
    return R"html(
      <div class="header">
        <h1>RTXUI Reflection Demo</h1>
        <button
          onclick="Increment"
          oncontextmenu="Decrement"
        >
          Clicks: {count}
        </button>

        ({double_clicks} doubled)
      </div>

      <div id="flex">
        <LabeledBox title="Box A (Clicks: {count})">
          This is a custom box.
        </LabeledBox>
        <LabeledBox title="Box B (Clicks: {double_clicks})">
          This is a box with doubled clicks.
        </LabeledBox>
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
          color: white;
          border-color: black;
        }
      </style>
    )html";
  }
};

int main() {
  auto app = Ref<App>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
