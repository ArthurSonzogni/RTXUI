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

The CSS parser supports a subset of standard selectors:
*   **Tag Selectors**: Targets specific tags (e.g. `div { margin: 1; }`).
*   **Class Selectors**: Targets class names (e.g. `.card { padding: 1; }`).
*   **ID Selectors**: Targets unique identifiers (e.g. `#submit-btn { background-color: green; }`).
*   **Pseudo-classes**: Focus states and hovers (e.g., `:hover`, `:focus`).

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
