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
| `min-width` | [`<length>`](#value-types) | ❌ | Minimum layout width constraint. |
| `max-width` | [`<length>`](#value-types) | ❌ | Maximum layout width constraint. |
| `min-height` | [`<length>`](#value-types) | ❌ | Minimum layout height constraint. |
| `max-height` | [`<length>`](#value-types) | ❌ | Maximum layout height constraint. |

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
| `border-color-top` | [`<color>`](#colors) |   | Color of the top border line. |
| `border-color-bottom` | [`<color>`](#colors) |   | Color of the bottom border line. |
| `border-color-left` | [`<color>`](#colors) |   | Color of the left border line. |
| `border-color-right` | [`<color>`](#colors) |   | Color of the right border line. |

---

## 3. Typography and Coloring

| Property | Value Type | Animatable | Description |
| :--- | :--- | :---: | :--- |
| `color` | [`<color>`](#colors) |   | Foreground text character color. |
| `foreground-color` | [`<color>`](#colors) |   | Alias for `color`. |
| `background-color` | [`<color>`](#colors) |   | Background block container cell color. |
| `opacity` | [`<number>`](#value-types) |   | Transparency value (`0.0` for transparent to `1.0` for opaque). |
| `text-align` | `left \| right \| center` | ❌ | Horizontal alignment of inline text flows. |
| `white-space` | `normal \| nowrap` | ❌ | `nowrap` disables text wrapping. |
| `font-weight` | `bold \| normal` | ❌ | Applies bold styling to text. |
| `text-decoration` | `underline \| double-underline \| line-through \| strikethrough \| blink \| none` | ❌ | Text decorations (can specify space-separated lists). |
| `text-overflow` | `clip \| ellipsis` | ❌ | Behavior when text overflows its block container. |
| `visibility` | `visible \| hidden` | ❌ | Controls element visibility. |
| `cursor` | `default \| pointer` | ❌ | Determines mouse pointer styling when hovering. |

---

## 4. Flexbox Layout

| Property | Value Type | Animatable | Description |
| :--- | :--- | :---: | :--- |
| `display` | `none \| block \| inline \| flex` | ❌ | Enables the flex layout engine or hides elements. |
| `flex-direction` | `row \| column` | ❌ | Main formatting axis direction. |
| `flex-wrap` | `nowrap \| wrap \| wrap-reverse` | ❌ | Controls wrapping behavior of flex items. |
| `flex-grow` | [`<number>`](#value-types) |   | Portion of free space item claims along main axis. |
| `flex-shrink` | [`<number>`](#value-types) |   | Factor determining how much item shrinks. |
| `flex-basis` | [`<length>`](#value-types) | ❌ | Initial size of flex item before free space is distributed. |
| `flex` | shorthand | ❌ | Shorthand for flex-grow, flex-shrink, and flex-basis. |
| `align-items` | `stretch \| flex-start \| flex-end \| center \| baseline` | ❌ | Alignment of items along the cross axis. |
| `align-self` | `auto \| stretch \| flex-start \| flex-end \| center \| baseline` | ❌ | Alignment of individual flex item along the cross axis. |
| `align-content` | `stretch \| flex-start \| flex-end \| center \| space-between \| space-around \| space-evenly` | ❌ | Alignment of flex lines in multi-line flex container. |
| `justify-content` | `flex-start \| flex-end \| center \| space-between \| space-around \| space-evenly` | ❌ | Alignment of items along the main axis. |
| `gap` | [`<length>`](#value-types) | ❌ | Spacing between flex items. |
| `row-gap` | [`<length>`](#value-types) | ❌ | Spacing between flex rows/lines. |
| `column-gap` | [`<length>`](#value-types) | ❌ | Spacing between flex columns/items. |

---

## 5. Scrolling & Overflow

| Property | Value Type | Animatable | Description |
| :--- | :--- | :---: | :--- |
| `overflow` | [`<overflow>`](#overflow-modes) | ❌ | Shorthand to configure horizontal & vertical overflow. |
| `overflow-x` | [`<overflow>`](#overflow-modes) | ❌ | Horizontal layout overflow (`visible`, `hidden`, `scroll`). |
| `overflow-y` | [`<overflow>`](#overflow-modes) | ❌ | Vertical layout overflow (`visible`, `hidden`, `scroll`). |
| `scrollbar-width` | `auto \| none` | ❌ | `none` hides visual scrollbars while keeping list scrollable. |
| `scrollbar-color` | [`<color>`](#colors) [`<color>`](#colors) |   | Foreground (thumb) and background (track) colors of scrollbars. |
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
*   **Transformations**:
    *   `lighten(<amount>)`: Lightens the current resolved color (e.g., `lighten(10%)` or `lighten(0.1)`). If no color has been resolved for the element, falls back to a transparent white overlay (`rgba(255, 255, 255, amount)`).
    *   `darken(<amount>)`: Darkens the current resolved color (e.g., `darken(15%)` or `darken(0.15)`). If no color has been resolved, falls back to a transparent black overlay (`rgba(0, 0, 0, amount)`).
    *   `alpha(<amount>)`: Sets the alpha transparency of the current resolved color to the specified amount (e.g., `alpha(50%)` or `alpha(0.5)`).

### Border Styles
*   `none`, `solid`, `dashed`, `dotted`, `heavy`, `double`, `round`, `wide`, `tall`, `ascii`, `blank`, `shadow`, `shade-light` (`░`), `shade-medium` (`▒`), `shade-dark` (`▓`), `squiggle`, `double-horizontal`, `double-vertical`, `hkey`, `vkey`, `inner`, `outer`, `panel`.
