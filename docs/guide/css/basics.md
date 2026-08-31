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
    *   `:nth-child(An+B)`: Matches sibling `A x k + B` for every whole `k >= 0`
        -- `3n` is every third sibling, `2n+1` the odd ones, `n+3` everything
        from the third on, and `-n+2` the first two. Whitespace is allowed
        around the parts and the `n` is case-insensitive, so `2N + 1` parses.
    *   `:nth-last-child(...)`: The same arguments, counted from the last sibling backwards.
    *   `:only-child`: Matches an element that is its parent's only child.
    *   `:empty`: Matches an element with no content between its tags.
    *   `:first-of-type` / `:last-of-type` / `:only-of-type` / `:nth-of-type(...)` / `:nth-last-of-type(...)`: The same as the `-child` family, but counting only siblings with the same tag.
    *   `:not(<selector>)`: Matches an element that the inner selector does not. Takes a single compound selector -- a tag, `.class`, `#id`, `[attribute]`, a pseudo-class, or a combination such as `:not(div.card)` or `:not(.item:first-child)`. Every part of the argument must match for the negation to exclude an element, so `:not(.x:first-child)` still matches a `.x` that is not first. Selector lists (`:not(.a, .b)`) and combinators inside the negation are not supported and match nothing, as does an empty `:not()`.

    `<style>` blocks and text do not count as siblings, so they never displace a
    `:first-child` nor stop an element being an `:only-child`. An `An+B` offset must
    carry its own sign, so `2n3` is not `2n+3`; it matches nothing, as does
    any other argument that is not one of the forms above.

    A pseudo-class outside the list above matches nothing, and one bad token
    disqualifies the whole selector: a misspelled `span:hovr` styles no span
    rather than every span. Rules that quietly stop applying are easier to
    spot than rules that quietly apply everywhere.

    `:empty` departs from CSS in one way: CSS counts any text node as content,
    which would mean nothing is ever empty in a template where tags sit
    indented on their own lines. Whitespace-only text is ignored here, so
    `<div></div>` and a `<div>` spanning two lines with nothing between them
    both match.
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

## The Cascade

When more than one rule sets the same property on an element, the winner is the
one with the highest **specificity** — the same rule CSS uses. Specificity
counts three things across the whole selector, ancestors included:

| Weight | Counts | Example |
| :--- | :--- | :--- |
| highest | id selectors | `#sidebar` |
| middle | classes, attributes, pseudo-classes | `.card`, `[open]`, `:hover` |
| lowest | element names | `div` |

A single id outranks any number of classes, and a single class outranks any
number of element names:

```css
#panel      { color: red; }   /* wins */
.a.b.c      { color: blue; }
section div { color: green; }
```

Because the count runs over the entire selector, rules that look similar still
rank correctly:

```css
.a.b        { color: red; }   /* wins over .a  — two classes beat one */
div.a       { color: red; }   /* wins over .a  — same classes, one more type */
.wrap .a    { color: red; }   /* wins over .a  — same classes, plus an ancestor */
```

`*` contributes nothing, and `self` selects the component's own root rather
than an element name, so neither adds weight.

Between two rules of **equal** specificity, the later one wins:

```css
.a { color: red; }
.b { color: blue; }   /* an element with class="a b" is blue */
```

The `style` attribute is applied after every selector, so it always wins, and
`!important` overrides even that — see [`!important`](#important) below.

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

::: warning Known limitation
An `!important` declaration does not currently survive a matching
pseudo-class rule on the same element: given

```css
.row       { color: green !important; }
.row:focus { color: blue; }
```

a focused `.row` renders blue, where CSS would keep it green. Interactive
state is resolved in a second pass layered on top of the resting style, and
that pass applies its normal declarations after the first pass has already
applied its important ones. Avoid combining `!important` with pseudo-class
rules for the same property.
:::

The [CSS property reference](/css_reference) lists every supported property
with its accepted values.
