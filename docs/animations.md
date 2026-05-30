# Animations & Hover

RTXUI supports CSS-style animations and dynamic interactive states (pseudo-classes) on DOM elements. This allows you to build terminals with smooth color transitions, scaling borders, and responsive hover/active effects without writing manual frame loops or blocking input polling.

## 1. Pseudo-Classes

RTXUI supports the following interactive pseudo-classes:
* `:hover`: Matched when the mouse pointer is over the element or its children.
* `:active`: Matched while the element is being clicked/pressed.
* `:focus`: Matched when the element has keyboard focus.

### Example Selector Usage
```css
.btn {
  background-color: rgb(30, 58, 138);
}
.btn:hover {
  background-color: rgb(29, 78, 216); /* Bright blue on hover */
}
.btn:active {
  background-color: rgb(30, 64, 175); /* Darker blue on click */
}
```

### Minimal Interactive Demo

Below is the C++ code and the live WebAssembly terminal illustrating `:hover`, `:active`, and `:focus` interactive pseudo-classes. This file is located in `example/pseudo_classes.cpp`.

```cpp
#include <iostream>
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class PseudoClassesDemo : public Component<PseudoClassesDemo> {
 public:
  std::string_view Setup() override {
    return R"html(
      <div class="container">
        <h3>Interactive Pseudo-Classes</h3>
        <p>Hover/Click/Focus the button below:</p>
        <div class="btn" tabindex="0">Interactive Button</div>
      </div>

      <style>
        self {
          display: block;
          padding: 1 2;
          background-color: rgb(15, 23, 42);
          color: rgb(241, 245, 249);
        }
        h3 {
          color: rgb(59, 130, 246);
          margin-bottom: 0;
        }
        p {
          color: rgb(148, 163, 184);
          margin-bottom: 1;
        }
        .btn {
          display: block;
          border: tall;
          border-color: rgb(30, 58, 138);
          background-color: rgb(17, 24, 39);
          color: rgb(191, 219, 254);
          padding: 1 3;
          text-align: center;
          width: 24;
        }
        .btn:hover {
          background-color: rgb(30, 58, 138);
          border-color: rgb(59, 130, 246);
          color: rgb(255, 255, 255);
        }
        .btn:active {
          background-color: rgb(29, 78, 216);
          border-color: rgb(96, 165, 250);
        }
        .btn:focus {
          border-color: rgb(147, 197, 253);
        }
      </style>
    )html";
  }
};

int main() {
  auto app = Ref<PseudoClassesDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
```

<WasmTerminal src="/wasm/rtxui_example_pseudo_classes.js" :cols="80" :rows="8" />

---

## 2. CSS Transitions

The `transition` property lets you define how style property changes should interpolate over time. 

### Syntax
```css
transition: <property> <duration> [<timing-function>] [<delay>]
```

You can transition multiple properties by separating them with commas, or use `all` to animate all supported properties.

### Supported Animatable Properties
* `background-color` (e.g. `#000` to `#fff`)
* `color` / `foreground-color`
* `border-top-color`, `border-right-color`, `border-bottom-color`, `border-left-color`
* `width` and `height` (e.g. `20` to `40`)
* `flex-grow` and `flex-shrink` (e.g. `1.0` to `3.0`)

### Timing Functions
The following timing/easing curves are supported:
* `linear`
* `ease` (default)
* `ease-in`
* `ease-out`
* `ease-in-out`
* `cubic-bezier(x1, y1, x2, y2)` (custom cubic bézier curves)

### Minimal Transition Demo

Below is the C++ code and the live WebAssembly terminal illustrating smooth background-color and border-color transitions. This file is located in `example/transitions.cpp`.

```cpp
#include <iostream>
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class TransitionsDemo : public Component<TransitionsDemo> {
 public:
  std::string_view Setup() override {
    return R"html(
      <div class="container">
        <h3>CSS Transitions</h3>
        <p>Hover over the box to trigger a smooth color transition:</p>
        <div class="box">Hover Me</div>
      </div>

      <style>
        self {
          display: block;
          padding: 1 2;
          background-color: rgb(15, 23, 42);
          color: rgb(241, 245, 249);
        }
        h3 {
          color: rgb(59, 130, 246);
          margin-bottom: 0;
        }
        p {
          color: rgb(148, 163, 184);
          margin-bottom: 1;
        }
        .box {
          display: block;
          border: solid;
          border-color: rgb(30, 58, 138);
          background-color: rgb(17, 24, 39);
          color: rgb(191, 219, 254);
          padding: 1 3;
          text-align: center;
          width: 24;
          transition: background-color 0.4s ease-in-out, border-color 0.3s ease-out, color 0.3s ease;
        }
        .box:hover {
          background-color: rgb(29, 78, 216);
          border-color: rgb(96, 165, 250);
          color: rgb(255, 255, 255);
        }
      </style>
    )html";
  }
};

int main() {
  auto app = Ref<TransitionsDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
```

