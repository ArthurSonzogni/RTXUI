# Keyboard Focus & Navigation

RTXUI applications are fully keyboard-operable out of the box. This page
describes what users can do with the keyboard, and how to control which
elements participate.

## Built-in keys

| Key | Behavior |
| :--- | :--- |
| <kbd>Tab</kbd> / <kbd>Shift-Tab</kbd> | Move focus through focusable elements in order. |
| Arrow keys / <kbd>h</kbd> <kbd>j</kbd> <kbd>k</kbd> <kbd>l</kbd> | Spatial navigation: focus the nearest focusable element in that direction; inside a scrollable, scroll. |
| <kbd>Enter</kbd> / <kbd>Space</kbd> | Activate the focused element (equivalent to a click). |
| <kbd>PageUp</kbd> / <kbd>PageDown</kbd> | Scroll the focused scrollable by a viewport. |
| <kbd>Home</kbd> / <kbd>End</kbd> | Jump the focused scrollable to its start or end. |
| <kbd>Escape</kbd> | Close the open `<dialog>`; otherwise exit the application. |

Component `OnEvent` overrides see events before these built-in behaviors, so
custom key handling always wins.

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

## Spatial Navigation

In addition to sequential tab navigation (using Tab/Shift-Tab), RTXUI supports **Spatial Navigation**. By pressing the **Arrow keys** (or **hjkl** keys), RTXUI performs a geometric search in 2D screen space to find the closest focusable element in that direction and moves focus there. This provides a natural, game-like, or TV-like focus navigation experience for complex layouts.

### Live Demo - Spatial Navigation

<ExampleTabs src="/wasm/rtxui_example_spatial_navigation.js">
<template #source>

<<< @/../example/spatial_navigation.cpp

</template>
</ExampleTabs>

