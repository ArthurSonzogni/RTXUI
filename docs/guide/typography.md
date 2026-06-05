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

## Text Alignment

Use `text-align` to control horizontal text placement within a block element:

```css
text-align: left;    /* Default */
text-align: center;
text-align: right;
```

The alignment is inherited by child elements.

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
