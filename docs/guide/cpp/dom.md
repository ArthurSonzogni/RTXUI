# Navigating the DOM

Once templates are parsed, components build a tree of elements. Applications
reach it through `rtxui::ElementHandle`, which `<rtxui/rtxui.hpp>` already
includes.

If all you want is the component rendered at a selector — rather than the
element itself — use `QueryComponent`:

```cpp
if (auto* preview = dynamic_cast<Preview*>(QueryComponent("#preview"))) {
  preview->Reload(text);
}
```

## Query Selector Queries

Elements are found with CSS-style selectors (`#id`, `.class`, or a tag
name). A component queries its own tree with `QueryElement`, and any handle
can query below itself with `QuerySelector`:

```cpp
void ResetScrollbar() {
  QueryElement("#my-list").SetScrollY(0);
}
```

A query that matches nothing returns a null handle. Every method on a null
handle is a no-op returning an empty value, so the line above is safe even when
`#my-list` is not rendered. Test a handle with `if (handle)` when you need to
know.

## Tree Traversal

`RootElement()` returns the root of a component's tree. From any handle:

*   `Parent()`: the parent element, or a null handle at the root.
*   `ChildCount()`: the number of child elements.
*   `ChildAt(index)`: the child element at `index`, or a null handle.
*   `tag()` and `GetAttribute(name)`: what the template declared.

```cpp
void InspectFirstChild(rtxui::ElementHandle parent) {
  if (parent.ChildCount() > 0) {
    rtxui::ElementHandle child = parent.ChildAt(0);
    // Perform operations on the child element...
  }
}
```

The tree follows the template as written: `<slot>` wrappers are transparent,
and text is not counted as a child.

## Lifetime

A handle does not keep its element alive. When a re-render removes the element
— an `<if>` turning false, say — the handle becomes null rather than dangling.
Removed elements are pooled for reuse until the next render, so for one frame
the handle may still point at the detached element (its `Parent()` is null).
Holding a handle across frames is safe, but query again rather than expecting
it to follow a re-created element.

## ABI note

The element itself stays internal: its layout, style and animation state change
between releases. `ElementHandle` is a single pointer and all of its methods
are out-of-line library functions, so a release can change the element freely
and add new handle methods without breaking programs built against an older
one.
