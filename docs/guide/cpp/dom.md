# Navigating the DOM

Once templates are parsed, components build an underlying tree of `rtxui::Element` nodes.

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
