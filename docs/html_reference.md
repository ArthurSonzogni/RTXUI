# HTML Element & Component Reference

This specification defines the complete set of built-in XML/HTML tags and components supported by the RTXUI template parser, including their classification, attribute interfaces, slot projections, event contracts, and default user-agent stylesheets.

---

## 1. Architectural Model

### 1.1 Unified Component Architecture
Every built-in tag—from `<div>` to `<dialog>`—is a component registered in the global component registry and built over `rtxui::Element`. Simpler tags like `<div>` or `<span>` use trivial templates (e.g. `<slot></slot>` with a default stylesheet), while richer widgets like `<input>` or `<select>` encapsulate internal HTML templates, reactive state bindings, slot projections, and scoped CSS rules. The difference is in complexity, not in kind.

### 1.2 Parser Contract
- **Well-Formed XML/HTML**: All tags must be balanced or explicitly self-closed (e.g. `<hr />`, `<br />`, `<input />`). Unclosed tags generate parser errors with line and column diagnostics.
- **Case Sensitivity**: Tag names and attribute names are parsed as lower-case identifiers.
- **Quoting**: Attribute values must be enclosed in double (`"..."`) or single (`'...'`) quotes.

### 1.3 Binding Syntax
| Syntax | Target | Example | Semantic Contract |
| :--- | :--- | :--- | :--- |
| `{member}` | Text / Attribute | `<span id="{node_id}">{title}</span>` | Interpolates C++ member snapshot; updates DOM on change. |
| `:attr="member"` | Attribute | `<input :value="username" />` | Equivalent to `attr="{member}"`. |
| `on<event>="Handler"` | Event Callback | `<button onclick="Submit">OK</button>` | Dispatches to registered C++ method or lambda. |
| `@<event>="Handler"` | Event Callback | `<button @click="Submit">OK</button>` | Shorthand for event dispatch. |
| `@<event>.<mod>="Handler"` | Event Modifier | `<button @click.right="OnMenu">OK</button>` | Restricts dispatch to specific event conditions (e.g. mouse right-click). |

---

## 2. Global Attributes & Directives

The following attributes apply to all elements and components:

| Attribute | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `id` | `string` | `""` | Unique element identifier; targets `#id` CSS rules and `for` associations. |
| `class` | `string` | `""` | Space-delimited token list for `.class` CSS selector matching. |
| `style` | `string` | `""` | Inline CSS declarations. Overrides stylesheet rules with higher specificity. |
| `tabindex` | `integer` | `-1` | Sequential keyboard focus order. Values `>= 0` participate in Tab navigation; `-1` excludes the element. |
| `focusable` | `boolean` | `false` | Shorthand; `focusable="true"` is equivalent to `tabindex="0"`. |
| `disabled` | `boolean` | `false` | Disables user interaction, removes focus, and applies the `:disabled` CSS pseudo-class. |
| `if` | `expression` | — | Conditional rendering directive. If the expression evaluates to `false`, the element and its subtree are pruned from the DOM. |
| `part` | `string` | `""` | Names an element part for `::part()` CSS selector targeting from ancestor components. |
| `onanimationend` | `handler` | — | Runs when one of the element's [keyframe animations](/guide/css/animations#animation-end) finishes. Not for an `infinite` one, nor one removed before its end. |

---

## 3. Structural Containers & Layout

### `<div>`
- **Classification**: Block Component
- **Layout Model**: `display: block`
- **Semantics**: Standard block-level container for flow, flex, or grid layouts.

### `<span>`
- **Classification**: Inline Component
- **Layout Model**: `display: inline`
- **Semantics**: Inline-level formatting wrapper for styled text segments.

### Semantic Elements (`<header>`, `<footer>`, `<main>`, `<nav>`, `<aside>`, `<section>`, `<article>`)
- **Classification**: Block Components
- **Layout Model**: `display: block`
- **Semantics**: Structural containers functionally identical to `<div>`, providing semantic document organization.

### `<table>`, `<thead>`, `<tbody>`, `<tfoot>`, `<tr>`, `<th>`, `<td>`
- **Classification**: Table Components
- **Layout Model**: Table formatting context with automatic column and row sizing.
- **Section Containers**: `<thead>`, `<tbody>`, and `<tfoot>` group rows logically. The layout engine traverses them transparently when assembling grid rows and computing cell geometry.

