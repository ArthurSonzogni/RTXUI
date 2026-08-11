# CSS Attribute Reference

This reference lists all CSS-like styling properties supported by the RTXUI layout and painting engine, indicating whether each property supports smooth animations/transitions.

---

## 1. Spacing, Sizing, and Box Model

<CssProperty name="margin" values="<integer>" shorthand description="Shorthand margin width on all sides." />
<CssProperty name="margin-top" values="<integer>" description="Vertical space above the element." />
<CssProperty name="margin-bottom" values="<integer>" description="Vertical space below the element." />
<CssProperty name="margin-left" values="<integer>" description="Horizontal space to the left." />
<CssProperty name="margin-right" values="<integer>" description="Horizontal space to the right." />
<CssProperty name="padding" values="<integer>" shorthand description="Shorthand internal padding on all sides." />
<CssProperty name="padding-top" values="<integer>" description="Internal vertical padding at the top." />
<CssProperty name="padding-bottom" values="<integer>" description="Internal vertical padding at the bottom." />
<CssProperty name="padding-left" values="<integer>" description="Internal horizontal padding at the left." />
<CssProperty name="padding-right" values="<integer>" description="Internal horizontal padding at the right." />
<CssProperty name="width" values="<length>" animatable description="Constrains element layout width." />
<CssProperty name="height" values="<length>" animatable description="Constrains element layout height." />
<CssProperty name="min-width" values="<length>" description="Minimum layout width constraint." />
<CssProperty name="max-width" values="<length>" description="Maximum layout width constraint." />
<CssProperty name="min-height" values="<length>" description="Minimum layout height constraint." />
<CssProperty name="max-height" values="<length>" description="Maximum layout height constraint." />
<CssProperty name="box-sizing" values="content-box | border-box" description="Whether width/height (and their min/max variants) describe the content box or the border box. Default is border-box, unlike web CSS." />

---

## 2. Borders and Frames

<CssProperty name="border" values="<border-style> | <integer>" shorthand description="Shorthand to configure borders on all sides." />
<CssProperty name="border-width" values="<integer>" description="Border frame cell thickness on all sides." />
<CssProperty name="border-top" values="<border-style> | <integer>" description="Top border frame thickness. A <border-style> keyword selects the frame character set and gives this side a thickness of 1." />
<CssProperty name="border-bottom" values="<border-style> | <integer>" description="Bottom border frame thickness. A <border-style> keyword selects the frame character set and gives this side a thickness of 1." />
<CssProperty name="border-left" values="<border-style> | <integer>" description="Left border frame thickness. A <border-style> keyword selects the frame character set and gives this side a thickness of 1." />
<CssProperty name="border-right" values="<border-style> | <integer>" description="Right border frame thickness. A <border-style> keyword selects the frame character set and gives this side a thickness of 1." />
<CssProperty name="border-style" values="<border-style>" description="Character set mapping style of the frame." />
<CssProperty name="border-color" values="<color>" animatable description="Color of all border frame lines." />
<CssProperty name="border-color-top" values="<color>" animatable description="Color of the top border line." />
<CssProperty name="border-color-bottom" values="<color>" animatable description="Color of the bottom border line." />
<CssProperty name="border-color-left" values="<color>" animatable description="Color of the left border line." />
<CssProperty name="border-color-right" values="<color>" animatable description="Color of the right border line." />

---

## 3. Typography and Coloring

