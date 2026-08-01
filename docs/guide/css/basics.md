# CSS Basics & Selectors

Each component carries its styles next to its markup, and those styles apply
only to that component's own template — class names cannot collide across
components. This page covers where styles live, which selectors match, and
how values work in a terminal.

## Declaring Styles

Styles are written using standard CSS rulesets inside a `<style>` block in your component's template string:

```html
<div>
  <span class="header">App Title</span>
</div>

<style>
  .header {
    color: aqua;
    font-weight: bold;
  }
</style>
```

## Supported Selectors

The CSS parser supports a wide range of standard selectors and combinators:
*   **Tag Selectors**: Targets specific tags (e.g., `div { margin: 1; }`).
*   **Class Selectors**: Targets class names (e.g., `.card { padding: 1; }`).
*   **ID Selectors**: Targets unique identifiers (e.g., `#submit-btn { background-color: green; }`).
*   **Compound Selectors**: Combine tags, classes, and IDs simultaneously (e.g., `div.card#active { border-color: red; }`).
*   **Pseudo-classes**: Targets interactive states (`:hover`, `:focus`, `:active`,
    `:disabled`, `:read-only`) and scrollbars.
*   **Structural Pseudo-classes**:
    *   `:first-child`: Matches the first element among its siblings.
    *   `:last-child`: Matches the last element among its siblings.
    *   `:nth-child(even)`/`:nth-child(odd)`: Matches even or odd siblings.
    *   `:nth-child(N)`: Matches the 1-based N-th sibling (e.g., `:nth-child(3)`).
*   **Combinators**:
    *   **Descendant combinator (space)**: Matches nested elements (e.g., `div span` targets any `span` inside a `div`).
    *   **Child combinator (`>`)**: Matches direct children (e.g., `div > span` targets `span` elements immediately nested under `div`).
    *   **Adjacent Sibling combinator (`+`)**: Matches immediate following sibling (e.g., `div + p` targets a `p` that is placed right after a `div`).
    *   **General Sibling combinator (`~`)**: Matches any following sibling (e.g., `div ~ p` targets any `p` that shares the same parent and follows a `div`).

### The Special `self` Selector
To target the component's root outer boundary tag itself (rather than one of its child elements), use the `self` selector keyword:

```css
self {
  display: block;
  border: round;
  border-color: yellow;
}
```
This is essential for wrapping custom components in custom borders or configuring their layout growth constraints in flex containers.

Attribute selectors are also supported: `input[value]` matches elements
having the attribute, `button[disabled=true]` matches an exact value.

### Styling a Nested Component's Internals: `::part()`

The opening line of this page said styles apply "only to that component's
own template" — that's true even for a component you instantiate yourself.
`<textarea linenumbers="true">`'s line-number gutter is built out of `<div>`s
inside `textarea`'s own template; a `.gutter { color: ... }` rule in *your*
component's stylesheet simply never reaches them, no matter how directly you
wrote the `<textarea>` tag.

A component opts specific internal elements into being styled from outside
by marking them with a `part` attribute (space-separated for more than one,
like `class`):

```html
<!-- inside some component's own template -->
<div class="row" part="gutter active">...</div>
```

An outside component then targets that name with `::part(name)`, following
whatever selector matches the *instantiation site* (tag, class, id — not the
part element itself):

```css
textarea::part(gutter) { color: rgb(100, 116, 139); }
.editor::part(active)  { color: rgb(129, 140, 248); }
```

This reaches through as many layers of nesting as it takes to get there —
internal markup inside the component, and even further component
boundaries in between. If `<Foo>`'s own template instantiates `<Bar
part="baz">`, an app that only ever writes `<Foo class="thing">` can still
reach `baz` directly with `.thing::part(baz)`, with no need for `Foo` to
forward or re-expose it itself (there's no `exportparts`-style ceremony
here, unlike standard CSS shadow DOM).

Built-in components that expose parts document them alongside their other
attributes — see the [`<textarea>`](/guide/forms) guide for the
line-number gutter and current-line highlight parts.

## Inline Styles

The `style` attribute applies declarations to a single element, taking
precedence over rules from `<style>` blocks:

```html
<div style="padding: 1; color: red;">Highlighted</div>
```

## Values in a Terminal

Lengths are measured in **character cells** — `padding: 1` is one cell, and
because cells are roughly twice as tall as they are wide, one vertical cell
looks about as large as two horizontal ones. Percentages resolve against the
parent, and lengths accept arithmetic:

```css
.sidebar { width: 25%; }
.content { width: calc(100% - 20); }
.panel   { width: min(100%, 60); height: clamp(5, 50%, 20); }
```

## Custom Properties

Variables declared with `--name` inherit down the tree and are read with
`var(--name)` or `var(--name, fallback)` — the usual way to define a theme
in one place:

```css
self { --accent: rgb(59, 130, 246); }
.card { border-color: var(--accent); }
.card-title { color: var(--accent); }
```

## `!important`

Appending `!important` to a declaration makes it win over normal
declarations from later rules and over inline styles, as in standard CSS.
Reach for it rarely; more specific selectors usually express intent better.

The [CSS property reference](/css_reference) lists every supported property
with its accepted values.
