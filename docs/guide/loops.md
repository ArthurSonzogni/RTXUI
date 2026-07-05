# Loops & Lists

The `<for>` tag renders its children once per item of a bound C++ container,
such as a `std::vector`.

## The `<for>` tag

Name the bound collection with `each`, and choose the loop variable's name
with `as`:

```cpp
// Bound in the constructor or InitReflection():
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

- `{fruit}` interpolates the current item, converted to a string.
- `{$index}` interpolates the current zero-based index. It is most useful for
  passing to a parameterized callback, e.g.
  `<button onclick="RemoveItem({$index})">`
  (see [Event Handlers](/guide/bindings)).

When the collection changes — items added, removed, or mutated — the loop's
rendered children are reconciled on the next digest.

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

// In the constructor or InitReflection():
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

## Examples

<ExampleTabs src="/wasm/rtxui_example_loop_simple.js">
<template #source>

<<< @/../example/loop_simple.cpp

</template>
</ExampleTabs>

<ExampleTabs src="/wasm/rtxui_example_loop.js">
<template #source>

<<< @/../example/loop.cpp

</template>
</ExampleTabs>

A larger interactive list with add/remove:

<ExampleTabs src="/wasm/rtxui_example_loop_complex.js">
<template #source>

<<< @/../example/loop_complex.cpp

</template>
</ExampleTabs>