#### `<th>` / `<td>` Attributes
| Attribute | Type | Default | Reactive | Description |
| :--- | :--- | :--- | :--- | :--- |
| `colspan` | `integer` | `1` | One-way | Number of columns the cell should span. |
| `rowspan` | `integer` | `1` | One-way | Number of rows the cell should span. |

#### Table Sizing Algorithm:
- **Columns**: Evaluates the maximum preferred cell width across each column. Extra horizontal space from fixed container `width` or `min-width` is distributed proportionally.
- **Rows**: Row height conforms to the tallest cell in the row. Shorter cells stretch vertically to maintain unbroken borders and backgrounds.

```xml
<table>
  <thead>
    <tr>
      <th>Service</th>
      <th>Status</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td>Database</td>
      <td>Active</td>
    </tr>
  </tbody>
</table>

<style>
  table {
    border: solid;
    border-color: #3b82f6;
  }
  th {
    font-weight: bold;
    border-bottom: solid;
    border-color: #334155;
  }
  td, th {
    padding-left: 1;
    padding-right: 1;
  }
</style>
```

<ExampleTabs src="/wasm/rtxui_example_table.js">
<template #source>

<<< @/../example/table.cpp

</template>
</ExampleTabs>

---

## 4. Form Controls & Interactive Widgets

### `<button>`
- **Classification**: Inline Component (`rtxui::button`)
- **Focusable**: Yes (`tabindex="0"`)

#### Attributes
| Attribute | Type | Default | Reactive | Description |
| :--- | :--- | :--- | :--- | :--- |
| `disabled` | `boolean` | `false` | One-way | Suppresses activation events and sets `:disabled` pseudo-class. |
| `onclick` | `handler` | — | — | Callback invoked on mouse click or `Enter`/`Space` keyboard activation. |

#### Slots
| Slot | Target Element | Description |
| :--- | :--- | :--- |
| `(default)` | Inline container | Button text or child element content. |

#### Default Stylesheet
```css
self {
  display: inline-block;
  padding-left: 1;
  padding-right: 1;
  cursor: pointer;
  background-color: lighten(10%);
  opacity: 0.85;
  transition: background-color 0.1s linear, opacity 0.1s linear, color 0.1s linear;
}
self:hover    { background-color: rgb(59, 130, 246); color: white; opacity: 0.95; }
self:focus    { background-color: rgb(37, 99, 235); color: white; opacity: 1.0; }
self:active   { background-color: rgb(29, 78, 216); color: white; opacity: 1.0; }
self:disabled { cursor: default; opacity: 0.4; }
```

---

### `<input>`
- **Classification**: Inline Component (`rtxui::input`)
- **Focusable**: Yes (`tabindex="0"`)

#### Attributes
| Attribute | Type | Default | Reactive | Description |
| :--- | :--- | :--- | :--- | :--- |
| `value` | `string` | `""` | Two-way | Text content. Automatically synchronized during typing. |
| `type` | `string` | `"text"` | One-way | Input format. `"text"` renders characters; `"password"` masks characters with `*`. |
| `placeholder` | `string` | `""` | One-way | Ghost text displayed when `value` is empty. |
| `disabled` | `boolean` | `false` | One-way | Disables input and focus; applies `:disabled` pseudo-class. |
| `readonly` | `boolean` | `false` | One-way | Prevents text modification while allowing focus and text selection. |
| `select_on_focus` | `boolean` | `false` | One-way | Automatically selects all text content when focused. |
| `maxlength` | `integer` | `-1` | One-way | Maximum character length constraint (`-1` indicates unconstrained). |
| `onchange` | `handler` | — | — | Callback invoked when value is committed. Accepts `void()` or `void(std::string)`. |

#### Styleable Parts (`::part()`)
- `placeholder`: The span element rendering placeholder text.
- `cursor`, `cursor-focused`: The active cursor indicator.
- `selection`: Highlighted text selection regions.

#### Default Stylesheet
```css
self {
  display: inline-flex;
  flex-direction: row;
  width: 20;
  padding-left: 1;
  padding-right: 1;
  overflow-x: scroll;
  scrollbar-width: none;
  white-space: nowrap;
  background-color: rgb(40, 40, 40);
  opacity: 0.8;
}
self:focus {
  opacity: 1.0;
}
self:disabled {
  opacity: 0.4;
  cursor: default;
}
```

