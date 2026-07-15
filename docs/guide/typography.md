# Typography & Text Styling

RTXUI supports rich text styling including colors, alignment, decorations,
bold/strong emphasis, and opacity. These are applied via CSS properties in
`<style>` blocks, just like in the web.

---

## Colors

RTXUI supports multiple color formats for `color` and `background-color`
properties:

| Format | Example |
|---|---|
| Named keywords | `red`, `navy`, `silver`, `fuchsia`, … |
| Hex (short) | `#f0f`, `#0ff` |
| Hex (full) | `#ff3366`, `#151520` |
| Hex with alpha | `#00ffcc88`, `#ff000080` |
| `rgb()` | `rgb(255, 0, 128)` |
| `rgba()` | `rgba(255, 0, 0, 0.5)` |

The 16 standard CSS color keywords are supported (`black`, `white`, `red`,
`green`, `blue`, `yellow`, `aqua`, `fuchsia`, `gray`, `silver`, `maroon`,
`olive`, `navy`, `purple`, `teal`, `lime`).

<ExampleTabs src="/wasm/rtxui_example_colors.js">
<template #source>

<<< @/../example/colors.cpp

</template>
</ExampleTabs>

---

## Weight and Style

Terminals render four text intensities and an italic face; RTXUI maps the
standard CSS properties onto them:

```css
font-weight: bold;      /* bold — also: bolder, or numbers >= 600 */
font-weight: lighter;   /* dim  — also: numbers <= 300 */
font-weight: normal;
font-style: italic;     /* also: oblique */
```

The semantic inline tags apply these for you: `<b>`/`<strong>` for bold,
`<i>`/`<em>` for italic, `<u>` for underline, `<s>`/`<del>` for
strikethrough.

---

## Case Transformation

`text-transform` changes the rendered case without touching the underlying
text. It is inherited.

```css
text-transform: uppercase;   /* MENU */
text-transform: lowercase;   /* menu */
text-transform: capitalize;  /* Menu Item */
```

ASCII letters are transformed; other characters pass through unchanged.

---

## Spacing

`letter-spacing` inserts blank cells between characters, and `line-height`
sets how many rows each line occupies. Both take whole numbers of cells and
are inherited:

```css
h1 { letter-spacing: 1; }   /* H E A D I N G */
p  { line-height: 2; }      /* one blank row between lines */
```

The cells added by `letter-spacing` are non-breaking: a spaced word never
wraps in the middle, while the gaps between words stay breakable.
`line-height` is a minimum — a taller inline element still grows its line.

---

## Text Alignment

Use `text-align` to control horizontal text placement within a block element:

```css
text-align: left;    /* Default */
text-align: center;
text-align: right;
text-align: justify;
```

`justify` widens the spaces of soft-wrapped lines so each fills the content
width; the last line, and lines ended by an explicit newline, keep their
natural width. The alignment is inherited by child elements.

<ExampleTabs src="/wasm/rtxui_example_text_align.js">
<template #source>

<<< @/../example/text_align.cpp

</template>
</ExampleTabs>

---

## Text Decoration

Use `text-decoration` to add visual emphasis to text. Multiple decorations can
be combined by separating them with spaces.

| Value | Effect |
|---|---|
| `underline` | Single underline |
| `double-underline` | Double underline |
| `line-through` / `strikethrough` | Horizontal line through text |
| `blink` | Blinking text (terminal support varies) |
| `none` | Remove decorations |

Combine multiple decorations:

```css
text-decoration: double-underline strikethrough;
```

<ExampleTabs src="/wasm/rtxui_example_text_decoration.js">
<template #source>

<<< @/../example/text_decoration.cpp

</template>
</ExampleTabs>

---

## Whitespace and Wrapping

`white-space` controls how newlines and wrapping behave inside an element:

| Value | Newlines | Wrapping |
|---|---|---|
| `normal` (default) | Rendered as spaces | Wraps at spaces |
| `nowrap` | Rendered as spaces | Never wraps |
| `pre` | Hard line breaks | Never wraps |
| `pre-wrap` | Hard line breaks | Wraps at spaces |
| `pre-line` | Hard line breaks; space runs collapse to one | Wraps at spaces |

The `<pre>` element sets `white-space: pre` for you; `<textarea>` uses
`pre-wrap`. For text that must not wrap, pair `nowrap` with
`text-overflow: ellipsis` to truncate with `…` instead of overflowing:

```css
.crumb {
  white-space: nowrap;
  text-overflow: ellipsis;
}
```

Words longer than the line break at the container edge by default
(`overflow-wrap: anywhere` — note this differs from CSS, whose default lets
long words overflow). Set `overflow-wrap: normal` to keep such words intact
and let them overflow instead:

```css
.url { overflow-wrap: normal; }
```

---

## Opacity

The `opacity` property controls the transparency of an element and all its
children. Values range from `0.0` (fully transparent) to `1.0` (fully opaque).

```css
opacity: 0.5;
```

Opacity is multiplicative: a child with `opacity: 0.5` inside a parent with
`opacity: 0.5` will render at 25% opacity. Opacity can also be animated using
CSS transitions.

<ExampleTabs src="/wasm/rtxui_example_opacity.js">
<template #source>

<<< @/../example/opacity.cpp

</template>
</ExampleTabs>
