# Conditional Rendering

Elements can be included or excluded from the interface based on component
state. A condition is a bound name — a `bool` member or a `const` method
returning `bool` — interpolated with `{...}`. An element renders when the
value is `true` (or `1`).

Templates do not evaluate expressions: comparisons such as
`{status == "loading"}` are not supported. Put the comparison in a computed
method and bind that instead:

```cpp
class Profile : public rtxui::Component<Profile> {
 public:
  std::string status = "loading";

  bool is_loading() const { return status == "loading"; }
  bool is_error() const { return status == "error"; }

  Profile() {
    Bind(status);
    Bind(is_loading);
    Bind(is_error);
  }
  // ...
};
```

## The `<if>` / `<elif>` / `<else>` chain

For block-level branching, wrap the alternatives in consecutive logic tags.
The chain renders the first branch whose condition holds; formatting
whitespace and comments between the tags do not break the chain.

```html
<if condition="{is_loading}">
  <progress value="50" />
</if>
<elif condition="{is_error}">
  <span class="text-error">Failed to fetch profile details.</span>
</elif>
<else>
  <span>Profile loaded.</span>
</else>
```

## The `if` attribute

To toggle a single element without a wrapper tag, put an `if` attribute on
the element itself. When the condition is false the element is not
constructed at all — it has no layout size and receives no events.

```html
<span if="{is_admin}">Delete profile</span>
```

## Choosing between them

- Use the `if` attribute for a single optional element.
- Use `<if>`/`<elif>`/`<else>` when branches are alternatives to each other,
  or when a branch contains several elements.
- To keep an element's space in the layout while hiding it, use CSS
  `visibility: hidden` instead; to remove its space without removing it from
  the DOM, toggle a class that sets `display: none`.

## Demo

<ExampleTabs src="/wasm/rtxui_example_conditional.js">
<template #source>

<<< @/../example/conditional.cpp

</template>
</ExampleTabs>