---

### `<textarea>`
- **Classification**: Block Component (`rtxui::textarea`)
- **Focusable**: Yes (`tabindex="0"`)

#### Attributes
| Attribute | Type | Default | Reactive | Description |
| :--- | :--- | :--- | :--- | :--- |
| `value` | `string` | `""` | Two-way | Multi-line text content. |
| `placeholder` | `string` | `""` | One-way | Ghost text rendered when content is empty. |
| `disabled` | `boolean` | `false` | One-way | Disables editing and interaction. |
| `readonly` | `boolean` | `false` | One-way | Prevents text edits while maintaining scroll and selection capabilities. |
| `select_on_focus` | `boolean` | `false` | One-way | Automatically selects all text content when focused. |
| `maxlength` | `integer` | `-1` | One-way | Maximum character capacity limit. |
| `linenumbers` | `string` | `""` | One-way | Displays line number gutter. Values: `""` (off), `"true"` / `"absolute"` (absolute), `"relative"` (relative). |
| `line_start` | `integer` | `1` | One-way | The displayed number of the first logical line. |
| `line_end` | `integer` | `-1` | One-way | The last displayed number to show a gutter entry for; `-1` means unbounded. |
| `line_wrap` | `string` | `""` | One-way | Soft line wrapping at viewport boundary. Values: `""`, `"ignore"`, `"subline"`. |
| `highlight_current_line` | `boolean` | `false` | One-way | Emphasizes the row hosting the active cursor. |

---

### `<checkbox>`
- **Classification**: Inline-Flex Component (`rtxui::checkbox`)
- **Focusable**: Yes (`tabindex="0"`)

#### Attributes
| Attribute | Type | Default | Reactive | Description |
| :--- | :--- | :--- | :--- | :--- |
| `checked` | `boolean` | `false` | Two-way | State toggle. Toggles between `☑` and `☐`. Synchronizes `:checked` pseudo-class. |
| `disabled` | `boolean` | `false` | One-way | Suppresses toggle interaction and sets `:disabled` pseudo-class. |
| `onchange` | `handler` | — | — | Fired on state change. Accepts `void()` or `void(bool)`. |

#### Styleable Parts (`::part()`)
- `checkmark`: Span element containing the checkmark glyph (`☑` or `☐`).

#### Default Stylesheet
```css
self {
  display: inline-flex;
  flex-direction: row;
  white-space: nowrap;
  cursor: pointer;
  padding-left: 1;
  padding-right: 1;
  transition: background-color 0.1s linear;
}
self:hover    { background-color: lighten(10%); }
self:focus    { background-color: lighten(25%); }
self:active   { background-color: lighten(35%); }
self:disabled { cursor: default; opacity: 0.4; }
.checkmark    { margin-right: 1; }
```

---

### `<radio>`
- **Classification**: Inline-Flex Component (`rtxui::radio`)
- **Focusable**: Yes (`tabindex="0"`)

#### Attributes
| Attribute | Type | Default | Reactive | Description |
| :--- | :--- | :--- | :--- | :--- |
| `name` | `string` | `""` | One-way | Radio group identifier. Only one radio sharing a `name` can be checked simultaneously. |
| `checked` | `boolean` | `false` | Two-way | Selection state. Toggles between `◉` and `○`. Synchronizes `:checked` pseudo-class. |
| `disabled` | `boolean` | `false` | One-way | Suppresses activation and applies `:disabled` pseudo-class. |
| `onchange` | `handler` | — | — | Fired when the radio is selected. |

#### Styleable Parts (`::part()`)
- `radio-mark`: Span element containing the radio glyph (`◉` or `○`).

```xml
<radio name="theme" checked="{dark_mode}">Dark</radio>
<radio name="theme" checked="{light_mode}">Light</radio>
```

<ExampleTabs src="/wasm/rtxui_example_radio.js">
<template #source>

<<< @/../example/radio.cpp

</template>
</ExampleTabs>

---

### `<select>` and `<option>`
- **Classification**: Component (`rtxui::select`, `rtxui::option`)
- **Focusable**: Yes (`tabindex="0"`)

