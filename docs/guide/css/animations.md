# Transitions & Hover States

RTXUI supports CSS-style transitions and interactive pseudo-classes on DOM elements. This allows you to build terminal applications with smooth color shifts, expanding borders, and responsive mouse effects without writing manual frame updates.

---

## 1. Interactive Pseudo-Classes

You can define styles that apply only under specific user interaction states:

*   **`:hover`**: Active when the mouse cursor is hovering over the element or its children.
*   **`:active`**: Active while the element is being clicked/pressed.
*   **`:focus`**: Active when the element has captured keyboard focus.

### Example Rulesets
```css
.button {
  background-color: rgb(30, 58, 138); /* Slate Blue */
}
.button:hover {
  background-color: rgb(29, 78, 216); /* Bright Blue on hover */
}
.button:active {
  background-color: rgb(30, 64, 175); /* Dark Blue on click */
}
```

### Minimal Interactive Demo
Below is the interactive tab view for `:hover`, `:active`, and `:focus` pseudo-classes:

<ExampleTabs src="/wasm/rtxui_example_pseudo_classes.js">
<template #source>

<<< @/../example/pseudo_classes.cpp

</template>
</ExampleTabs>

---

## 2. CSS Transitions

The `transition` property configures how style attributes interpolate smoothly over time instead of changing instantly.

### Shorthand Syntax
```css
transition: <property> <duration> [<timing-function>] [<delay>]
```
You can declare multiple property transitions by separating them with commas, or use the keyword `all` to animate all supported variables:
```css
transition: background-color 0.2s ease-in-out, width 0.3s ease-out;
```

### Animatable Properties
*   **Dimensions**: `width`, `height` (e.g. `20` to `40` cells).
*   **Borders**: `border-top-color`, `border-right-color`, `border-bottom-color`, `border-left-color` (or shorthand `border-color`).
*   **Colors & Opacities**: `color`, `background-color`, `opacity`.
*   **Flex Constraints**: `flex-grow`, `flex-shrink`.

### Easing Functions
*   `linear`
*   `ease` (Default curve)
*   `ease-in`
*   `ease-out`
*   `ease-in-out`
*   `cubic-bezier(x1, y1, x2, y2)` (Defines a custom cubic Bézier easing curve)

### Minimal Transition Demo
Below is the interactive tab view for background-color and border-color transitions:

<ExampleTabs src="/wasm/rtxui_example_transitions.js">
<template #source>

<<< @/../example/transitions.cpp

</template>
</ExampleTabs>

---

## 3. Advanced Transition Examples

### Primary Animation & Flex Layout Demo
Hover over the **Hover Me** button and click **Grow Me** to observe interactive transitions reflow the flex container:

<ExampleTabs src="/wasm/rtxui_example_animation.js">
<template #source>

<<< @/../example/animation.cpp

</template>
</ExampleTabs>

---

### Component-Level Transition Demos

*   **Slider Transition**: Smooth track and thumb focus highlighting. (See [Slider Demo](/wasm/rtxui_example_slider.js))
*   **Checkbox Transition**: Background check state changes. (See [Checkbox Demo](/wasm/rtxui_example_checkbox.js))
*   **Dropdown Select Menu**: Highlight shifts. (See [Select Demo](/wasm/rtxui_example_select.js))
