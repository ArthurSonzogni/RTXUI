# CSS Engine & Attribute Specification

This document provides the formal specification of the CSS styling engine implemented in RTXUI, defining value grammar, cascade and specificity rules, box-model semantics, and the exhaustive property reference.

---

## 1. Engine Formal Grammar

RTXUI implements an inline stylesheet parser and property resolver operating directly over terminal character cells.

```ebnf
Declaration      ::= PropertyName ':' Value ('!important')? ';'
PropertyName     ::= [a-z-]+ | '--' [a-z0-9-]+
Value            ::= Token (Whitespace Token)*

Length           ::= Integer | Percentage | Fractional | 'auto' | CalcExpr | MinMaxExpr
Integer          ::= [-+]? [0-9]+
Percentage       ::= [-+]? [0-9]+ ('.' [0-9]+)? '%'
Fractional       ::= [0-9]+ ('.' [0-9]+)? 'fr'
Color            ::= HexColor | RgbColor | HslColor | ColorKeyword | ColorTransform

HexColor         ::= '#' ([0-9a-fA-F]{3} | [0-9a-fA-F]{4} | [0-9a-fA-F]{6} | [0-9a-fA-F]{8})
RgbColor         ::= ('rgb(' | 'rgba(') Number ',' Number ',' Number (',' Alpha)? ')'
                   | ('rgb(' | 'rgba(') Number Number Number ('/' Alpha)? ')'
HslColor         ::= ('hsl(' | 'hsla(') Angle (',' | Whitespace) Percentage (',' | Whitespace) Percentage (('/' | ',') Alpha)? ')'
ColorTransform   ::= ('lighten(' | 'darken(' | 'alpha(') (Percentage | Number) ')'
ColorKeyword     ::= 'transparent' | 'orange' | 'red' | 'white' | 'blue' | 'yellow' | 'green' | 'lime' | 'black' | 'gray' | 'grey' | 'cyan' | 'aqua' | 'magenta' | 'fuchsia' | 'silver' | 'maroon' | 'purple' | 'olive' | 'navy' | 'teal'
```

### Coordinate & Dimensional Model
- **Grid Unit**: 1 unit = 1 terminal character cell.
- **Physical Cell Aspect Ratio**: Terminal cells are approximately 1:2 (width:height). An aspect ratio of `2 / 1` produces an approximately square visual block.
- **Default Box Model**: Unlike web browsers which default to `content-box`, RTXUI defaults to `border-box` (`box-sizing: border-box`). Declared `width` and `height` define the outer dimensions including padding and borders.

---

## 2. Cascade, Inheritance & Specificity

### 2.1 Specificity Vector `(A, B, C)`
Selector precedence is determined by a 3-component specificity vector `(A, B, C)`:
1. **$A$ (ID Component)**: Count of ID selectors (`#id`).
2. **$B$ (Class & Pseudo Component)**: Count of class selectors (`.class`), attribute selectors (`[attr]`), and pseudo-classes (`:hover`, `:focus`, `:active`, `:checked`, `:disabled`, `:first-child`, etc.).
3. **$C$ (Type Component)**: Count of element type selectors (`div`, `button`, `span`).

*Note*: Pseudo-elements (`::part(...)`) participate in target isolation rather than global specificity weighting. Negation (`:not(X)`) contributes the specificity of its inner argument $X$.

### 2.2 Precedence Order (Descending)
1. Declarations with `!important` (resolved in reverse cascade order).
2. Inline element styles (`style="..."`).
3. Higher specificity vector `(A, B, C)` (evaluated lexicographically: $A_1 > A_2$, then $B_1 > B_2$, then $C_1 > C_2$).
4. Source declaration order (later rules override earlier rules).
5. Inherited values from parent elements (only properties marked as `inherited`).
6. Engine initial property defaults.

---

## 3. Spacing, Sizing, and Box Model