#### `<select>` Attributes
| Attribute | Type | Default | Reactive | Description |
| :--- | :--- | :--- | :--- | :--- |
| `value` | `string` | `""` | Two-way | Active selected option value. |
| `disabled` | `boolean` | `false` | One-way | Disables dropdown expansion. |
| `onchange` | `handler` | — | — | Fired on option change. Accepts `void()` or `void(std::string)`. |

#### `<option>` Attributes
| Attribute | Type | Default | Reactive | Description |
| :--- | :--- | :--- | :--- | :--- |
| `value` | `string` | `""` | One-way | Distinct value payload passed to `<select>`. |
| `disabled` | `boolean` | `false` | One-way | Prevents selection of this specific option. |

#### Styleable Parts (`::part()`)
- `select-btn`: Main clickable trigger container.
- `select-label`: Active selected text label.
- `select-arrow`: Dropdown expand/collapse arrow glyph.
- `dropdown-list`: Floating options list container.

---

### `<slider>`
- **Classification**: Inline Component (`rtxui::slider`)
- **Focusable**: Yes (`tabindex="0"`)

#### Attributes
| Attribute | Type | Default | Reactive | Description |
| :--- | :--- | :--- | :--- | :--- |
| `value` | `integer` | `0` | Two-way | Current slider value. Clamped between `min` and `max`. |
| `min` | `integer` | `0` | One-way | Lower bound of the range. |
| `max` | `integer` | `100` | One-way | Upper bound of the range. |
| `step` | `integer` | `1` | One-way | Discrete increment step on keyboard navigation or dragging. |
| `width` | `integer` | `20` | One-way | Visual cell track width. |
| `direction` | `string` | `"horizontal"`| One-way | Track orientation (`"horizontal"` or `"vertical"`). |
| `disabled` | `boolean` | `false` | One-way | Disables slider manipulation. |
| `onchange` | `handler` | — | — | Fired on value updates. Accepts `void()` or `void(int)`. |

#### Styleable Parts (`::part()`)
- `slider-container`: Wrapper element.
- `track-left`: Filled track portion preceding the thumb.
- `thumb`: Visual thumb handle glyph (`●`).
- `track-right`: Unfilled track portion following the thumb.

---

### `<progress>`
- **Classification**: Inline Component (`rtxui::progress`)
- **Focusable**: No

#### Attributes
| Attribute | Type | Default | Reactive | Description |
| :--- | :--- | :--- | :--- | :--- |
| `value` | `float` | `0.0` | One-way | Current progress amount. |
| `max` | `float` | `100.0` | One-way | Maximum range value. |
| `width` | `integer` | `20` | One-way | Physical cell width of the bar. |

#### Styleable Parts (`::part()`)
- `filled`: Active progress section (rendered using `█`).
- `empty`: Inactive remainder section.

---

### `<label>`
- **Classification**: Inline-Flex Component (`rtxui::label`)
- **Focusable**: No (`cursor: pointer`)

#### Attributes
| Attribute | Type | Default | Reactive | Description |
| :--- | :--- | :--- | :--- | :--- |
| `for` | `string` | `""` | One-way | ID of target interactive element. |

#### Event Delegation Semantics:
Clicking a `<label>` delegates focus and activation to the target:
1. **Explicit Target**: Element whose `id` matches the `for` attribute.
2. **Implicit Target**: The first focusable descendant nested within `<label></label>`.

<ExampleTabs src="/wasm/rtxui_example_label.js">
<template #source>

<<< @/../example/label.cpp

</template>
</ExampleTabs>

---

## 5. Disclosure, Navigation & Overlays

### `<details>` and `<summary>`
- **Classification**: Block Component (`rtxui::details`, `rtxui::summary`)

#### Attributes
| Attribute | Type | Default | Reactive | Description |
| :--- | :--- | :--- | :--- | :--- |
| `open` | `boolean` | `false` | Two-way | Expansion state. Toggled by clicking `<summary>` or pressing `Enter`/`Space`. |

#### Slots
| Slot | Target Element | Description |
| :--- | :--- | :--- |
| `summary` | `<summary>` | Clickable disclosure header row. If omitted, defaults to `"Details"`. |
| `(default)` | Block container | Collapsible body content displayed when `open="true"`. |

