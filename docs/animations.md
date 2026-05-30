<script setup>
import { ref } from 'vue'
const tabPseudo = ref('demo')
const tabTransitions = ref('demo')
const tabAnimation = ref('demo')
const tabSlider = ref('demo')
const tabCheckbox = ref('demo')
const tabSelect = ref('demo')
</script>


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

Below is the interactive tab view for `:hover`, `:active`, and `:focus` pseudo-classes (defined in `example/pseudo_classes.cpp`).

<div class="tabs-container">
  <div class="tabs-nav">
    <button class="tab-btn" :class="{ active: tabPseudo === 'demo' }" @click="tabPseudo = 'demo'">Interactive Demo</button>
    <button class="tab-btn" :class="{ active: tabPseudo === 'code' }" @click="tabPseudo = 'code'">C++ Source Code</button>
  </div>
  <div class="tab-content">
    <div v-show="tabPseudo === 'demo'">
      <WasmTerminal src="/wasm/rtxui_example_pseudo_classes.js" :cols="80" :rows="60" />
    </div>
    <div v-show="tabPseudo === 'code'">
      <div v-pre>

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
  </div>
</div>
</div>
</div>

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

Below is the interactive tab view for smooth background-color and border-color transitions (defined in `example/transitions.cpp`).

<div class="tabs-container">
  <div class="tabs-nav">
    <button class="tab-btn" :class="{ active: tabTransitions === 'demo' }" @click="tabTransitions = 'demo'">Interactive Demo</button>
    <button class="tab-btn" :class="{ active: tabTransitions === 'code' }" @click="tabTransitions = 'code'">C++ Source Code</button>
  </div>
  <div class="tab-content">
    <div v-show="tabTransitions === 'demo'">
      <WasmTerminal src="/wasm/rtxui_example_transitions.js" :cols="80" :rows="60" />
    </div>
    <div v-show="tabTransitions === 'code'">
      <div v-pre>

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
  </div>
</div>
</div>
</div>

---

## 3. Interactive Demos

Here are several interactive demos built with RTXUI and compiled to WebAssembly. For each example, you can interact with the live terminal display or inspect the underlying C++ source code. The demo is displayed first by default.

### Primary Animation & Flex Layout Demo
Move your mouse cursor over the **Hover Me** button and the **Grow Me** flex box to see the transitions interpolate in real-time.

<div class="tabs-container">
  <div class="tabs-nav">
    <button class="tab-btn" :class="{ active: tabAnimation === 'demo' }" @click="tabAnimation = 'demo'">Interactive Demo</button>
    <button class="tab-btn" :class="{ active: tabAnimation === 'code' }" @click="tabAnimation = 'code'">C++ Source Code</button>
  </div>
  <div class="tab-content">
    <div v-show="tabAnimation === 'demo'">
      <WasmTerminal src="/wasm/rtxui_example_animation.js" :cols="80" :rows="60" />
    </div>
    <div v-show="tabAnimation === 'code'">
      <div v-pre>

```cpp
// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
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
  </div>
</div>
</div>
</div>

---

### Slider Component Demo
The Slider component utilizes `:hover` and `:active` transitions to smoothly highlight the track and adjust the value slider thumb.

<div class="tabs-container">
  <div class="tabs-nav">
    <button class="tab-btn" :class="{ active: tabSlider === 'demo' }" @click="tabSlider = 'demo'">Interactive Demo</button>
    <button class="tab-btn" :class="{ active: tabSlider === 'code' }" @click="tabSlider = 'code'">C++ Source Code</button>
  </div>
  <div class="tab-content">
    <div v-show="tabSlider === 'demo'">
      <WasmTerminal src="/wasm/rtxui_example_slider.js" :cols="80" :rows="60" />
    </div>
    <div v-show="tabSlider === 'code'">
      <div v-pre>

```cpp
// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <iostream>
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class SliderDemo : public Component<SliderDemo> {
 public:
  int volume = 50;

  SliderDemo() { Bind(volume); }

  std::string_view Setup() override {
    return R"html(
      <div class="container">
        <p class="title">Interactive Slider Element</p>
        <p class="desc">
          Drag the thumb with mouse click or navigate using Arrow Keys (Left/Right or Up/Down) when focused to adjust the value.
        </p>
        
        <div class="slider-wrapper">
          <span class="label">Volume:</span>
          <slider value="{volume}" min="0" max="100" step="5" width="30"></slider>
        </div>
        
        <div class="output-box">
          <span class="label">Live Volume Value:</span>
          <span class="value">{volume}%</span>
        </div>
      </div>

      <style>
        self {
          display: block;
          padding: 1;
          background-color: rgb(15, 23, 42);
          color: white;
          border: tall;
          border-color: rgb(56, 189, 248);
        }
        .container {
          display: block;
        }
        .title {
          display: block;
          font-weight: bold;
          color: rgb(56, 189, 248);
          margin-bottom: 1;
        }
        .desc {
          display: block;
          color: rgb(156, 163, 175);
          margin-bottom: 2;
        }
        .slider-wrapper {
          display: flex;
          gap: 2;
          margin-bottom: 2;
          align-items: center;
        }
        .output-box {
          display: flex;
          gap: 2;
          margin-top: 2;
        }
        .label {
          color: rgb(56, 189, 248);
        }
        .value {
          font-weight: bold;
          color: rgb(244, 63, 94);
        }
      </style>
    )html";
  }
};