<CssProperty name="margin" values="<length> | auto" shorthand description="Initial: `0`. Shorthand margin width on all sides." />
<CssProperty name="margin-top" values="<length> | auto" description="Initial: `0`. Vertical space above the element." />
<CssProperty name="margin-bottom" values="<length> | auto" description="Initial: `0`. Vertical space below the element." />
<CssProperty name="margin-left" values="<length> | auto" description="Initial: `0`. Horizontal space to the left." />
<CssProperty name="margin-right" values="<length> | auto" description="Initial: `0`. Horizontal space to the right." />
<CssProperty name="padding" values="<integer>" shorthand description="Initial: `0`. Shorthand internal padding on all sides." />
<CssProperty name="padding-top" values="<integer>" description="Initial: `0`. Internal vertical padding at the top." />
<CssProperty name="padding-bottom" values="<integer>" description="Initial: `0`. Internal vertical padding at the bottom." />
<CssProperty name="padding-left" values="<integer>" description="Initial: `0`. Internal horizontal padding at the left." />
<CssProperty name="padding-right" values="<integer>" description="Initial: `0`. Internal horizontal padding at the right." />
<CssProperty name="width" values="<length>" animatable description="Initial: `auto`. Constrains element layout width." />
<CssProperty name="height" values="<length>" animatable description="Initial: `auto`. Constrains element layout height." />
<CssProperty name="min-width" values="<length>" description="Initial: `auto`. Minimum layout width constraint." />
<CssProperty name="max-width" values="<length>" description="Initial: `none`. Maximum layout width constraint." />
<CssProperty name="min-height" values="<length>" description="Initial: `auto`. Minimum layout height constraint." />
<CssProperty name="max-height" values="<length>" description="Initial: `none`. Maximum layout height constraint." />
<CssProperty name="box-sizing" values="content-box | border-box" description="Initial: `border-box`. Whether width/height (and their min/max variants) describe the content box or the border box. Default is border-box, unlike web CSS." />

### `calc()` Expressions
Length properties accept `calc()` expressions combining discrete cells and percentages:
```css
.sidebar { width: calc(100% - 20); }
.split   { height: calc((100% - 1) / 2); }
```

### `min()`, `max()`, and `clamp()`
- `min(a, b)`: Evaluates to the smaller resolved length.
- `max(a, b)`: Evaluates to the larger resolved length.
- `clamp(min, preferred, max)`: Clamps `preferred` between lower and upper bounds.
```css
.responsive-box { width: clamp(20, 50%, 80); }
```

---

## 4. Borders and Frames

<CssProperty name="border" values="<border-style> | <integer>" shorthand description="Initial: `none`. Shorthand to configure borders on all sides." />
<CssProperty name="border-width" values="<integer>" description="Initial: `0`. Border frame cell thickness on all sides." />
<CssProperty name="border-top" values="<border-style> | <integer>" description="Initial: `0`. Top border frame thickness. A <border-style> keyword selects the frame character set and gives this side a thickness of 1." />
<CssProperty name="border-bottom" values="<border-style> | <integer>" description="Initial: `0`. Bottom border frame thickness. A <border-style> keyword selects the frame character set and gives this side a thickness of 1." />
<CssProperty name="border-left" values="<border-style> | <integer>" description="Initial: `0`. Left border frame thickness. A <border-style> keyword selects the frame character set and gives this side a thickness of 1." />
<CssProperty name="border-right" values="<border-style> | <integer>" description="Initial: `0`. Right border frame thickness. A <border-style> keyword selects the frame character set and gives this side a thickness of 1." />
<CssProperty name="border-style" values="<border-style>" description="Initial: `none`. Character set mapping style of the frame." />
<CssProperty name="border-color" values="<color>" animatable description="Initial: `currentcolor`. Color of all border frame lines." />
<CssProperty name="border-color-top" values="<color>" animatable description="Initial: `currentcolor`. Color of the top border line." />
<CssProperty name="border-color-bottom" values="<color>" animatable description="Initial: `currentcolor`. Color of the bottom border line." />
<CssProperty name="border-color-left" values="<color>" animatable description="Initial: `currentcolor`. Color of the left border line." />
<CssProperty name="border-color-right" values="<color>" animatable description="Initial: `currentcolor`. Color of the right border line." />