<CssProperty name="color" values="<color>" animatable inherited description="Foreground text character color." />
<CssProperty name="foreground-color" values="<color>" animatable inherited description="Alias for color." />
<CssProperty name="background-color" values="<color>" animatable description="Background block container cell color." />
<CssProperty name="opacity" values="<number>" animatable description="Transparency value (0.0 for transparent to 1.0 for opaque)." />
<CssProperty name="text-align" values="left | right | center | justify" inherited description="Horizontal alignment of inline text flows. justify widens space runs on soft-wrapped lines (never the last line or lines ended by an explicit newline)." />
<CssProperty name="white-space" values="normal | nowrap | pre | pre-wrap | pre-line | break-spaces" inherited description="nowrap/pre disable wrapping; pre-line collapses space runs while honoring newlines. Note: this engine preserves interior whitespace and newlines in all modes, so normal, pre-wrap, and break-spaces behave alike." />
<CssProperty name="font-weight" values="bold | bolder | lighter | normal | <number>" inherited description="Text weight. bold/bolder/>=600 render bold; lighter/<=300 render with the terminal's dim attribute." />
<CssProperty name="font-style" values="italic | oblique | normal" inherited description="Applies italic styling to text (rendered with the terminal's italic attribute)." />
<CssProperty name="text-decoration" values="underline | double-underline | overline | line-through | strikethrough | blink | none" inherited description="Text decorations (can specify space-separated lists, e.g. `overline underline`). `overline` renders as SGR 53, which not every terminal implements; those that do not simply draw no line. `none` clears all of them." />
<CssProperty name="text-transform" values="uppercase | lowercase | capitalize | none" inherited description="Case transformation of text (ASCII letters; other characters pass through)." />
<CssProperty name="letter-spacing" values="<integer> | normal" inherited description="Blank cells inserted between characters. Whole cells only; negative values clamp to 0. Spaced words never wrap mid-word." />
<CssProperty name="line-height" values="<integer> | normal" inherited description="Minimum rows each line box occupies (whole rows; values below 1 clamp to 1). Tall inline content can still grow a line further." />
<CssProperty name="overflow-wrap" values="anywhere | break-word | normal" inherited description="How words longer than the line are handled. Unlike CSS the default is anywhere: break at the container edge. normal keeps the word intact and lets it overflow." />
<CssProperty name="word-wrap" values="anywhere | break-word | normal" inherited description="Legacy alias for overflow-wrap." />
<CssProperty name="word-break" values="normal | break-all" inherited description="break-all treats every character boundary as a break opportunity, wrapping as soon as a line is full instead of pushing an overflowing word whole to the next line (unlike overflow-wrap, a last-resort fallback for otherwise-unbreakable words)." />
<CssProperty name="text-overflow" values="clip | ellipsis" description="Behavior when text overflows its block container." />
<CssProperty name="visibility" values="visible | hidden" description="Controls element visibility. Hidden elements keep their layout size." />
<CssProperty name="cursor" values="default | pointer" description="Mouse pointer styling when hovering the element." />

### `calc()`

Length values (`width`, `height`, `top`/`right`/`bottom`/`left`, `min-`/`max-` sizes, `flex-basis`, grid tracks) accept `calc()` expressions mixing cell counts and percentages with `+`, `-`, `*`, `/` and parentheses:

```css
.sidebar { width: calc(100% - 20); }
.half { height: calc((100% - 1) / 2); }
```

Multiplication requires at least one plain-number operand and division a plain non-zero number divisor (as in CSS). Invalid expressions are treated as `auto`.

`min(a, b)`, `max(a, b)`, and `clamp(min, preferred, max)` are also supported; each argument is a calc-style expression (which may itself contain `calc()`):

```css
.content { width: min(100%, 60); }
.panel { height: clamp(5, 50%, 20); }
```

`min()`/`max()` take exactly two arguments. They may be nested inside each other and inside `calc()`, and scaled by plain numbers:

```css
.column { width: calc(min(100%, 60) - 4); }
.thumb { height: max(min(50%, 10), 3); }
```

One limitation: a single `calc()`-style expression may contain at most one basis-dependent `min()`/`max()`/`clamp()` term (adding two of them together is treated as invalid).

### `!important`

Appending `!important` to a declaration value makes it win over normal declarations from later rules and over normal inline styles:

```css
.warning { color: red !important; }
```

### Custom properties (`--*` / `var()`)

Declarations whose property name starts with `--` define **custom properties**. They inherit through the element tree and can be referenced in any declaration value with `var(--name)` or `var(--name, fallback)`:

```css
self {
  --accent: rgb(59, 130, 246);
}
.card {
  border-color: var(--accent);
  color: var(--undefined-color, white); /* fallback used */
}
```

A declaration referencing an undefined variable without a fallback is ignored. Fallbacks may nest further `var()` references. Custom properties declared inside pseudo-class rules (e.g. `#box:hover { --tone: ...; }`) apply to that element's own declarations; they do not propagate to descendants.

---

## 4. Flexbox Layout

