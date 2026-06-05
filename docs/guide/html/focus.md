# Keyboard Focus & Traversal

Interactive applications require keyboard focus routing so components can intercept keystrokes or mouse clicks.

## Focus Control

To make any layout container (like a custom card or wrapper `<div>`) interactive and focusable, use these properties:

### `tabindex`
Determines where the element fits in the sequential keyboard **Tab** navigation loop:
*   `tabindex="0"`: The element is sequentially focusable (standard behavior).
*   `tabindex="-1"`: The element is focusable programmatically or via mouse clicks, but skipped when hitting Tab/Shift-Tab.
*   `tabindex="1"` (or higher): Establishes custom sequential traversal order (lower positive values focus first).

```html
<!-- Tab navigates to this div first, then the next -->
<div tabindex="1">First focusable zone</div>
<div tabindex="2">Second focusable zone</div>
```

### `focusable` Shorthand
Set `focusable="true"` as a clean shorthand for setting `tabindex="0"`:
```html
<div focusable="true" class="button">
  Interactive Box
</div>
```
Focusable items automatically capture keyboard events (`ArrowLeft`, `Enter`, etc.) and focus CSS pseudoclasses like `:focus`.

## Live Demo

<ExampleTabs src="/wasm/rtxui_example_tabindex.js">
<template #source>

<<< @/../example/tabindex.cpp

</template>
</ExampleTabs>