### 4.1 Border Style Glyph Matrix
RTXUI supports 30 border style keywords mapping directly to Unicode box-drawing and block elements:

| Style Keyword | Canonical Alias | Visual Glyphs / Behavior Description |
| :--- | :--- | :--- |
| `none` | `hidden` | No border rendered; 0 cells allocated. |
| `blank` | — | Reserves 1 cell perimeter without drawing lines (background passes through). |
| `ascii` | — | Plain ASCII fallback: `+`, `-`, `\|`. |
| `solid` | — | Standard light box-drawing lines: `┌`, `─`, `┐`, `│`, `└`, `┘`. |
| `round` | `rounded` | Curved corner box-drawing lines: `╭`, `─`, `╮`, `│`, `╰`, `╯`. |
| `heavy` | `thick` | Bold / heavy stroke lines: `┏`, `━`, `┓`, `┃`, `┗`, `┛`. |
| `double` | — | Double parallel lines: `╔`, `═`, `╗`, `║`, `╚`, `╝`. |
| `double-horizontal` | — | Double lines on horizontal axes; single line on vertical axes. |
| `double-vertical` | — | Double lines on vertical axes; single line on horizontal axes. |
| `dashed` | — | Heavy dashed stroke lines (`╌`, `╎`). |
| `dotted` | — | Centered dot perimeter (`·`). |
| `squiggle` | `wave` | Wavy perimeter line (`~`). |
| `outer` | — | Half-cell block hugging outer cell edge (`▀`, `▄`, `▌`, `▐`). |
| `inner` | — | Half-cell block hugging inner cell edge. |
| `tall` | — | Quarter-block vertical bars; eighth-block horizontal lines. |
| `wide` | `tab` | Quarter-block vertical bars with inset horizontal lines. |
| `panel` | — | Tall border structure featuring a solid top bar for title integration. |
| `hkey` | — | Horizontal-only boundary lines on top and bottom. |
| `vkey` | — | Vertical-only boundary lines on left and right. |
| `shade-light` | — | 25% stippled block fill (`░`). |
| `shade-medium` | — | 50% stippled block fill (`▒`). |
| `shade-dark` | — | 75% stippled block fill (`▓`). |
| `shadow` | `3d` | Light shading top/left; dark shading bottom/right (depth effect). |
| `block` | — | Full block vertical columns (`█`) with half-block horizontals. |

<ExampleTabs src="/wasm/rtxui_example_borders.js">
<template #source>

<<< @/../example/borders.cpp

</template>
</ExampleTabs>

---

## 5. Typography and Coloring

