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

## Differences from CSS

Two box-model behaviours differ from a browser and are worth knowing before
you port a stylesheet:

**Vertical margins do not collapse.** In CSS, a `margin-bottom: 1` above a
`margin-top: 1` collapse into a single cell of space. Here they add up to two.
Set the margin on one side only when you want a predictable gap, or use `gap`
on a flex container:

```css
.a { margin-bottom: 1; }
.b { margin-top: 1; }   /* two blank rows between them, not one */
```

**Auto margins centre inside a parent, not at the component root.**
`margin: 0 auto` (and the `margin-left`/`margin-right` longhands) centre a
fixed-width block within its containing block, and a single `auto` pushes the
block to the opposite edge. A block that is the component's own root element
has no containing block to centre within, so wrap it:

```html
<div><div class="card">centred</div></div>
```

```css
.card { width: 40; margin: 0 auto; }
```

For centring a whole screen, `display: flex` with `justify-content: center` on
the root is usually clearer than auto margins.

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

Both forms combine in `calc()` and are bounded by `min()`, `max()`, and
`clamp()` — see [values in a terminal](/guide/css/basics#values-in-a-terminal).
`min-width`/`max-width` and `min-height`/`max-height` constrain the resolved
size.

## box-sizing

`width` and `height` describe the border box (border + padding + content) by
default — unlike web CSS, whose initial value is `content-box`. A box styled
`width: 10; padding-left: 2; border: solid;` is exactly 10 cells wide; the
content area shrinks to fit inside the padding and border.

```css
.card { width: 10; padding: 1; border: solid; } /* 10 cells wide total */
```

Set `box-sizing: content-box` to make `width`/`height` (and their `min-`/`max-`
variants, and `flex-basis`) describe the content area instead, with padding
and border added on top — the familiar web-CSS behavior:

```css
.card {
  box-sizing: content-box;
  width: 10;    /* 10 cells of content ... */
  padding: 1;   /* ... plus 1 cell of padding each side ... */
  border: solid; /* ... plus a 1-cell border each side: 14 cells total. */
}
```

## Aspect Ratio

`aspect-ratio: <width> / <height>` derives a block element's automatic
height from its used width. Ratios are measured in cells; since terminal
cells are roughly twice as tall as they are wide, `2 / 1` produces a
visually square box:

```css
.tile { width: 30%; aspect-ratio: 2 / 1; }
```

An explicit `height`, or `min-`/`max-height`, takes precedence. Content
taller than the ratio overflows — combine with `overflow` when clipping is
wanted.

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

