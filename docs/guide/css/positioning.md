# Positioning & Layers

By default, RTXUI components arrange elements within the standard flexbox layout flow. For overlay blocks, modals, status bars, or layered widgets, you can use absolute coordinates and z-index ordering.

---

## 1. Positioning Modes (`position`)

RTXUI supports four different element positioning contexts:

1.  **`static`** (Default): The element renders in order within the normal flex layout flow.
2.  **`relative`**: The element remains in the normal flow but establishes a reference origin point for any descendant child elements set to `position: absolute`.
3.  **`absolute`**: The element is removed from the normal flow. It is positioned relative to its closest parent ancestor that has `position: relative` (or the root screen boundary if no ancestor is relative).
4.  **`fixed`**: The element is removed from the normal flow and positioned relative to the outermost terminal screen viewport.

---

## 2. Offset Coordinates

For elements configured with `position: absolute` or `position: fixed`, use these properties to set the spacing offsets from the container margins (measured in character cells):

*   `left`: Spacing from the container's left edge.
*   `right`: Spacing from the container's right edge.
*   `top`: Spacing from the container's top edge.
*   `bottom`: Spacing from the container's bottom edge.

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