<CssProperty name="color" values="<color>" animatable inherited description="Initial: `white`. Foreground text character color." />
<CssProperty name="foreground-color" values="<color>" animatable inherited description="Initial: `white`. Alias for color." />
<CssProperty name="background-color" values="<color>" animatable description="Initial: `transparent`. Background block container cell color." />
<CssProperty name="opacity" values="<number>" animatable description="Initial: `1.0`. Transparency value (0.0 for transparent to 1.0 for opaque)." />
<CssProperty name="text-align" values="left | right | center | justify" inherited description="Initial: `left`. Horizontal alignment of inline text flows. justify widens space runs on soft-wrapped lines (never the last line or lines ended by an explicit newline)." />
<CssProperty name="white-space" values="normal | nowrap | pre | pre-wrap | pre-line | break-spaces" inherited description="Initial: `normal`. nowrap/pre disable wrapping; pre-line collapses space runs while honoring newlines. Note: this engine preserves interior whitespace and newlines in all modes, so normal, pre-wrap, and break-spaces behave alike." />
<CssProperty name="font-weight" values="bold | bolder | lighter | normal | <number>" inherited description="Initial: `normal`. Text weight. bold/bolder/>=600 render bold; lighter/<=300 render with the terminal's dim attribute." />
<CssProperty name="font-style" values="italic | oblique | normal" inherited description="Initial: `normal`. Applies italic styling to text (rendered with the terminal's italic attribute)." />
<CssProperty name="text-decoration" values="underline | double-underline | overline | line-through | strikethrough | blink | none" inherited description="Initial: `none`. Text decorations (can specify space-separated lists, e.g. `overline underline`). `overline` renders as SGR 53, which not every terminal implements; those that do not simply draw no line. `none` clears all of them." />
<CssProperty name="text-transform" values="uppercase | lowercase | capitalize | none" inherited description="Initial: `none`. Case transformation of text (ASCII letters; other characters pass through)." />
<CssProperty name="letter-spacing" values="<integer> | normal" inherited description="Initial: `normal`. Blank cells inserted between characters. Whole cells only; negative values clamp to 0. Spaced words never wrap mid-word." />
<CssProperty name="tab-size" values="<integer>" inherited description="Initial: `8`. Cells between tab stops; 8 by default. A tab advances to the next multiple of this from the start of its line, counted in cells so a full-width glyph moves the stop by two. Tabs are expanded during layout and never handed to the terminal, so the engine's own column accounting is what decides where they land. Outside white-space: pre and pre-wrap a tab is collapsible whitespace and becomes a single space, as in CSS. 0 removes tabs entirely." />
<CssProperty name="line-height" values="<integer> | normal" inherited description="Initial: `normal`. Minimum rows each line box occupies (whole rows; values below 1 clamp to 1). Tall inline content can still grow a line further." />
<CssProperty name="overflow-wrap" values="anywhere | break-word | normal" inherited description="Initial: `anywhere`. How words longer than the line are handled. Unlike CSS the default is anywhere: break at the container edge. normal keeps the word intact and lets it overflow." />
<CssProperty name="word-wrap" values="anywhere | break-word | normal" inherited description="Initial: `anywhere`. Legacy alias for overflow-wrap." />
<CssProperty name="word-break" values="normal | break-all" inherited description="Initial: `normal`. break-all treats every character boundary as a break opportunity, wrapping immediately at the boundary of a full line instead of pushing an overflowing word whole to the next line (unlike overflow-wrap, a last-resort fallback for otherwise-unbreakable words)." />
<CssProperty name="text-overflow" values="clip | ellipsis" description="Initial: `clip`. Behavior when text overflows its block container." />
<CssProperty name="visibility" values="visible | hidden" description="Initial: `visible`. Controls element visibility. Hidden elements keep their layout size." />
<CssProperty name="cursor" values="default | pointer" description="Initial: `default`. Mouse pointer styling when hovering the element." />

### Color Transforms
- `lighten(amount)`: Interpolates toward `#ffffff` in sRGB space.
- `darken(amount)`: Interpolates toward `#000000` in sRGB space.
- `alpha(amount)`: Sets the alpha transparency channel to `amount`.

---

## 6. Custom Properties (`--*` and `var()`)

Properties with a `--` prefix declare custom variables. They inherit down the element hierarchy and resolve during style application.
```css
self {
  --primary-color: rgb(59, 130, 246);
}
.header {
  color: var(--primary-color, white);
}
```
- **Fallback Semantics**: If a variable is missing, `var(--name, fallback)` evaluates the fallback expression. Fallbacks may contain nested `var()` calls.
- **Substitution Limit**: Recursive resolution is capped at 128 expansions to prevent cyclic reference hangs.

---

## 7. Flexbox Layout

