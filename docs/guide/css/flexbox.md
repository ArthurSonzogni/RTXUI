# Flexbox Layouts

To design structured terminal views that adapt dynamically to varying viewport sizes, use the **Flexbox** positioning engine.

## Activating Flex Layout

To lay out children inside a container using flex rules, set the display property of the parent to `flex`:

```css
.row-container {
  display: flex;
  flex-direction: row; /* Layout items side-by-side */
}
```

## Alignment Properties

*   **`flex-direction`**: Defines the main axis formatting flow:
    *   `row` (default): Stacks child elements horizontally from left to right.
    *   `column`: Stacks child elements vertically from top to bottom.
*   **`flex-grow`**: Factors how much free space the element claims along the main axis:
    *   `flex-grow: 1`: Expands the element to fill all available leftover cells.
*   **`flex-shrink`**: Factors how much the element shrinks when space is constrained:
    *   `flex-shrink: 0`: Prevents the element from shrinking below its default content boundaries.

```css
.sidebar {
  width: 25;
  flex-shrink: 0; /* Keeps sidebar width exactly 25 cells */
}
.main-content {
  flex-grow: 1; /* Claims remaining terminal width */
}
```
## Live Demo

A row of boxes laid out with `justify-content` and `align-items`:

<ExampleTabs src="/wasm/rtxui_example_layout.js">
<template #source>

<<< @/../example/layout.cpp

</template>
</ExampleTabs>

## Live Demo - Interactive Flex Playground

The same properties, driven from the UI. Change `flex-direction`,
`justify-content`, `align-items` and the per-item `flex-grow`/`flex-shrink`
/`flex-basis`, and watch the boxes redistribute:

<ExampleTabs src="/wasm/rtxui_example_layout_flex.js" :cols="100">
<template #source>

<<< @/../example/layout_flex.cpp

</template>
</ExampleTabs>