<ExampleTabs src="/wasm/rtxui_example_details.js">
<template #source>

<<< @/../example/details.cpp

</template>
</ExampleTabs>

---

### `<fieldset>` and `<legend>`
- **Classification**: Block Component (`rtxui::fieldset`, `rtxui::legend`)

#### Attributes
| Attribute | Type | Default | Reactive | Description |
| :--- | :--- | :--- | :--- | :--- |
| `disabled` | `boolean` | `false` | One-way | Disables the fieldset and all enclosed form controls. |

#### Slots
| Slot | Target Element | Description |
| :--- | :--- | :--- |
| `legend` | `<legend>` | Header caption nested into the top border frame. |
| `(default)` | Block container | Enclosed form controls and content. |

<ExampleTabs src="/wasm/rtxui_example_fieldset.js">
<template #source>

<<< @/../example/fieldset.cpp

</template>
</ExampleTabs>

---

### `<tabs>` and `<tab-pane>`
- **Classification**: Component (`rtxui::tabs`, `rtxui::tab_pane`)

#### `<tabs>` Attributes
| Attribute | Type | Default | Reactive | Description |
| :--- | :--- | :--- | :--- | :--- |
| `value` | `string` | `""` | Two-way | Active pane identifier matching `<tab-pane name="...">`. |
| `onchange` | `handler` | — | — | Fired when active tab changes. |

#### `<tab-pane>` Attributes
| Attribute | Type | Default | Reactive | Description |
| :--- | :--- | :--- | :--- | :--- |
| `name` | `string` | `""` | One-way | Unique tab identifier. |
| `label` | `string` | `""` | One-way | Header text displayed on the tab button. |

<ExampleTabs src="/wasm/rtxui_example_tabs.js">
<template #source>

<<< @/../example/tabs.cpp

</template>
</ExampleTabs>

---

### `<dialog>`
- **Classification**: Modal Overlay (`rtxui::dialog`)
- **Layer**: Top-layer modal (`z-index: 100`)

#### Attributes
| Attribute | Type | Default | Reactive | Description |
| :--- | :--- | :--- | :--- | :--- |
| `open` | `boolean` | `false` | Two-way | Visibility toggle. When `true`, presents centered overlay with translucent backdrop. |
| `title` | `string` | `"Dialog"` | One-way | Title text displayed in the header frame. |

#### Keyboard Invariants:
Pressing the `Escape` key automatically dismisses an open dialog and updates the bound `open` property to `false`.

<ExampleTabs src="/wasm/rtxui_example_dialog.js">
<template #source>

<<< @/../example/dialog.cpp

</template>
</ExampleTabs>

---

### `<toast>`
- **Classification**: Notification Overlay (`rtxui::toast`)
- **Layer**: Top-layer popup (`z-index: 200`)

A short notification in a corner of the screen, over everything else. It
closes itself after `duration` milliseconds, or when clicked.

#### Attributes
| Attribute | Type | Default | Reactive | Description |
| :--- | :--- | :--- | :--- | :--- |
| `open` | `boolean` | `false` | Two-way | Shows the toast. It returns to `false` when the toast closes, by its timer or a click. Each opening starts the timer again. |
| `duration` | `integer` | `3000` | One-way | Milliseconds before it closes itself; `0` keeps it until clicked or closed by the application. |
| `placement` | `string` | `"bottom-right"` | One-way | Corner: `bottom-right`, `bottom-left`, `top-right` or `top-left`. Anything else is reported as a diagnostic. |

#### Parts
`toast` (the box). It has a `tall` border by default; restyle it like any
element, e.g. `.error::part(toast) { border-color: red; }` or
`toast::part(toast) { border: round; }`. While it closes, the box is also part
`closing`.

#### Animation
A toast slides in horizontally from its side of the screen, and slides back
out when it closes: a terminal has more columns than rows, so a sideways move
looks smoother than a vertical one. It stays on screen until its closing
animation ends. Replace either animation with your own `@keyframes`, declared
in your component:

```css
@keyframes fade-in { from { opacity: 0; } }
@keyframes fade-out { to { opacity: 0; } }
toast::part(toast) { animation: fade-in 200ms; }
toast::part(closing) { animation: fade-out 200ms forwards; }
```