<CssProperty name="display" values="none | block | inline | flex | grid | inline-block | inline-flex | inline-grid | flow-root" description="Initial: `inline`. Enables the flex/grid layout engine or hides elements." />
<CssProperty name="flex-direction" values="row | column | row-reverse | column-reverse" description="Initial: `row`. Main formatting axis direction." />
<CssProperty name="flex-wrap" values="nowrap | wrap | wrap-reverse" description="Initial: `nowrap`. Controls wrapping behavior of flex items." />
<CssProperty name="flex-grow" values="<number>" animatable description="Initial: `0`. Portion of free space item claims along main axis." />
<CssProperty name="flex-shrink" values="<number>" animatable description="Initial: `1`. Factor determining how much item shrinks." />
<CssProperty name="flex-basis" values="<length>" description="Initial: `auto`. Initial size of flex item before free space is distributed." />
<CssProperty name="flex" values="shorthand" shorthand description="Initial: `0 1 auto`. Shorthand for flex-grow, flex-shrink, and flex-basis." />
<CssProperty name="order" values="<integer>" description="Initial: `0`. Lays a flex item out earlier or later than its document position. May be negative; items sharing a value keep document order." />
<CssProperty name="justify-items" values="stretch | start | center | end" description="Initial: `stretch`. Inline-axis alignment of items inside their grid cell." />
<CssProperty name="justify-self" values="auto | stretch | start | center | end" description="Initial: `auto`. Per-item override of justify-items." />
<CssProperty name="place-items" values="<align-items> <justify-items>?" shorthand description="Initial: `stretch`. Shorthand for align-items + justify-items." />
<CssProperty name="place-self" values="<align-self> <justify-self>?" shorthand description="Initial: `auto`. Shorthand for align-self + justify-self." />
<CssProperty name="align-items" values="stretch | flex-start | flex-end | center | baseline" description="Initial: `stretch`. Alignment of items along the cross axis." />
<CssProperty name="align-self" values="auto | stretch | flex-start | flex-end | center | baseline" description="Initial: `auto`. Alignment of individual flex item along the cross axis." />
<CssProperty name="align-content" values="stretch | flex-start (start) | flex-end (end) | center | space-between | space-around | space-evenly" description="Initial: `stretch`. Alignment of flex lines in multi-line flex container. `start` and `end` are accepted as aliases of `flex-start` and `flex-end`." />
<CssProperty name="justify-content" values="flex-start (start) | flex-end (end) | center | space-between | space-around | space-evenly" description="Initial: `flex-start`. Alignment of items along the main axis. `start` and `end` are accepted as aliases of `flex-start` and `flex-end`." />
<CssProperty name="place-content" values="<align-content> <justify-content>?" shorthand description="Initial: `stretch flex-start`. Shorthand for align-content + justify-content. One value sets both axes; a value valid on only one axis (e.g. `stretch`) leaves the other unchanged." />
<CssProperty name="gap" values="<length>" shorthand description="Initial: `0`. Spacing between flex items." />
<CssProperty name="row-gap" values="<length>" description="Initial: `0`. Spacing between flex rows/lines." />
<CssProperty name="column-gap" values="<length>" description="Initial: `0`. Spacing between flex columns/items." />

---

## 8. Grid Layout

<CssProperty name="grid-template-columns" values="list of <length>" description="Initial: `none`. Defines the column tracks of the grid. Supports repeat(count, track_size)." />
<CssProperty name="grid-template-rows" values="list of <length>" description="Initial: `none`. Defines the row tracks of the grid. Supports repeat(count, track_size)." />
<CssProperty name="grid-template" values="shorthand" shorthand description="Initial: `none`. Shorthand for grid-template-rows and grid-template-columns (separated by /)." />
<CssProperty name="grid-column" values="span <integer> | <integer>" description="Initial: `auto`. Sets the column span of the grid item." />
<CssProperty name="grid-column-end" values="span <integer> | <integer>" description="Initial: `auto`. Alias for grid-column." />
<CssProperty name="grid-row" values="span <integer> | <integer>" description="Initial: `auto`. Sets the row span of the grid item." />
<CssProperty name="grid-row-end" values="span <integer> | <integer>" description="Initial: `auto`. Alias for grid-row." />
<CssProperty name="grid-gap" values="<length>" shorthand description="Initial: `0`. Alias for gap." />
<CssProperty name="grid-row-gap" values="<length>" description="Initial: `0`. Alias for row-gap." />
<CssProperty name="grid-column-gap" values="<length>" description="Initial: `0`. Alias for column-gap." />

