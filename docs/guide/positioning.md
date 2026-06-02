# Positioning and Layering

RTXUI supports advanced CSS positioning properties to position elements outside the standard flexbox flow, along with Z-index sorting to control overlapping layers.

## Positioning Modes: `position`

RTXUI elements support three positioning modes:

1. **`static`** (Default): The element is positioned according to the normal layout flow.
2. **`relative`**: The element remains in the normal flow but establishes a positioning context for any child elements using `position: absolute`.
3. **`absolute`**: The element is removed from the normal flow. It is positioned relative to its nearest positioned ancestor (usually one with `position: relative`).
4. **`fixed`**: The element is removed from the normal flow and positioned relative to the overall screen viewport boundary.

### Coordinates: `top`, `bottom`, `left`, `right`

For `absolute` and `fixed` elements, you can position the box margins relative to its container boundaries:

- `left`: Distance from the container's left edge.
- `right`: Distance from the container's right edge.
- `top`: Distance from the container's top edge.
- `bottom`: Distance from the container's bottom edge.

```html
<div class="overlay">I am absolute!</div>
<style>
  .overlay {
    position: absolute;
    top: 2;
    left: 5;
    width: 20;
    height: 3;
  }
</style>
```

---

## Layering Order: `z-index`

When elements overlap due to absolute or fixed positioning, you can control which elements appear on top using the `z-index` property.

- Elements with a higher `z-index` value are rendered on top of elements with lower values.
- Default `z-index` is `0` (or the order of declaration in the template).

```html
<div class="background-layer">Behind (Z-Index = 1)</div>
<div class="foreground-layer">In Front (Z-Index = 10)</div>
<style>
  .background-layer {
    position: absolute;
    z-index: 1;
  }
  .foreground-layer {
    position: absolute;
    z-index: 10;
  }
</style>
```

---


<ExampleTabs src="/wasm/rtxui_example_positioning.js">
<template #source>

<<< @/../example/positioning.cpp

</template>
</ExampleTabs>
