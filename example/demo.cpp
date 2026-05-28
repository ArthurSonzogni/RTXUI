#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <rtxui/rtxui.hpp>


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
        <LabeledBox title="Box A (Scrollable List)">
          <div class="scroll-container">
            <div>[Item 1] Click here to focus & scroll</div>
            <div>[Item 2] Use Arrow keys / Page keys</div>
            <div>[Item 3] Mouse wheel scrolls up/down</div>
            <div>[Item 4] Scrollbar reduces width</div>
            <div>[Item 5] Configurable scroll speed</div>
            <div>[Item 6] Live viewport clipping</div>
            <div>[Item 7] Responsive layout block</div>
            <div>[Item 8] Overflow-y: scroll in CSS</div>
            <div>[Item 9] Scrollbar-width: auto</div>
            <div>[Item 10] Track styled with shade</div>
            <div>[Item 11] Thumb styled with solid</div>
            <div>[Item 12] ArrowUp / ArrowDown works</div>
            <div>[Item 13] PageUp / PageDown works</div>
            <div>[Item 14] End of scrolling demo</div>
          </div>
        </LabeledBox>
        <LabeledBox title="Box B (Nested Scroll Demo)">
          <div class="outer-scroll">
            <div class="scroll-title">[Outer Start] Scroll down to find inner boxes</div>
            <div class="inner-scroll">
              <div>[Inner A-1] Scroll me</div>
              <div>[Inner A-2] Use mouse wheel</div>
              <div>[Inner A-3] Or keyboard focus</div>
              <div>[Inner A-4] Inner A bottom</div>
            </div>
            <div class="scroll-title">Some text in between</div>
            <div class="inner-scroll">
              <div>[Inner B-1] Scroll me</div>
              <div>[Inner B-2] Scroll down</div>
              <div>[Inner B-3] Close to end</div>
              <div>[Inner B-4] Inner B bottom</div>
            </div>
            <div class="scroll-title">[Outer End] End of outer list</div>
          </div>
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
        .scroll-container {
          display: block;
          height: 6;
          overflow-y: scroll;
          scroll-speed: 1;
          border: ascii;
          border-color: yellow;
        }
        .outer-scroll {
          display: block;
          height: 6;
          overflow-y: scroll;
          scroll-speed: 1;
          border: ascii;
          border-color: green;
        }
        .inner-scroll {
          display: block;
          height: 3;
          overflow-y: scroll;
          scroll-speed: 1;
          border: dashed;
          border-color: cyan;
          margin-top: 1;
          margin-bottom: 1;
        }
        .scroll-title {
          color: white;
          font-weight: bold;
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