<CssProperty name="display" values="none | block | inline | flex | grid" description="Enables the flex/grid layout engine or hides elements." />
<CssProperty name="flex-direction" values="row | column" description="Main formatting axis direction." />
<CssProperty name="flex-wrap" values="nowrap | wrap | wrap-reverse" description="Controls wrapping behavior of flex items." />
<CssProperty name="flex-grow" values="<number>" animatable description="Portion of free space item claims along main axis." />
<CssProperty name="flex-shrink" values="<number>" animatable description="Factor determining how much item shrinks." />
<CssProperty name="flex-basis" values="<length>" description="Initial size of flex item before free space is distributed." />
<CssProperty name="flex" values="shorthand" shorthand description="Shorthand for flex-grow, flex-shrink, and flex-basis." />
<CssProperty name="order" values="<integer>" description="Lays a flex item out earlier or later than its document position. May be negative; items sharing a value keep document order." />
<CssProperty name="justify-items" values="stretch | start | center | end" description="Inline-axis alignment of items inside their grid cell." />
<CssProperty name="justify-self" values="auto | stretch | start | center | end" description="Per-item override of justify-items." />
<CssProperty name="place-items" values="<align-items> <justify-items>?" shorthand description="Shorthand for align-items + justify-items." />
<CssProperty name="place-self" values="<align-self> <justify-self>?" shorthand description="Shorthand for align-self + justify-self." />
<CssProperty name="align-items" values="stretch | flex-start | flex-end | center | baseline" description="Alignment of items along the cross axis." />
<CssProperty name="align-self" values="auto | stretch | flex-start | flex-end | center | baseline" description="Alignment of individual flex item along the cross axis." />
<CssProperty name="align-content" values="stretch | flex-start (start) | flex-end (end) | center | space-between | space-around | space-evenly" description="Alignment of flex lines in multi-line flex container. `start` and `end` are accepted as aliases of `flex-start` and `flex-end`." />
<CssProperty name="justify-content" values="flex-start (start) | flex-end (end) | center | space-between | space-around | space-evenly" description="Alignment of items along the main axis. `start` and `end` are accepted as aliases of `flex-start` and `flex-end`." />
<CssProperty name="place-content" values="<align-content> <justify-content>?" shorthand description="Shorthand for align-content + justify-content. One value sets both axes; a value valid on only one axis (e.g. `stretch`) leaves the other unchanged." />
<CssProperty name="gap" values="<length>" shorthand description="Spacing between flex items." />
<CssProperty name="row-gap" values="<length>" description="Spacing between flex rows/lines." />
<CssProperty name="column-gap" values="<length>" description="Spacing between flex columns/items." />

---

## 5. Grid Layout

<CssProperty name="grid-template-columns" values="list of <length>" description="Defines the column tracks of the grid. Supports repeat(count, track_size)." />
<CssProperty name="grid-template-rows" values="list of <length>" description="Defines the row tracks of the grid. Supports repeat(count, track_size)." />
<CssProperty name="grid-template" values="shorthand" shorthand description="Shorthand for grid-template-rows and grid-template-columns (separated by /)." />
<CssProperty name="grid-column" values="span <integer> | <integer>" description="Sets the column span of the grid item." />
<CssProperty name="grid-column-end" values="span <integer> | <integer>" description="Alias for grid-column." />
<CssProperty name="grid-row" values="span <integer> | <integer>" description="Sets the row span of the grid item." />
<CssProperty name="grid-row-end" values="span <integer> | <integer>" description="Alias for grid-row." />
<CssProperty name="grid-gap" values="<length>" shorthand description="Alias for gap." />
<CssProperty name="grid-row-gap" values="<length>" description="Alias for row-gap." />
<CssProperty name="grid-column-gap" values="<length>" description="Alias for column-gap." />

---

## 6. Scrolling & Overflow

