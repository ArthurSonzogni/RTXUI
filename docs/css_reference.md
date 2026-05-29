# RTXUI CSS Attribute Reference

This reference catalog lists all CSS-like styling properties supported by the RTXUI layout and painting engine.

---

## 1. Box Model (Margin & Padding)

Standard spacing properties to control layout positioning.

- `margin` [`<integer>`](#integers): Shorthand to configure margin width on all sides of the element.
  - `margin-top` [`<integer>`](#integers): Vertical space above the element. Sibling block margins collapse according to standard block flow rules.
  - `margin-bottom` [`<integer>`](#integers): Vertical space below the element.
  - `margin-left` [`<integer>`](#integers): Horizontal space to the left of the element.
  - `margin-right` [`<integer>`](#integers): Horizontal space to the right of the element.
- `padding` [`<integer>`](#integers): Shorthand to configure padding width inside the boundary on all sides of the element.
  - `padding-top` [`<integer>`](#integers): Internal vertical padding at the top.
  - `padding-bottom` [`<integer>`](#integers): Internal vertical padding at the bottom.
  - `padding-left` [`<integer>`](#integers): Internal horizontal padding at the left.
  - `padding-right` [`<integer>`](#integers): Internal horizontal padding at the right.

---

## 2. Dimensions & Sizing

Used to control element widths and heights.

- `width` [`<length>`](#lengths): Constrains the width of the element.
- `height` [`<length>`](#lengths): Constrains the height of the element.

---

## 3. Borders

Borders are drawn using ASCII/Unicode styling characters around elements.

- `border` [`<border-style>`](#border-styles) | [`<integer>`](#integers): Shorthand to enable borders on all sides of the box. Specifying a `<border-style>` keyword defaults the border width to 1. Specifying an `<integer>` width defaults the style to `solid`.
  - `border-width` [`<integer>`](#integers): Shorthand to set the border width on all sides of the element.
  - `border-top` [`<integer>`](#integers): Enables or configures the top border width.
  - `border-bottom` [`<integer>`](#integers): Enables or configures the bottom border width.
  - `border-left` [`<integer>`](#integers): Enables or configures the left border width.
  - `border-right` [`<integer>`](#integers): Enables or configures the right border width.
  - `border-style` [`<border-style>`](#border-styles): Chooses the character set style of the border.
  - `border-color` [`<color>`](#colors): Shorthand for the color of all border sides.
    - `border-color-top` [`<color>`](#colors): Specifies color of the top border line.
    - `border-color-bottom` [`<color>`](#colors): Specifies color of the bottom border line.
    - `border-color-left` [`<color>`](#colors): Specifies color of the left border line.
    - `border-color-right` [`<color>`](#colors): Specifies color of the right border line.

---

## 4. Typography & Coloring

Color formatting for component text and backgrounds.

- `color` / `foreground-color` [`<color>`](#colors): Configures the foreground text color of the element.
- `background-color` [`<color>`](#colors): Configures the background color of cells within the element's box boundary.

<WasmTerminal src="/wasm/rtxui_example_colors.js" :cols="80" :rows="22" />

---

## 5. Layout Alignment (Flexbox)

RTXUI includes a subset of CSS Flexbox for horizontal and vertical layouts.

- `display` [`<display>`](#display-modes): Enables the flex layout engine. E.g., `display: flex` or `display: block flow`.
- `flex-direction` `row | column`: Layout axis for flex items.
- `flex-grow` [`<number>`](#numbers): Portion of free space assigned to the item along the main axis.

<WasmTerminal src="/wasm/rtxui_example_layout.js" :cols="80" :rows="16" />

---

## 6. Scrolling & Overflow

Enables viewport scrolling when children overflow parent boundaries.

- `overflow` [`<overflow>`](#overflow-modes): Shorthand to configure horizontal and vertical overflow behavior.
  - `overflow-x` [`<overflow>`](#overflow-modes): Horizontal overflow behavior (`visible`, `hidden`, `scroll`).
  - `overflow-y` [`<overflow>`](#overflow-modes): Vertical overflow behavior (`visible`, `hidden`, `scroll`).
- `scrollbar-width` [`<scrollbar-width>`](#scrollbar-widths): Controls scrollbar rendering footprint.
- `scroll-speed` [`<integer>`](#integers): Shorthand to configure scrolling step speed on mouse/keyboard inputs.
  - `scroll-speed-x` [`<integer>`](#integers): Step scroll distance horizontally on event triggers.
  - `scroll-speed-y` [`<integer>`](#integers): Step scroll distance vertically on event triggers.

<WasmTerminal src="/wasm/rtxui_example_horizontal_scroll.js" :cols="80" :rows="30" />

---

## Value Types Reference

This section details the formatting, syntax, and allowed values for the placeholders referenced above.

### Integers

An integer represents a whole number of characters/terminal cells.

- **Format**: A positive or negative whole number.
- **Examples**: `0`, `2`, `12`, `-1`

### Numbers

A standard floating-point or integer value.

- **Format**: A decimal or integer number.
- **Examples**: `1`, `2.5`, `0.5`

### Lengths

A length represents a layout dimension, which can be fixed in terminal cells or proportional to the parent container.

- **Format**:
  - Raw `<number>`: Interpreted as terminal cell count (e.g., `20`).
  - `<number>%`: Interpreted as a percentage of the parent's layout dimension (e.g., `50%`).

### Colors

Colors define foreground text, background cells, or border colors.

- **Format**:
  - **Hex Color Codes**: `#RGB`, `#RGBA`, `#RRGGBB`, or `#RRGGBBAA` (e.g., `#f0f`, `#ff3366`, `#00ff0088`).
  - **RGB Function**: `rgb(R, G, B)` where `R`, `G`, `B` are integers from `0` to `255` (e.g., `rgb(255, 128, 0)`).
  - **RGBA Function**: `rgba(R, G, B, A)` where `R`, `G`, `B` are integers from `0` to `255` and `A` is an opacity value from `0.0` to `1.0` (e.g., `rgba(0, 0, 255, 0.5)`).
  - **Color Keywords**:
    - `black`
    - `silver`
    - `gray` (or `grey`)
    - `white`
    - `maroon`
    - `red`
    - `purple`
    - `fuchsia` (or `magenta`)
    - `green`
    - `lime`
    - `olive`
    - `yellow`
    - `navy`
    - `blue`
    - `teal`
    - `aqua` (or `cyan`)

### Border Styles

RTXUI supports 24 different character sets for border drawing.

- `none`: No border line is painted.
- `solid`: A standard thin border line.
- `dashed`: Dashed Unicode lines.
- `dotted`: Dotted border line.
- `heavy` / `thick`: Bold Unicode borders.
- `double`: Double-line borders.
- `round` / `rounded`: Rounded corners.
- `wide`: Double-width horizontal lines.
- `tall`: Double-height vertical lines.
- `ascii`: Standard `+`, `-`, `|` characters.
- `blank`: Empty character placeholder border.
- `shadow` / `3d`: 3D shadow blocks.
- `shade-light`: Light shading block (`░`).
- `shade-medium`: Medium shading block (`▒`).
- `shade-dark`: Dark shading block (`▓`).
- `squiggle` / `wave`: Wavy border line.
- `double-horizontal`: Double lines horizontally, single vertical.
- `double-vertical`: Double lines vertically, single horizontal.
- `hkey`: Horizontal keycap style border.
- `vkey`: Vertical keycap style border.
- `inner`: Inner frame border style.
- `outer`: Outer frame border style.
- `panel`: Panel frame border style.

<WasmTerminal src="/wasm/rtxui_example_borders.js" :cols="80" :rows="50" />

### Overflow Modes

Defines viewport behavior when child elements exceed the parent container boundaries.

- **Format**:
  - `visible`: Content renders outside the container with no clipping.
  - `hidden`: Content is clipped to container bounds; scrolling is disabled.
  - `scroll` (or `auto`): Content is clipped; scrollbars are displayed if needed, and scrolling is interactive.

### Scrollbar Widths

Controls the visibility and size layout of scrollbars.

- **Format**:
  - `auto`: Visual scrollbar track is rendered, taking up 1 cell width/height.
  - `none`: Visual scrollbar is hidden, but the viewport remains scrollable.

### Display Modes

Controls the layout display formatting context of an element.

- **Format**:
  - `none`: The element and its descendants are not displayed and take up no layout space.
  - `block`: Behaves as a block container.
  - `inline`: Behaves as an inline container.
  - `flex`: Behaves as a flexible box container.
  - `<display-outside> <display-inside>`: A combination of behaviors, where `<display-outside>` is `block | inline` and `<display-inside>` is `flow | flex` (e.g., `block flow`).
