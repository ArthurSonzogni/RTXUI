# Loops & Lists

To render lists of elements from a bound C++ container, RTXUI supports loops. You can loop over standard collections like `std::vector` using **block-level `<for>` tags** or the **inline `for` attribute**.

---

## 1. Block-Level Loops (`<for>`)

To iterate over a bound collection and render a block of children, use the `<for>` tag. Define the target collection with `each` and specify the element variable name with `as`:

```cpp
// Bound in InitReflection
std::vector<std::string> fruits = {"Apple", "Banana", "Cherry"};
Bind(fruits);
```

```html
<ul>
  <for each="{fruits}" as="fruit">
    <li>{fruit} (Index: {$index})</li>
  </for>
</ul>
```
*   **`$index`**: A built-in iteration variable representing the current index (0-indexed).

---

## 2. Inline Loops (`for` attribute)

To repeat a single element without wrapping it in a `<for>` block, use the `for` attribute directly on the tag.

### React-style inline loop
```html
<ul>
  <li for="{fruit in fruits}">{fruit}</li>
</ul>
```

### Vue-style inline loop
```html
<ul>
  <li :for="fruit in fruits">{fruit}</li>
</ul>
```

---

## 3. Complex Objects & Mappers

For containers holding complex structs or classes, you must supply a custom mapper function during `BindCollection()` to expose object properties to the template:

```cpp
struct TodoItem {
  std::string title;
  bool completed;
};

// Inside your Component class definition:
std::vector<TodoItem> items;

void InitReflection() override {
  ComponentBase::InitReflection();
  
  // Provide mapper mapper function returning a field dictionary
  BindCollection("items", &items, [](const TodoItem& item) {
    return rtxui::FieldMap{
      {"title", item.title},
      {"completed", item.completed}
    };
  });
}
```

Template usage:
```html
<ul>
  <li for="{todo in items}">
    <span>{todo.title}</span> - Status: {todo.completed ? 'Done' : 'Pending'}
  </li>
</ul>
```

---

## Simple Loop Example

<ExampleTabs src="/wasm/rtxui_example_loop_simple.js">
<template #source>

<<< @/../example/loop_simple.cpp

</template>
</ExampleTabs>

---

## Advanced Loop Example

<ExampleTabs src="/wasm/rtxui_example_loop.js">
<template #source>

<<< @/../example/loop.cpp

</template>
</ExampleTabs>

---

## Interactive Demo

Below is the interactive tab view demonstrating collection loops:

<ExampleTabs src="/wasm/rtxui_example_loop_complex.js">
<template #source>

<<< @/../example/loop_complex.cpp

</template>
</ExampleTabs>