Write the `closing` rule after the `toast` one: the closing box is both
parts. `toast::part(toast) { animation: none; }` turns both animations off,
and a toast with no closing animation, or an `infinite` one, hides at once.

Several toasts open at once in the same corner overlap. To stack them, give
each a different `placement`, or render them from a `<for>` inside a fixed
container of your own and set `toast::part(toast) { position: static; }`.

<ExampleTabs src="/wasm/rtxui_example_toast.js">
<template #source>

<<< @/../example/toast.cpp

</template>
</ExampleTabs>

---

### `<tree-item>`
- **Classification**: Interactive Tree Node (`rtxui::tree_item`)
- **Focus**: Its row is focusable.

One node of a tree: a row showing `label`, and the `<tree-item>`s nested inside
it, shown indented below while it is open. A node with no `<tree-item>` (or
other element) inside is a leaf.

```html
<tree-item label="src" open="{src_open}">
  <tree-item label="main.cpp" onclick="Open(main.cpp)"/>
  <tree-item label="ui">
    <tree-item label="app.cpp" onclick="Open(ui/app.cpp)"/>
  </tree-item>
</tree-item>
```

#### Attributes
| Attribute | Type | Default | Reactive | Description |
| :--- | :--- | :--- | :--- | :--- |
| `label` | `string` | `""` | One-way | Text of the row. |
| `open` | `boolean` | `false` | Two-way | Whether the children show. Ignored on a leaf. |
| `onclick` | callback | — | — | Runs when the row is clicked or activated with Enter. A branch without one toggles instead. |

#### Interaction
- Clicking the arrow toggles a branch. Clicking the row, or pressing
  <kbd>Enter</kbd> on it, runs its `onclick`; a branch without one toggles.
- <kbd>Right</kbd> opens and <kbd>Left</kbd> closes the focused branch; with
  nothing to open or close, the arrows move focus as usual.

#### Parts
`row`, `arrow` (`▸`, `▾`, or blank for a leaf), `label`, `children`.

<ExampleTabs src="/wasm/rtxui_example_tree.js">
<template #source>

<<< @/../example/tree.cpp

</template>
</ExampleTabs>

---

### `<tooltip>`
- **Classification**: Hover Overlay (`rtxui::tooltip`)
- **Layer**: Top-layer popup (`z-index: 1000`)

#### Attributes
| Attribute | Type | Default | Reactive | Description |
| :--- | :--- | :--- | :--- | :--- |
| `content` | `string` | `""` | One-way | Text rendered inside the floating popup. |
| `placement`| `string` | `"top"` | One-way | Positioning anchor: `"top"`, `"bottom"`, `"left"`, `"right"`, `"top-start"`, `"top-end"`, `"bottom-start"`, `"bottom-end"`. |

<ExampleTabs src="/wasm/rtxui_example_tooltip.js">
<template #source>

<<< @/../example/tooltip.cpp

</template>
</ExampleTabs>

---

## 6. Typography & Formatting Elements

| Tag | Classification | Default Style | Semantic Invariant |
| :--- | :--- | :--- | :--- |
| `<h1>`, `<h2>`, `<h3>`, `<h4>`, `<h5>`, `<h6>` | Block | `display: block; font-weight: bold;` | Hierarchical headings with stepped margins and sizes. |
| `<p>` | Block | `display: block; margin: 1 0;` | Paragraph block with vertical separation. |
| `<a>` | Inline | `color: rgb(59, 130, 246); text-decoration: underline;` | Hyperlink. `href="#id"` scrolls the element with that id into view, when clicked or when focused and activated with <kbd>Enter</kbd>. With an `href`, it is in the Tab order, as in a browser. An `href` that is a URL with a scheme (`https://…`, `mailto:…`, `file://…`) makes the text a terminal hyperlink (OSC 8): kitty, WezTerm, iTerm2, foot, GNOME Terminal and Windows Terminal open it the way each does for links (usually Ctrl- or Cmd-click, plus Shift in some while the app reads the mouse), and other terminals show the text as usual. |
| `<b>`, `<strong>` | Inline | `font-weight: bold;` | Emphasized text with bold weight attribute. |
| `<i>`, `<em>` | Inline | `font-style: italic;` | Slanted text with italic terminal attribute. |
| `<u>` | Inline | `text-decoration: underline;` | Underlined text. |
| `<s>`, `<strike>`, `<del>`| Inline | `text-decoration: line-through;` | Strikethrough text. |
| `<code>` | Inline | `background-color: rgb(40, 40, 40);` | Monospaced inline code snippet. |
| `<kbd>` | Inline | `display: inline; font-weight: bold;` | Keyboard shortcut token. |
| `<pre>` | Block | `white-space: pre;` | Preformatted text preserving line breaks and spaces. |
| `<blockquote>` | Block | `display: block; padding-left: 2; border-left: solid;` | Indented quotation block with left accent line. |
| `<hr />` | Block | `height: 1; border-top: solid;` | Full-width horizontal divider line. |
| `<br />` | Inline | — | Forces hard line break in inline flow algorithms. |

