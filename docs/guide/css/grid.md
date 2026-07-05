# Grid

`display: grid` lays children out on a two-dimensional track grid — the
right tool when rows and columns must stay aligned with each other, which
flexbox cannot guarantee. Dashboards, key/value panes, and button pads are
typical uses.

## Defining tracks

Column and row tracks are lists of sizes:

```css
.board {
  display: grid;
  grid-template-columns: 10 1fr 2fr;
  grid-template-rows: repeat(3, 1fr);
  gap: 1;
}
```

Track sizes accept:

- **Cells** — `10` is ten characters wide.
- **Percentages** — resolved against the container.
- **Fractions** — `1fr`, `2fr`: after fixed tracks are placed, remaining
  space is divided proportionally.
- **`repeat(count, size)`** — shorthand for repeated tracks.
- **`calc()` expressions** — e.g. `calc(50% - 5)`.

`grid-template` combines both axes as `rows / columns`:

```css
grid-template: 1fr 1fr / 10 1fr 10;
```

`gap` (one value, or `row-gap column-gap` as two) inserts space between
tracks without adding outer margins.

## Placing and spanning items

Children fill the grid in source order, left to right, top to bottom. An
item can cover several tracks:

```css
.header  { grid-column: span 2; }
.sidebar { grid-row: span 3; }
```

## Aligning items in their cells

By default items stretch to fill their cell. `justify-items` (inline axis)
and `align-items` (block axis) on the container change that for all items;
`justify-self` / `align-self` override per item, and an explicit
width/height opts an item out of stretching:

```css
.board { justify-items: center; align-items: start; }
.badge { justify-self: end; }
```

`place-items` and `place-self` set both axes at once
(`place-items: center center`).

## Demo

Fraction tracks, spanning, and gaps together:

<ExampleTabs src="/wasm/rtxui_example_grid.js">
<template #source>

<<< @/../example/grid.cpp

</template>
</ExampleTabs>
