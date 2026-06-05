# CSS Attribute Reference

This reference lists all CSS-like styling properties supported by the RTXUI layout and painting engine, indicating whether each property supports smooth animations/transitions.

---

## 1. Spacing, Sizing, and Box Model

| Property | Value Type | Animatable | Description |
| :--- | :--- | :---: | :--- |
| `margin` | [`<integer>`](#value-types) | ❌ | Shorthand margin width on all sides. |
| `margin-top` | [`<integer>`](#value-types) | ❌ | Vertical space above the element. |
| `margin-bottom` | [`<integer>`](#value-types) | ❌ | Vertical space below the element. |
| `margin-left` | [`<integer>`](#value-types) | ❌ | Horizontal space to the left. |
| `margin-right` | [`<integer>`](#value-types) | ❌ | Horizontal space to the right. |
| `padding` | [`<integer>`](#value-types) | ❌ | Shorthand internal padding on all sides. |
| `padding-top` | [`<integer>`](#value-types) | ❌ | Internal vertical padding at the top. |
| `padding-bottom` | [`<integer>`](#value-types) | ❌ | Internal vertical padding at the bottom. |
| `padding-left` | [`<integer>`](#value-types) | ❌ | Internal horizontal padding at the left. |
| `padding-right` | [`<integer>`](#value-types) | ❌ | Internal horizontal padding at the right. |
| `width` | [`<length>`](#value-types) |   | Constrains element layout width. |
| `height` | [`<length>`](#value-types) |   | Constrains element layout height. |

---

## 2. Borders and Frames

| Property | Value Type | Animatable | Description |
| :--- | :--- | :---: | :--- |
| `border` | [`<border-style>`](#border-styles) \| [`<integer>`](#value-types) | ❌ | Shorthand to configure borders on all sides. |
| `border-width` | [`<integer>`](#value-types) | ❌ | Border frame cell thickness on all sides. |
| `border-top` | [`<integer>`](#value-types) | ❌ | Top border frame thickness. |
| `border-bottom` | [`<integer>`](#value-types) | ❌ | Bottom border frame thickness. |
| `border-left` | [`<integer>`](#value-types) | ❌ | Left border frame thickness. |
| `border-right` | [`<integer>`](#value-types) | ❌ | Right border frame thickness. |
| `border-style` | [`<border-style>`](#border-styles) | ❌ | Character set mapping style of the frame. |
| `border-color` | [`<color>`](#colors) |   | Color of all border frame lines. |
| `border-top-color` | [`<color>`](#colors) |   | Color of the top border line. |
| `border-bottom-color` | [`<color>`](#colors) |   | Color of the bottom border line. |
| `border-left-color` | [`<color>`](#colors) |   | Color of the left border line. |
| `border-right-color` | [`<color>`](#colors) |   | Color of the right border line. |

---

## 3. Typography and Coloring

| Property | Value Type | Animatable | Description |
| :--- | :--- | :---: | :--- |
| `color` | [`<color>`](#colors) |   | Foreground text character color. |
| `background-color` | [`<color>`](#colors) |   | Background block container cell color. |
| `opacity` | [`<number>`](#value-types) |   | Transparency value (`0.0` for transparent to `1.0` for opaque). |
| `text-align` | `left \| right \| center` | ❌ | Horizontal alignment of inline text flows. |
| `white-space` | `normal \| nowrap` | ❌ | `nowrap` disables text wrapping. |
| `font-weight` | `bold \| normal` | ❌ | Applies bold styling to text. |
| `text-decoration` | `underline \| double-underline \| line-through \| strikethrough \| blink \| none` | ❌ | Text decorations (can specify space-separated lists). |

---

## 4. Flexbox Layout

| Property | Value Type | Animatable | Description |
| :--- | :--- | :---: | :--- |
| `display` | `none \| block \| inline \| flex` | ❌ | Enables the flex layout engine or hides elements. |
| `flex-direction` | `row \| column` | ❌ | Main formatting axis direction. |
| `flex-grow` | [`<number>`](#value-types) |   | Portion of free space item claims along main axis. |
| `flex-shrink` | [`<number>`](#value-types) |   | Factor determining how much item shrinks. |

---

## 5. Scrolling & Overflow

| Property | Value Type | Animatable | Description |
| :--- | :--- | :---: | :--- |
| `overflow` | [`<overflow>`](#overflow-modes) | ❌ | Shorthand to configure horizontal & vertical overflow. |
| `overflow-x` | [`<overflow>`](#overflow-modes) | ❌ | Horizontal layout overflow (`visible`, `hidden`, `scroll`). |
| `overflow-y` | [`<overflow>`](#overflow-modes) | ❌ | Vertical layout overflow (`visible`, `hidden`, `scroll`). |
| `scrollbar-width` | `auto \| none` | ❌ | `none` hides visual scrollbars while keeping list scrollable. |
| `scroll-speed` | [`<integer>`](#value-types) | ❌ | Shorthand scroll step speed multiplier. |
| `scroll-speed-x` | [`<integer>`](#value-types) | ❌ | Horizontal scroll step distance. |
| `scroll-speed-y` | [`<integer>`](#value-types) | ❌ | Vertical scroll step distance. |
| `scroll-behavior` | `auto \| smooth` | ❌ | Smooth scrolling transitions configuration. |

---

## 6. Transitions and Positioning

| Property | Value Type | Animatable | Description |
| :--- | :--- | :---: | :--- |
| `transition` | `property duration timing-function` | ❌ | Shorthand (e.g. `transition: background-color 0.2s linear`). |
| `position` | `static \| relative \| absolute \| fixed` | ❌ | Selects positioning flow model. |
| `top` | [`<length>`](#value-types) | ❌ | Offset relative to top boundary. |
| `bottom` | [`<length>`](#value-types) | ❌ | Offset relative to bottom boundary. |
| `left` | [`<length>`](#length) | ❌ | Offset relative to left boundary. |
| `right` | [`<length>`](#length) | ❌ | Offset relative to right boundary. |
| `z-index` | [`<integer>`](#value-types) | ❌ | Determines rendering paint layers. |

---

## 7. List Styles

| Property | Value Type | Animatable | Description |
| :--- | :--- | :---: | :--- |
| `list-style-type` | `disc \| circle \| square \| decimal \| none` | ❌ | Sets the marker prefix style for list items (`• `, `○ `, `■ `, numbers, or none). |
| `list-style` | `disc \| circle \| square \| decimal \| none` | ❌ | Shorthand configuration for list styling. |

---

## Value Types Reference

### Core Types
*   **`<integer>`**: A whole number. E.g. `0`, `3`, `-2`.
*   **`<number>`**: A decimal float. E.g. `0.25`, `2.0`.
*   **`<length>`**: Dimension size. A raw number is character cells (e.g., `15`), whereas a percentage ends with `%` (e.g., `50%`).

### Colors
*   **Hex Codes**: `#RGB`, `#RGBA`, `#RRGGBB`, `#RRGGBBAA` (e.g., `#f0f`, `#00ff0088`).
*   **RGB/RGBA**: `rgb(R, G, B)` and `rgba(R, G, B, A)` where color channels range from 0-255, and Alpha ranges 0.0-1.0.
*   **Keywords**: `black`, `silver`, `gray`, `white`, `maroon`, `red`, `purple`, `fuchsia`, `green`, `lime`, `olive`, `yellow`, `navy`, `blue`, `teal`, `aqua`.

### Border Styles
*   `none`, `solid`, `dashed`, `dotted`, `heavy`, `double`, `round`, `wide`, `tall`, `ascii`, `blank`, `shadow`, `shade-light` (`░`), `shade-medium` (`▒`), `shade-dark` (`▓`), `squiggle`, `double-horizontal`, `double-vertical`, `hkey`, `vkey`, `inner`, `outer`, `panel`.
