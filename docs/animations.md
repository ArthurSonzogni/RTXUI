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

<ExampleTabs src="/wasm/rtxui_example_pseudo_classes.js">
<template #source>

<<< @/../example/pseudo_classes.cpp

</template>
</ExampleTabs>

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

<ExampleTabs src="/wasm/rtxui_example_transitions.js">
<template #source>

<<< @/../example/transitions.cpp

</template>
</ExampleTabs>

---

## 3. Interactive Demos

Here are several interactive demos built with RTXUI and compiled to WebAssembly. For each example, you can interact with the live terminal display or inspect the underlying C++ source code. The demo is displayed first by default.

### Primary Animation & Flex Layout Demo
Move your mouse cursor over the **Hover Me** button and the **Grow Me** flex box to see the transitions interpolate in real-time.

<ExampleTabs src="/wasm/rtxui_example_animation.js">
<template #source>

<<< @/../example/animation.cpp

</template>
</ExampleTabs>

---

### Slider Component Demo
The Slider component utilizes `:hover` and `:active` transitions to smoothly highlight the track and adjust the value slider thumb.

<ExampleTabs src="/wasm/rtxui_example_slider.js">
<template #source>

<<< @/../example/slider.cpp

</template>
</ExampleTabs>

---

### Checkbox & Button Transitions Demo
Buttons and checkboxes fade their background colors and borders on hover/clicks using transitions.

<ExampleTabs src="/wasm/rtxui_example_checkbox.js">
<template #source>

<<< @/../example/checkbox.cpp

</template>
</ExampleTabs>

---

### Dropdown Select Component Demo
The Select dropdown list transitions menu items smoothly during keyboard navigation or mouse hovering.

<ExampleTabs src="/wasm/rtxui_example_select.js">
<template #source>

<<< @/../example/select.cpp

</template>
</ExampleTabs>
