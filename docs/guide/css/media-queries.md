# Terminal Media Queries

RTXUI supports media queries to automatically adapt component styling when the terminal viewport is resized.

## Viewport Features

Media queries parse current terminal size columns and rows:
*   `width` / `min-width` / `max-width` (measured in character columns)
*   `height` / `min-height` / `max-height` (measured in character rows)

## Responsive Rulesets

Define conditional styles by enclosing rules within `@media` blocks. Multiple rules can be chained using the `and` operator:

```css
/* General mobile layout (terminal width < 60 columns) */
@media (max-width: 59) {
  .container {
    flex-direction: column;
  }
  .sidebar {
    width: 100%;
    border-right: none;
    border-bottom: solid;
  }
}

/* Landscape desktop constraints */
@media (min-width: 80) and (min-height: 24) {
  .container {
    flex-direction: row;
    padding: 1 2;
  }
}
```
Whenever the terminal interface detects a resize event, styles are automatically re-evaluated, and layout reflow runs instantly.

## Live Demo

<ExampleTabs src="/wasm/rtxui_example_media.js">
<template #source>

<<< @/../example/media.cpp

</template>
</ExampleTabs>