int main() {
  auto app = Ref<SliderDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
```
  </div>
</div>
</div>
</div>

---

### Checkbox & Button Transitions Demo
Buttons and checkboxes fade their background colors and borders on hover/clicks using transitions.

<div class="tabs-container">
  <div class="tabs-nav">
    <button class="tab-btn" :class="{ active: tabCheckbox === 'demo' }" @click="tabCheckbox = 'demo'">Interactive Demo</button>
    <button class="tab-btn" :class="{ active: tabCheckbox === 'code' }" @click="tabCheckbox = 'code'">C++ Source Code</button>
  </div>
  <div class="tab-content">
    <div v-show="tabCheckbox === 'demo'">
      <WasmTerminal src="/wasm/rtxui_example_checkbox.js" :cols="80" :rows="60" />
    </div>
    <div v-show="tabCheckbox === 'code'">
      <div v-pre>

```cpp
// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <iostream>
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class CheckboxDemo : public Component<CheckboxDemo> {
 public:
  bool checked = false;

  CheckboxDemo() { Bind(checked); }

  std::string_view Setup() override {
    return R"html(
      <div class="container">
        <p class="title">Interactive Checkbox Element</p>
        <p class="desc">A binary toggle component. Click on the checkbox or focus it and press Space to toggle the state.</p>
        
        <checkbox checked="{checked}">Enable Notifications</checkbox>
        
        <div class="output-box">
          <span class="label">Live Checked State:</span>
          <span class="value">{checked}</span>
        </div>
      </div>

      <style>
        self {
          display: block;
          padding: 1;
          background-color: rgb(15, 23, 42);
          color: white;
          border: tall;
          border-color: rgb(56, 189, 248);
        }
        .container {
          display: block;
        }
        .title {
          display: block;
          font-weight: bold;
          color: rgb(56, 189, 248);
          margin-bottom: 1;
        }
        .desc {
          display: block;
          color: rgb(156, 163, 175);
          margin-bottom: 2;
        }
        .output-box {
          display: flex;
          gap: 2;
          margin-top: 2;
        }
        .label {
          color: rgb(56, 189, 248);
        }
        .value {
          font-weight: bold;
          color: rgb(244, 63, 94);
        }
      </style>
    )html";
  }
};

int main() {
  auto app = Ref<CheckboxDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
```
  </div>
</div>
</div>
</div>

---

### Dropdown Select Component Demo
The Select dropdown list transitions menu items smoothly during keyboard navigation or mouse hovering.

<div class="tabs-container">
  <div class="tabs-nav">
    <button class="tab-btn" :class="{ active: tabSelect === 'demo' }" @click="tabSelect = 'demo'">Interactive Demo</button>
    <button class="tab-btn" :class="{ active: tabSelect === 'code' }" @click="tabSelect = 'code'">C++ Source Code</button>
  </div>
  <div class="tab-content">
    <div v-show="tabSelect === 'demo'">
      <WasmTerminal src="/wasm/rtxui_example_select.js" :cols="80" :rows="60" />
    </div>
    <div v-show="tabSelect === 'code'">
      <div v-pre>

```cpp
// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <iostream>
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class SelectDemo : public Component<SelectDemo> {
 public:
  std::string my_theme = "light";

  SelectDemo() { Bind(my_theme); }

  std::string_view Setup() override {
    return R"html(
      <div class="container">
        <p class="title">Interactive Select & Option Elements</p>
        <p class="desc">Click the select dropdown or focus it with Tab and use Enter/Space to open. Navigate options using ArrowUp/ArrowDown, and select with Enter.</p>
        
        <select value="{my_theme}">
          <option value="dark">Dark Theme</option>
          <option value="light">Light Theme</option>
          <option value="solarized">Solarized</option>
        </select>
        
        <div class="output-box">
          <span class="label">Current Theme Value:</span>
          <span class="value">{my_theme}</span>
        </div>
      </div>

      <style>
        self {
          display: block;
          padding: 1;
          background-color: rgb(15, 23, 42);
          color: white;
          border: tall;
          border-color: rgb(56, 189, 248);
        }
        .container {
          display: block;
        }
        .title {
          display: block;
          font-weight: bold;
          color: rgb(56, 189, 248);
          margin-bottom: 1;
        }
        .desc {
          display: block;
          color: rgb(156, 163, 175);
          margin-bottom: 2;
        }
        .output-box {
          display: flex;
          gap: 2;
          margin-top: 2;
        }
        .label {
          color: rgb(56, 189, 248);
        }
        .value {
          font-weight: bold;
          color: rgb(244, 63, 94);
        }
      </style>
    )html";
  }
};

int main() {
  auto app = Ref<SelectDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
```
  </div>
</div>
</div>
</div>

<style>
.tabs-container {
  border: 1px solid var(--vp-c-divider);
  border-radius: 8px;
  margin: 20px 0;
  overflow: hidden;
  background-color: var(--vp-c-bg-soft);
}
.tabs-nav {
  display: flex;
  background-color: var(--vp-c-bg-mute);
  border-bottom: 1px solid var(--vp-c-divider);
  padding: 0 12px;
}
.tab-btn {
  padding: 10px 16px;
  font-size: 14px;
  font-weight: 500;
  color: var(--vp-c-text-2);
  border: none;
  background: none;
  cursor: pointer;
  border-bottom: 2px solid transparent;
  transition: all 0.2s ease;
}
.tab-btn:hover {
  color: var(--vp-c-text-1);
}
.tab-btn.active {
  color: var(--vp-c-brand-1);
  border-bottom-color: var(--vp-c-brand-1);
  font-weight: 600;
}
.tab-content {
  padding: 16px;
}
</style>
