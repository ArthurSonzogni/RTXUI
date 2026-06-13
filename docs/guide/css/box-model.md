# Box Model & Dimensions

RTXUI elements behave as standard CSS boxes, though spacing measurements represent terminal character cells instead of pixels.

## Margin & Padding

Spacing properties accept positive integer values representing the number of character columns (for horizontal spacing) or character rows (for vertical spacing):

*   **Shorthand Syntax**:
    *   `padding: 1`: Spacing of 1 cell on all sides (top, right, bottom, left).
    *   `padding: 1 2`: Spacing of 1 vertical cell (top/bottom) and 2 horizontal cells (left/right).
*   **Specific Properties**:
    *   `margin-top`, `margin-bottom`, `margin-left`, `margin-right`
    *   `padding-top`, `padding-bottom`, `padding-left`, `padding-right`

```css
.card {
  margin: 1 2; /* 1 row margin top/bottom, 2 columns left/right */
  padding: 1;   /* 1 cell padding internally on all sides */
}
```

---

## Dimension Units

Sizing attributes `width` and `height` accept two types of length units:

1.  **Integer Cell Counts**: A raw number indicates character cell counts.
    ```css
    .sidebar { width: 20; } /* Exactly 20 columns wide */
    ```
2.  **Percentage Boundaries**: Appending a `%` dynamically resolves the dimension relative to the parent box.
    ```css
    .half-pane { width: 50%; } /* Fills half the parent width */
    ```

## Live Demo - Borders

<ExampleTabs src="/wasm/rtxui_example_borders.js">
<template #source>

<<< @/../example/borders.cpp

</template>
</ExampleTabs>

## Live Demo - Borders & Scrollbars Interaction

To see how all the different border styles behave interactively and how they coordinate with scrollbars when content overflows, check out the following interactive demo.

<ExampleTabs src="/wasm/rtxui_example_border_scroll_demo.js">
<template #source>

<<< @/../example/border_scroll_demo.cpp

</template>
</ExampleTabs>

