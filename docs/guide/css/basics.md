# CSS Basics & Selectors

RTXUI components package style templates alongside markup declarations, matching modern component design standards.

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
*   **Pseudo-classes**: Targets interactive states (`:hover`, `:focus`, `:active`) and scrollbars.
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
