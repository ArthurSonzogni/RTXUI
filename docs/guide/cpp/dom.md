# Navigating the DOM

Once templates are parsed, components build an underlying tree of `rtxui::Element` nodes.

DOM access is opt-in, so that programs which never need it don't pay for the
header:

```cpp
#include <rtxui/dom/element.hpp>
```

If all you want is the component rendered at a selector — rather than the
element itself — `QueryComponent` avoids the DOM header entirely:

```cpp
if (auto* preview = dynamic_cast<Preview*>(QueryComponent("#preview"))) {
  preview->Reload(text);
}
```

## Query Selector Queries

Elements are found with CSS-style selectors (`#id`, `.class`, or a tag
name) using `QuerySelector` on any `Element`. A component reaches its own
tree through `Root()`:

```cpp
void ResetScrollbar() {
  rtxui::Element* element = Root()->QuerySelector("#my-list");
  if (element) {
    element->set_scroll_y(0);
  }
}
```

## Tree Traversal

The `rtxui::Element` class provides family-style navigation API:

*   `Parent()`: Get a pointer to the parent element node.
*   `ChildCount()`: Get the number of immediate child nodes.
*   `ChildAt(index)`: Access the child node at a specific index.

```cpp
void InspectFirstChild(rtxui::Element* parent) {
  if (parent && parent->ChildCount() > 0) {
    rtxui::Element* child = parent->ChildAt(0);
    // Perform operations on the child element...
  }
}
```

## ABI note

`Element` is the one public type whose *layout* is part of the ABI: it holds
its computed styles and scroll state as members, so adding or reordering them
changes the ABI even though the methods are unchanged. Everything else in the
public API either hides its state behind a pointer (`Screen`) or is a value
type.

That means a program compiled against one release and run against another with
a different `SOVERSION` must be rebuilt — which the versioned SONAME enforces
rather than leaving to chance. Because `Element` exposes its layout members directly, changes across shared-library `SOVERSION` releases require rebuilding callers.