### `<markdown>`
- **Classification**: Block Component (`rtxui::markdown`)

#### Attributes
| Attribute | Type | Default | Reactive | Description |
| :--- | :--- | :--- | :--- | :--- |
| `content` | `string` | `""` | Two-way | Raw Markdown text to parse and render. |
| `stylesheet` | `string` | `""` | One-way | Custom CSS applied to the rendered DOM tree. |

<ExampleTabs src="/wasm/rtxui_example_hr.js">
<template #source>

<<< @/../example/hr.cpp

</template>
</ExampleTabs>

---

## 7. Lists (`<ul>`, `<ol>`, `<li>`)

### Unordered Lists (`<ul>`)
Unordered lists render alternating glyph markers based on structural nesting depth:
- **Level 1**: Disc (`• `)
- **Level 2**: Circle (`○ `)
- **Level 3+**: Square (`■ `)

### Ordered Lists (`<ol>`)
Ordered lists format numbered decimal prefixes (`1. `, `2. `):
- `start` (`integer`, default `1`): Initial counter value.
- `reversed` (`boolean`, default `false`): Counts down rather than up.

### List Items (`<li>`)
- `value` (`integer`): Explicit sequence override within ordered lists.

<ExampleTabs src="/wasm/rtxui_example_lists.js">
<template #source>

<<< @/../example/lists.cpp

</template>
</ExampleTabs>

---

## 8. Template Control Flow

### `<if>`, `<elif>`, `<else>`
Conditional evaluation nodes. Direct children of `<if>` branches are mounted or unmounted dynamically:
```xml
<if condition="{has_error}">
  <span class="error">{error_message}</span>
</if>
<elif condition="{is_loading}">
  <span>Loading...</span>
</elif>
<else>
  <span>Ready.</span>
</else>
```

### `<for>`
Repeats template structures across bound collections. Syntax: `<for each="{collection}" as="item">`. The loop index is available as `{$index}` (zero-based).
- `each`: Bound collection expression.
- `as`: Variable alias for collection element.
- `key`: (Optional) Unique identifier field on struct item. Preserves element state and cursor position across sort and filter operations.
- `virtual`: (Optional) Inside a scroll container, renders only the items around what it shows. See [Long lists](/guide/loops#long-lists-virtual).
- `item-height`: (Optional, with `virtual`) Rows each item takes; `1` by default.

### `<slot>` and `<template>` (Content Projection)
Enables reusable components to project child content passed by their caller:
- **Default Slot**: `<slot></slot>` inside a component definition renders all unassigned child elements.
- **Named Slots**: `<slot.header></slot.header>` or `<slot name="header"></slot>` defines named projection targets.
- **Content Injection**: Callers project into named slots via `<template.header>` or `<template name="header">`.
- **Fallback Content**: Markup placed inside `<slot>Fallback</slot>` is rendered if the caller provides no projecting children.

```xml
<!-- Component Definition (e.g. Card) -->
<div class="card">
  <div class="card-header"><slot.header>Default Header</slot.header></div>
  <div class="card-body"><slot></slot></div>
</div>

<!-- Caller Usage -->
<Card>
  <template.header>
    <span>Custom Title</span>
  </template.header>
  <p>Body projected into default slot.</p>
</Card>
```

<ExampleTabs src="/wasm/rtxui_example_slots.js">
<template #source>

<<< @/../example/slots.cpp

</template>
</ExampleTabs>
