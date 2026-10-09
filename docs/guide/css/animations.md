# Transitions, Animations & Hover States

RTXUI supports CSS-style transitions, keyframe animations and interactive pseudo-classes on DOM elements. This allows you to build terminal applications with smooth color shifts, expanding borders, pulsing indicators and responsive mouse effects without writing manual frame updates.

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
*   **Offsets**: `top`, `right`, `bottom`, `left` (or shorthand `inset`), which move a positioned element. Prefer moving sideways: a terminal has more columns than rows, so a horizontal move takes smaller steps.
*   **Translation**: `translate`, which moves any box without moving what is around it. Its percentages are of the box's own size, so `from { translate: calc(100% + 2); }` slides a box in from just past its own right edge, whatever its width.
*   **Borders**: `border-top-color`, `border-right-color`, `border-bottom-color`, `border-left-color` (or shorthand `border-color`).
*   **Colors & Opacities**: `color`, `background-color`, `opacity`.
*   **Flex Constraints**: `flex-grow`, `flex-shrink`.

### Easing Functions
*   `linear`
*   `ease` (Default curve)
*   `ease-in`
*   `ease-out`
*   `ease-in-out`
*   **Sine curves**: `ease-in-sine`, `ease-out-sine`, `ease-in-out-sine`
*   **Quad curves**: `ease-in-quad`, `ease-out-quad`, `ease-in-out-quad`
*   **Cubic curves**: `ease-in-cubic`, `ease-out-cubic`, `ease-in-out-cubic`
*   **Quart curves**: `ease-in-quart`, `ease-out-quart`, `ease-in-out-quart`
*   **Quint curves**: `ease-in-quint`, `ease-out-quint`, `ease-in-out-quint`
*   **Expo curves**: `ease-in-expo`, `ease-out-expo`, `ease-in-out-expo`
*   **Circ curves**: `ease-in-circ`, `ease-out-circ`, `ease-in-out-circ`
*   **Back curves**: `ease-in-back`, `ease-out-back`, `ease-in-out-back`
*   `cubic-bezier(x1, y1, x2, y2)` (Defines a custom cubic Bézier easing curve)
*   **Steps**: `steps(n)` holds each value for `1/n` of the time, then jumps to the next; `steps(n, start)` jumps at the start of each step instead. `step-start` and `step-end` are `steps(1, start)` and `steps(1)`. Terminal cells are discrete, so steps often look cleaner than a smooth curve.

### Minimal Transition Demo
Below is the interactive tab view for background-color and border-color transitions:

<ExampleTabs src="/wasm/rtxui_example_transitions.js">
<template #source>

<<< @/../example/transitions.cpp

</template>
</ExampleTabs>

---

## 3. Keyframe Animations

A transition needs a change of state to run. An animation runs on its own as soon as an element declares it, which suits indicators: a pulsing status light, a loading bar, a blinking cursor.

`@keyframes` names a sequence of styles, and `animation` plays it:

```css
@keyframes pulse {
  from { color: rgb(35, 134, 54); }
  to { color: rgb(126, 231, 135); }
}

.light {
  animation: pulse 0.8s ease-in-out infinite alternate;
}
```

### Shorthand Syntax
```css
animation: <name> <duration> [<timing-function>] [<delay>] [<iteration-count>]
           [<direction>] [<fill-mode>] [<play-state>]
```

The parts may come in any order: the first time is the duration and the second the delay. Each also has its own longhand, `animation-name`, `animation-duration`, and so on (see the [CSS reference](/css_reference)).

*   **`iteration-count`**: a number, or `infinite`.
*   **`direction`**: `normal`, `reverse`, `alternate` (forwards, then backwards) or `alternate-reverse`.
*   **`fill-mode`**: `forwards` holds the last keyframe once the animation ends, `backwards` shows the first one during the delay, `both` does both. With the default, `none`, the properties go back to their own values.
*   **`play-state`**: `paused` freezes the animation where it is; `running` resumes it from there.

### Keyframes

Blocks are selected by `from`, `to` or a percentage, several at once when comma-separated (`0%, 100% { ... }`). A property missing from the first or the last block animates from, or to, the element's own value. A block can declare its own `animation-timing-function`, which eases the way from it to the next block:

```css
@keyframes blink {
  from { opacity: 1; }
  50% { opacity: 0; }
}
.cursor { animation: blink 1s step-end infinite; }
```

Keyframes animate the same properties as transitions (listed above). Anything else in a block is reported as a [diagnostic](/guide/diagnostics), as is an `animation` whose name matches no `@keyframes`.

### Where Keyframes Are Found

Like every other rule, `@keyframes` belong to the component that declares them: an animation looks for its keyframes in the stylesheet of the component whose rule declared it, and nowhere else. A component can still animate a child component's tag from its own stylesheet, since the tag is part of its own template.

### Lifetime

*   An animation starts when an element starts declaring it, and keeps running when the component re-renders.
*   Changing its duration, easing or play state does not restart it. Removing it (for example by removing the class that declares it) stops it; adding it back starts it again from the beginning.
*   An element with `display: none` runs no animation. Its animations start from the beginning once it is displayed, so an element that appears plays them in full.
*   While any animation plays, the screen keeps redrawing. A [headless](/guide/headless) run waits for finite animations to end, but not for `infinite` ones.
*   A length animates smoothly between cells, percentages and `calc()`, in any mix (`50%` to `10` is `calc(25% + 5)` halfway). From or to `auto`, it jumps at the end.

### Animation End

`onanimationend` runs a handler once one of the element's animations finishes, as `onclick` does on a click. It does not run for an `infinite` animation, nor for one removed before its end. Use it to act after an exit animation, such as removing the item that just faded out:

<!-- snippet: fragment -->
```html
<div class="{row_class}" onanimationend="Removed">{label}</div>
```

[`<toast>`](/html_reference#toast) uses it to hide only once it has slid out.

### Keyframes Demo

<ExampleTabs src="/wasm/rtxui_example_keyframes.js">
<template #source>

<<< @/../example/keyframes.cpp

</template>
</ExampleTabs>

### Translate Demo

Cards lift through a `translate` transition when hovered or focused, and each task slides out of its list once done, removed by `onanimationend`:

<ExampleTabs src="/wasm/rtxui_example_translate.js">
<template #source>

<<< @/../example/translate.cpp

</template>
</ExampleTabs>

---

## 4. Advanced Transition Examples

### Primary Animation & Flex Layout Demo
Hover over the **Hover Me** button and click **Grow Me** to observe interactive transitions reflow the flex container:

<ExampleTabs src="/wasm/rtxui_example_animation.js">
<template #source>

<<< @/../example/animation.cpp

</template>
</ExampleTabs>

---

### Component-Level Transition Demos

*   **Slider Transition**: Smooth track and thumb focus highlighting. (See [Slider Demo](/guide/examples/slider))
*   **Checkbox Transition**: Background check state changes. (See [Checkbox Demo](/guide/examples/checkbox))
*   **Dropdown Select Menu**: Highlight shifts. (See [Select Demo](/guide/examples/select))
