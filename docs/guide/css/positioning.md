# Positioning & Layers

By default, RTXUI components arrange elements within the standard flexbox layout flow. For overlay blocks, modals, status bars, or layered widgets, you can use absolute coordinates and z-index ordering.

---

## 1. Positioning Modes (`position`)

RTXUI supports element positioning contexts:

1.  **`static`** (Default): The element renders in order within the normal flex layout flow.
2.  **`relative`**: The element remains in the normal flow but establishes a reference origin point for any descendant child elements set to `position: absolute`.
3.  **`absolute`**: The element is removed from the normal flow. It is positioned relative to its closest parent ancestor that has `position: relative` (or the root screen boundary if no ancestor is relative).
4.  **`fixed`**: The element is removed from the normal flow and positioned relative to the outermost terminal screen viewport.
5.  **`sticky`**: The element is positioned based on the user's scroll position. It behaves like `relative` until the viewport scrolls past a given offset (such as `top: 0`), at which point it "sticks" to that position, similar to `fixed`.

---

## 2. Offset Coordinates

For elements configured with `position: absolute`, `position: fixed`, or `position: sticky`, use these properties to set the spacing offsets from the container margins (measured in character cells):

*   `left`: Spacing from the container's left edge.
*   `right`: Spacing from the container's right edge.
*   `top`: Spacing from the container's top edge.
*   `bottom`: Spacing from the container's bottom edge.

A sticky element honours all four offsets. `top`/`left` pin it as it scrolls
up or left out of view; `bottom`/`right` pin it while it is still below or to
the right of the viewport, which is how a footer stays visible until the
content it belongs to scrolls past. When both edges of one axis are set, the
leading edge (`top`, `left`) wins.

```html
<div class="modal-box">Centered Overlay</div>

<style>
  .modal-box {
    position: absolute;
    top: 5;
    left: 10;
    width: 40;
    height: 10;
    border: solid;
  }
</style>
```

---

## 3. Layer Ordering (`z-index`)

To manage depth when multiple absolute or fixed elements overlap, assign the drawing layers using `z-index`:

*   Elements with a **higher `z-index`** are drawn on top of elements with lower values.
*   Default `z-index` value is `0` (where overlap order falls back to template declaration order).

```html
<div class="background-pane">Behind (z-index = 1)</div>
<div class="foreground-pane">In Front (z-index = 10)</div>

<style>
  .background-pane {
    position: absolute;
    z-index: 1;
  }
  .foreground-pane {
    position: absolute;
    z-index: 10;
  }
</style>
```

---

## Interactive Demo

Below is the interactive tab view for layout layering:

<ExampleTabs src="/wasm/rtxui_example_positioning.js">
<template #source>

<<< @/../example/positioning.cpp

</template>
</ExampleTabs>

---

## Sticky Positioning Demo

The interactive demo below showcases `position: sticky` inside a scrollable container. Notice how category headers stay pinned at the top until they are pushed out of the way by the next category.

<ExampleTabs src="/wasm/rtxui_example_sticky.js">
<template #source>

<<< @/../example/sticky.cpp

</template>
</ExampleTabs>
