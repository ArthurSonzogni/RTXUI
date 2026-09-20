# Loops & Lists

The `<for>` tag renders its children once per item of a bound C++ container,
such as a `std::vector`.

## The `<for>` tag

Name the bound collection with `each`, and choose the loop variable's name
with `as`:

```cpp
// Bound in the constructor:
std::vector<std::string> fruits = {"Apple", "Banana", "Cherry"};
Bind(fruits);
```

```html
<ul>
  <for each="{fruits}" as="fruit">
    <li>{fruit} (index: {$index})</li>
  </for>
</ul>
```

Inside the loop body:

- `{fruit}` interpolates the item, converted to a string.
- `{$index}` interpolates the zero-based index. It is most useful for
  passing to a parameterized callback, e.g.
  `<button onclick="RemoveItem({$index})">`
  (see [Event Handlers](/guide/bindings)).

When the collection changes — items added, removed, or mutated — the loop's
rendered children are reconciled on the next digest.

This demo loops over a `std::vector<std::string>`, appending with a bound
`<input>` and removing by `{$index}`:

<ExampleTabs src="/wasm/rtxui_example_loop_simple.js">
<template #source>

<<< @/../example/loop_simple.cpp

</template>
</ExampleTabs>

## Keyed loops

By default, collection children reconcile by **position**: the DOM node at index `i` is reused for the `i`-th item. When items reorder or shift, element-local state (keyboard focus, scroll offsets, active CSS transitions) stays locked to the index rather than tracking the item.

Give the loop a `key` to identify items instead:

```html
<for each="{tasks}" as="task" key="{task.id}">
  <div tabindex="0">{task.name}</div>
</for>
```

The key is interpolated per item, exactly like the loop body, so it can be any
expression that names the item — an id field is the usual choice. On the next
digest each item's existing elements are moved to the item's new position and
reused there, carrying their state with them. Focus stays on the task the user
had focused, even if it shifts three rows down.

Keys must be unique within the loop, and stable across frames: keying by
`{$index}` is the same as not keying at all. A loop without `key` keeps the
position matching described above, which stays the cheaper option for a list
that never reorders.

## Collections of structs

A collection of plain values stringifies each item directly. For a collection
of structs, provide a mapper that exposes named fields to the template. The
mapper returns a `ManualStructVisitor` built from a field-name → value map:

```cpp
struct Task {
  std::string name;
  bool completed;
};

std::vector<Task> tasks;

// In the constructor:
Bind(tasks, [](const Task& t) {
  return std::make_shared<ManualStructVisitor>(
      std::map<std::string, std::string, std::less<>>{
          {"name", t.name},
          {"status", t.completed ? "Done" : "Pending"}});
});
```

The template reads the mapped fields with dot notation on the loop variable:

```html
<ul>
  <for each="{tasks}" as="task">
    <li>{task.name} — {task.status}</li>
  </for>
</ul>
```

`BindCollection("name", &collection, mapper)` is equivalent when you want the
template name to differ from the member name.

When compiled with C++26 static reflection (`RTXUI_HAS_REFLECTION`), struct fields are mapped automatically and explicit mappers are not required.

This demo loops over a `std::vector<Task>` through a mapper, toggling each
item's `completed` field from the template:

<ExampleTabs src="/wasm/rtxui_example_loop_complex.js">
<template #source>

<<< @/../example/loop_complex.cpp

</template>
</ExampleTabs>