<CssProperty name="overflow" values="<overflow>" shorthand description="Shorthand to configure horizontal & vertical overflow." />
<CssProperty name="overflow-x" values="<overflow>" description="Horizontal layout overflow (visible, hidden, scroll)." />
<CssProperty name="overflow-y" values="<overflow>" description="Vertical layout overflow (visible, hidden, scroll)." />
<CssProperty name="scrollbar-width" values="auto | none" description="none hides visual scrollbars while keeping list scrollable." />
<CssProperty name="scrollbar-color" values="<color> <color>" animatable description="Foreground (thumb) and background (track) colors of scrollbars." />
<CssProperty name="scroll-speed" values="<integer>" description="Shorthand scroll step speed multiplier." />
<CssProperty name="scroll-speed-x" values="<integer>" description="Horizontal scroll step distance." />
<CssProperty name="scroll-speed-y" values="<integer>" description="Vertical scroll step distance." />
<CssProperty name="scroll-behavior" values="auto | smooth" description="Smooth scrolling transitions configuration." />

---

## 7. Transitions and Positioning

<CssProperty name="transition" values="property duration timing-function" shorthand description="Shorthand (e.g. transition: background-color 0.2s linear)." />
<CssProperty name="position" values="static | relative | absolute | fixed | sticky" description="Selects positioning flow model." />
<CssProperty name="inset" values="1-4 <length> values" shorthand description="Shorthand setting top/right/bottom/left (same expansion as margin)." />
<CssProperty name="aspect-ratio" values="<w> / <h> | <number> | auto" description="Derives an element's auto dimension from whichever of width/height is definite: height-from-width in block, flex, and grid contexts (items and containers), and width-from-height in block contexts and on flex items/containers (grid containers only derive height from width). Ratios are in cells — terminal cells are ~2:1 tall, so 2 / 1 looks square. Content larger than the ratio overflows." />
<CssProperty name="top" values="<length>" description="Offset relative to top boundary." />
<CssProperty name="bottom" values="<length>" description="Offset relative to bottom boundary." />
<CssProperty name="left" values="<length>" description="Offset relative to left boundary." />
<CssProperty name="right" values="<length>" description="Offset relative to right boundary." />
<CssProperty name="z-index" values="<integer>" description="Determines rendering paint layers." />

---

## 8. List Styles

<CssProperty name="list-style-type" values="disc | circle | square | decimal | none" description="Sets the marker prefix style for list items (• , ○ , ■ , numbers, or none)." />
<CssProperty name="list-style" values="disc | circle | square | decimal | none" shorthand description="Shorthand configuration for list styling." />

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
    *   `lighten(<amount>)`: Mixes `amount` of white into the current resolved color (e.g., `lighten(10%)` or `lighten(0.1)`), equivalent to CSS `color-mix(in srgb, white <amount>, <current>)`. Each channel moves that fraction of its remaining distance to 255, so a bright color moves less than a dark one and the result cannot clip. If no color has been resolved for the element, the mix is deferred to paint time as a white overlay at `amount` alpha, which composites to the same result over whatever ends up behind.
    *   `darken(<amount>)`: The same, mixing toward black.
    *   `alpha(<amount>)`: Sets the alpha transparency of the current resolved color to the specified amount (e.g., `alpha(50%)` or `alpha(0.5)`).

### Border Styles

