# RTXUI CSS Attribute Reference

This reference catalog lists all CSS-like styling properties supported by the RTXUI layout and painting engine.

---

## 1. Box Model (Margin & Padding)

Standard spacing properties to control layout positioning.

- `margin`: Shorthand for all margins around the element.
- `margin-top`: Vertical space above the element. Sibling block margins collapse according to standard block flow rules.
- `margin-bottom`: Vertical space below the element.
- `margin-left`: Horizontal space to the left of the element.
- `margin-right`: Horizontal space to the right of the element.

- `padding`: Shorthand for padding inside the element boundary.
- `padding-top`: Internal vertical padding at the top.
- `padding-bottom`: Internal vertical padding at the bottom.
- `padding-left`: Internal horizontal padding at the left.
- `padding-right`: Internal horizontal padding at the right.

---

## 2. Dimensions & Sizing

Used to control element widths and heights. Sizing supports raw numbers (cells) or percentages (e.g. `50%`).

- `width`: Constrains the width of the element.
- `height`: Constrains the height of the element.

---

## 3. Borders

Borders are drawn using ASCII/Unicode styling characters based on the specified border style and color.

- `border`: Shorthand to enable borders on all sides of the box. E.g., `border: wide`.
- `border-width`: Specifying a border line width (usually `1` cell).
- `border-top`: Enables or configures the top border line.
- `border-bottom`: Enables or configures the bottom border line.
- `border-left`: Enables or configures the left border line.
- `border-right`: Enables or configures the right border line.
- `border-style`: Chooses the character set style of the border:
  - `none`: No border line is painted.
  - `solid`: A standard thin border line.
  - `dashed`: Dashed unicode lines.
  - `dotted`: Dotted border line.
  - `heavy` / `thick`: Bold unicode borders.
  - `double`: Double-line borders.
  - `round`: Rounded corners.
  - `wide`: Double-width horizontal lines.
  - `tall`: Double-height vertical lines.
  - `ascii`: Standard `+`, `-`, `|` characters.
  - `blank`: Empty character placeholder border.
  - `shadow` / `shade-light` / `shade-medium` / `shade-dark`: Shading blocks.
  - `squiggle`: Wave/squiggle border line.
  - `double-horizontal` / `double-vertical` / `hkey` / `vkey` / `inner` / `outer` / `panel`: Keyed and custom panel borders.
- `border-color`: Shorthand for color of all border sides. Supports `rgb(...)`, `rgba(...)`, or standard hex/keyword declarations.
- `border-color-top`: Specifies color of the top border line.
- `border-color-bottom`: Specifies color of the bottom border line.
- `border-color-left`: Specifies color of the left border line.
- `border-color-right`: Specifies color of the right border line.

<WasmTerminal src="/wasm/rtxui_example_borders.js" :cols="80" :rows="50" />

---

## 4. Typography & Coloring

Color formatting for component rendering.

- `color` / `foreground-color`: Configures the foreground character text color of the element (e.g., `rgb(255, 255, 255)`).
- `background-color`: Configures the background color of cells within the element's box boundary.

---

## 5. Layout Alignment (Flexbox)

RTXUI includes a subset of CSS Flexbox for robust horizontal and vertical layouts.

- `display`: Enables the flex layout engine. E.g., `display: flex` or `display: block flow`.
- `flex-direction`: Layout axis for flex items (`row` or `column`).
- `flex-grow`: Portion of free space assigned to the item along the main axis.

<WasmTerminal src="/wasm/rtxui_example_layout.js" :cols="80" :rows="16" />

---

## 6. Scrolling & Overflow

Enables viewport scrolling when children overflow parent boundaries.

- `overflow`: Shorthand to configure horizontal and vertical overflow. E.g., `overflow: scroll`.
- `overflow-x`: Horizontal overflow behavior (`visible`, `hidden`, `scroll`).
- `overflow-y`: Vertical overflow behavior (`visible`, `hidden`, `scroll`).
- `scrollbar-width`: Controls scrollbar rendering footprint.
  - `auto`: Visual scrollbars are painted inside the box.
  - `none`: Hide scrollbars but keep scrolling interactive.
- `scroll-speed`: Shorthand to configure scrolling step speed on mouse/keyboard inputs.
- `scroll-speed-x`: Step scroll distance horizontally on event triggers (e.g. `scroll-speed-x: 3`).
- `scroll-speed-y`: Step scroll distance vertically on event triggers.

<WasmTerminal src="/wasm/rtxui_example_horizontal_scroll.js" :cols="80" :rows="30" />