<WasmTerminal src="/wasm/rtxui_example_transitions.js" :cols="80" :rows="8" />

---

## 3. Code Example: Animation & Layout Demo

Here is a complete, compilable example illustrating hover transitions and flex layout grows. This file is located in `example/animation.cpp`.

```cpp
#include <iostream>
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class AnimationDemo : public Component<AnimationDemo> {
 public:
  std::string_view Setup() override {
    return R"html(
      <div class="container">
        <h1>Animation & Transition Demo</h1>
        <p>Hover over the elements below to see smooth C++ terminal animations.</p>

        <div class="row">
          <div class="card btn-card">
            <h3>Button Hover Transitions</h3>
            <div class="btn">Hover Me</div>
          </div>

          <div class="card grow-card">
            <h3>Hover Grow Effect (Flex)</h3>
            <div class="grow-container">
              <div class="grow-box grow-box-1">Box 1</div>
              <div class="grow-box grow-box-2">Grow Me</div>
              <div class="grow-box grow-box-3">Box 3</div>
            </div>
          </div>
        </div>
      </div>

      <style>
        self {
          display: block;
          padding: 1 2;
          background-color: rgb(15, 23, 42);
          color: rgb(241, 245, 249);
        }
        h1 {
          color: rgb(59, 130, 246);
          margin-bottom: 1;
        }
        p {
          color: rgb(148, 163, 184);
          margin-bottom: 2;
        }
        .container {
          display: block;
        }
        .row {
          display: flex;
          flex-direction: row;
          gap: 4;
        }
        .card {
          display: block;
          border: tall;
          border-color: rgb(30, 41, 59);
          padding: 1 2;
          width: 32;
          height: 10;
        }
        h3 {
          color: rgb(148, 163, 184);
          margin-bottom: 1;
        }
        
        /* 1. Button Animations */
        .btn {
          display: block;
          border: tall;
          border-color: rgb(59, 130, 246);
          background-color: rgb(30, 58, 138);
          color: rgb(191, 219, 254);
          padding: 1 3;
          text-align: center;
          width: 16;
          transition: background-color 0.3s ease-in-out, border-color 0.2s linear, color 0.2s ease;
        }
        .btn:hover {
          background-color: rgb(29, 78, 216);
          border-color: rgb(96, 165, 250);
          color: rgb(255, 255, 255);
        }
        .btn:active {
          background-color: rgb(30, 64, 175);
          border-color: rgb(147, 197, 253);
        }

        /* 2. Hover Grow Animations */
        .grow-container {
          display: flex;
          flex-direction: row;
          width: 28;
          height: 4;
          gap: 1;
        }
        .grow-box {
          border: solid;
          border-color: rgb(30, 41, 59);
          text-align: center;
          padding: 1 1;
        }
        .grow-box-1 {
          background-color: rgb(30, 58, 138);
          width: 8;
        }
        .grow-box-2 {
          background-color: rgb(29, 78, 216);
          flex-grow: 1.0;
          transition: flex-grow 0.4s ease-in-out, background-color 0.3s linear;
        }
        .grow-box-2:hover {
          flex-grow: 3.0;
          background-color: rgb(96, 165, 250);
        }
        .grow-box-3 {
          background-color: rgb(30, 58, 138);
          width: 8;
        }
      </style>
    )html";
  }
};

int main() {
  auto app = Ref<AnimationDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
```

---

## 4. Interactive Easing & Transitions Demo

Below is the live interactive WebAssembly build of `example/animation.cpp`. Move your mouse cursor over the **Hover Me** button and the **Grow** flex box to see the CSS transitions interpolate in real-time.

<WasmTerminal src="/wasm/rtxui_example_animation.js" :cols="80" :rows="20" />

---

## 5. Other Interactive Components Using Transitions

Transitions are deeply integrated into RTXUI's default component library to provide micro-animations on interactive states.

### Slider Component
The Slider component utilizes `:hover` and `:active` transitions to smoothly highlight the track and handle.
<WasmTerminal src="/wasm/rtxui_example_slider.js" :cols="80" :rows="10" />

### Checkbox & Button Transitions
Buttons and checkboxes fade their background colors and borders on hover/clicks using transitions.
<WasmTerminal src="/wasm/rtxui_example_checkbox.js" :cols="80" :rows="12" />

### Dropdown Select Component
The Select dropdown list transitions menu items smoothly during keyboard navigation or mouse hovering.
<WasmTerminal src="/wasm/rtxui_example_select.js" :cols="80" :rows="15" />