Values for `border` and `border-style`. Each draws the one-cell ring around the
element; `border-color` sets the color the glyphs are drawn in (defaulting to
the element's `color`).

```
  ascii      blank      dashed     double
  +-------+             ┏╍╍╍╍╍╍╍┓  ╔═══════╗
  |       |             ╏       ╏  ║       ║
  +-------+             ┗╍╍╍╍╍╍╍┛  ╚═══════╝

  hkey       heavy      inner      none
  ▔▔▔▔▔▔▔▔▔  ┏━━━━━━━┓  ▗▄▄▄▄▄▄▄▖
             ┃       ┃  ▐       ▌
  ▁▁▁▁▁▁▁▁▁  ┗━━━━━━━┛  ▝▀▀▀▀▀▀▀▘

  outer      panel      round      solid
  ▛▀▀▀▀▀▀▀▜  ▊███████▎  ╭───────╮  ┌───────┐
  ▌       ▐  ▊       ▎  │       │  │       │
  ▙▄▄▄▄▄▄▄▟  ▊▁▁▁▁▁▁▁▎  ╰───────╯  └───────┘

  tall       thick      vkey       wide
  ▊▔▔▔▔▔▔▔▎  █▀▀▀▀▀▀▀█  ▏       ▕  ▁▁▁▁▁▁▁▁▁
  ▊       ▎  █       █  ▏       ▕  ▎       ▊
  ▊▁▁▁▁▁▁▁▎  █▄▄▄▄▄▄▄█  ▏       ▕  ▔▔▔▔▔▔▔▔▔

  dotted     double-horizontal  double-vertical  shadow
  ·········  ╒═══════╕          ╓───────╖        ░░░░░░░░▓
  ·       ·  │       │          ║       ║        ░       ▓
  ·········  ╘═══════╛          ╙───────╜        ░▓▓▓▓▓▓▓▓

  shade-light  shade-medium  shade-dark  squiggle
  ░░░░░░░░░    ▒▒▒▒▒▒▒▒▒     ▓▓▓▓▓▓▓▓▓   ~~~~~~~~~
  ░       ░    ▒       ▒     ▓       ▓   ~       ~
  ░░░░░░░░░    ▒▒▒▒▒▒▒▒▒     ▓▓▓▓▓▓▓▓▓   ~~~~~~~~~

  block      tab
  ▄▄▄▄▄▄▄▄▄  ▁▁▁▁▁▁▁▁▁
  █       █  ▎       ▊
  ▀▀▀▀▀▀▀▀▀  ▔▔▔▔▔▔▔▔▔
```

| Keyword | Alias | Notes |
| --- | --- | --- |
| `none` | | No ring, and no cell reserved for one. |
| `blank` | | Reserves the ring but draws nothing, so the element's own background shows through it. |
| `ascii` | | `+ - \|` only — for terminals or fonts without box-drawing glyphs. |
| `solid` | | Light box-drawing lines. |
| `round` | `rounded` | `solid` with rounded corners. |
| `heavy` | | Heavy box-drawing lines. |
| `double` | | Double lines on all four sides. |
| `double-horizontal` | | Double lines top and bottom, single at the sides. |
| `double-vertical` | | Double lines at the sides, single top and bottom. |
| `dashed` | | Dashed heavy lines. |
| `dotted` | | A ring of `·`. |
| `squiggle` | `wave` | A ring of `~`. |
| `thick` | | Full blocks at the sides, half blocks top and bottom. |
| `outer` | | Half-cell bars hugging the outside of the ring. |
| `inner` | | Half-cell bars hugging the inside of the ring. |
| `tall` | | Quarter-cell bars at the sides, eighth-cell lines top and bottom. |
| `wide` | | Quarter-cell bars at the sides, eighth-cell lines inset top and bottom. |
| `panel` | | `tall` with a solid bar across the top, for a title bar. |
| `hkey` | | Eighth-cell lines top and bottom only; no sides. |
| `vkey` | | Eighth-cell lines at the sides only; no top or bottom. |
| `shade-light` | | A ring of `░`. |
| `shade-medium` | | A ring of `▒`. |
| `shade-dark` | | A ring of `▓`. |
| `shadow` | `3d` | Light on the top and left, dark on the bottom and right, for a raised look. |
| `block` | | Full blocks at the sides, half blocks top and bottom, drawn on the parent's background. |
| `tab` | | Identical to `wide`; the name Textual uses when the border is a tab strip. |
| `hidden` | | Accepted as a synonym of `none`, matching Textual. |

The partial-block styles (`tall`, `panel`, `wide`, `block`, `tab`, `inner`,
`outer`, `thick`) draw a glyph that sits astride the boundary between the
element and its parent, so each border cell has to pick which of the two
backgrounds fills the part the glyph does not cover. RTXUI follows Textual's
table exactly: `inner`, and the sides of `tall` and `panel`, sit on the
*parent's* background, which is what makes them read as drawn outside the
element; the top and bottom of `tall` and `panel` sit on the element's own.

Some cells are drawn in reverse video (`ESC[7m`) so the strip lands on the far
side of the cell — Unicode has left-side partial blocks in every width but only
one right-side one. The swap is left to the terminal rather than done by
exchanging the two colors, so that an element with no background of its own
still resolves against the terminal's default *background* rather than its
default foreground.

Single sides can be set independently with `border-top`, `border-right`,
`border-bottom` and `border-left`; a lone side renders as a plain line rather
than a partial box.
