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
- **`minmax(min, max)`** — at least `min`, at most `max`: `minmax(20, 1fr)`
  takes its share of the space but never less than 20; `minmax(5, 10)`
  grows from 5 toward 10 when there is room. `min` can be `auto` (the
  content), not `fr`.
- **`repeat(auto-fill, size)`** — as many tracks as fit in the width (or a
  known height). `repeat(auto-fit, size)` is the same, except that the
  repeated tracks no item is in are dropped, so the items share the room.
- **`calc()` expressions** — e.g. `calc(50% - 5)`.
- **`auto`** — as wide as the widest item in the track. Auto tracks share
  the space left over when no `fr` track takes it.

A card grid that adapts to the terminal's width, as many columns of at least
24 cells as fit, sharing the rest:

```css
.cards {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(24, 1fr));
  gap: 1;
}
```

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

Or name the lines it goes between. Lines are numbered from 1, and from -1
back from the end:

```css
.title  { grid-column: 1 / -1; }        /* every column */
.aside  { grid-column: 3; grid-row: 2 / span 2; }
.corner { grid-area: 1 / 1 / 3 / 3; }   /* row-start / col-start / row-end / col-end */
```

Lines can also be named, in brackets in the track list, and items placed by
name. A name standing for both edges, as in `grid-column: main`, means the
lines `main-start` and `main-end`:

```css
.page {
  display: grid;
  grid-template-columns: [full-start] 2 [main-start] 1fr [main-end] 2 [full-end];
}
.text  { grid-column: main; }
.photo { grid-column: full-start / full-end; }
```

Items placed on both axes are placed first, then those placed on their row
only, then the others fill the gaps in order, as in CSS.

## Implicit tracks and auto-placement

Rows past the template are added as items need them, and columns too when an
item is placed past the last one. They are sized by `grid-auto-rows` and
`grid-auto-columns`: `auto` by default, or a list of sizes that repeats.

`grid-auto-flow` decides how auto-placed items fill the grid: `row` (the
default) fills each row and adds rows, `column` fills each column and adds
columns. Adding `dense` puts each item in the first hole it fits, rather than
after the last item placed, so a small item can come before a large one that
did not fit:

```css
.gallery {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  grid-auto-rows: 3;
  grid-auto-flow: row dense;
}
.wide { grid-column: span 2; }
```

## Named areas

`grid-template-areas` draws the layout, one quoted string per row, and
`grid-area` puts an item in a named area:

```css
.app {
  display: grid;
  grid-template-columns: 20 1fr;
  grid-template-areas:
    "head head"
    "nav  main"
    "nav  foot";
}
.header  { grid-area: head; }
.nav     { grid-area: nav; }
.content { grid-area: main; }
.footer  { grid-area: foot; }
```

`.` leaves a cell empty. Each name must cover a rectangle, and every row must
have as many cells; a template breaking either rule is reported and ignored.
Each area also names its lines `<name>-start` and `<name>-end`, so
`grid-column: head-start / main-end` works too.

`grid-template` sets the areas, the row sizes and the columns in one go, each
row's size after its string:

```css
.app {
  display: grid;
  grid-template:
    "head head" 1
    "nav  main" 1fr
    / 20 1fr;
}
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

## Aligning the tracks in the grid

When the grid is taller than its rows, as with a `height` of its own or as a
stretched flex item, `align-content` decides where the rest goes. By default
(`stretch`) the `auto` rows grow to fill it; `start`, `end`, `center`,
`space-between`, `space-around` and `space-evenly` place the rows instead.
`justify-content` does the same with the columns in the grid's width, where
the default (`normal`) stretches the `auto` columns:

```css
.form { display: grid; height: 20; align-content: start; }
.toolbar { display: grid; grid-template-columns: auto auto auto; justify-content: end; }
```

## Demo

Fraction tracks, spanning, and gaps together:

<ExampleTabs src="/wasm/rtxui_example_grid.js">
<template #source>

<<< @/../example/grid.cpp

</template>
</ExampleTabs>