---

## 9. Scrolling & Overflow Management

<CssProperty name="overflow" values="visible | hidden | scroll | auto" shorthand description="Initial: `visible`. Shorthand to configure horizontal & vertical overflow." />
<CssProperty name="overflow-x" values="visible | hidden | scroll | auto" description="Initial: `visible`. Horizontal layout overflow (visible, hidden, scroll)." />
<CssProperty name="overflow-y" values="visible | hidden | scroll | auto" description="Initial: `visible`. Vertical layout overflow (visible, hidden, scroll)." />
<CssProperty name="scrollbar-width" values="auto | none" description="Initial: `auto`. none hides visual scrollbars while keeping list scrollable." />
<CssProperty name="scrollbar-color" values="<color> <color>" animatable description="Initial: `auto auto`. Foreground (thumb) and background (track) colors of scrollbars." />
<CssProperty name="scroll-speed" values="<integer>" description="Initial: `1`. Shorthand scroll step speed multiplier." />
<CssProperty name="scroll-speed-x" values="<integer>" description="Initial: `1`. Horizontal scroll step distance." />
<CssProperty name="scroll-speed-y" values="<integer>" description="Initial: `1`. Vertical scroll step distance." />
<CssProperty name="scroll-behavior" values="auto | smooth" description="Initial: `auto`. Smooth scrolling transitions configuration." />

---

## 10. Positioning & Transitions

<CssProperty name="transition" values="property duration timing-function" shorthand description="Initial: `none`. Shorthand (e.g. transition: background-color 0.2s linear). Easing keywords: linear, ease, ease-in, ease-out, ease-in-out, ease-in-sine, ease-out-sine, ease-in-out-sine, ease-in-quad, ease-out-quad, ease-in-out-quad, ease-in-cubic, ease-out-cubic, ease-in-quart, ease-out-quart, ease-in-quint, ease-out-quint, ease-in-expo, ease-out-expo, ease-in-circ, ease-out-circ, ease-in-back, ease-out-back." />
<CssProperty name="position" values="static | relative | absolute | fixed | sticky" description="Initial: `static`. Selects positioning flow model. An absolute/fixed box with an auto width or height sizes itself from its content, capped by the space available to it — it is free to be wider than the element it is anchored to, which is what lets a tooltip overhang a narrow trigger. Pinning both opposite edges (top and bottom, or left and right) stretches it between them instead, so inset: 0 fills the nearest positioned ancestor; auto margins on that axis opt back out, keeping the box content-sized and centering it between the edges." />
<CssProperty name="inset" values="1-4 <length> values" shorthand description="Initial: `auto`. Shorthand setting top/right/bottom/left (same expansion as margin)." />
<CssProperty name="aspect-ratio" values="<w> / <h> | <number> | auto" description="Initial: `auto`. Derives an element's auto dimension from whichever of width/height is definite: height-from-width in block, flex, and grid contexts (items and containers), and width-from-height in block contexts and on flex items/containers (grid containers only derive height from width). Ratios are in cells — terminal cells are ~2:1 tall, so 2 / 1 looks square. Content larger than the ratio overflows." />
<CssProperty name="top" values="<length>" description="Initial: `auto`. Offset relative to top boundary." />
<CssProperty name="bottom" values="<length>" description="Initial: `auto`. Offset relative to bottom boundary." />
<CssProperty name="left" values="<length>" description="Initial: `auto`. Offset relative to left boundary." />
<CssProperty name="right" values="<length>" description="Initial: `auto`. Offset relative to right boundary." />
<CssProperty name="z-index" values="<integer> | auto" description="Initial: `auto`. Determines rendering paint layers." />

---

## 11. List Styling

<CssProperty name="list-style-type" values="disc | circle | square | decimal | none" description="Initial: `disc`. Sets the marker prefix style for list items (• , ○ , ■ , numbers, or none)." />
<CssProperty name="list-style" values="disc | circle | square | decimal | none" shorthand description="Initial: `disc`. Shorthand configuration for list styling." />
