# Event Bindings

RTXUI binds user interactions (like mouse clicks or keyboard events) to C++ callback functions.

## Binding Callbacks

Member functions are registered using the `Bind()` macro. Standard functions or lambda expressions are imported using `Import()`.

```cpp
CounterApp() {
  Bind(Increment);
  Bind(Decrement);
}

void Increment() { count++; }
void Decrement() { count--; }
```

## Binding in Templates

RTXUI supports two styles for binding events:

### 1. React-style (`onclick`, `onchange`, etc.)

Use the full attribute name with curly braces around the handler.

```html
<button onclick="{Increment}">Increment</button>
```

### 2. Vue-style (`@click`, `@change`, etc.)

Use the `@` shorthand without braces.

```html
<button @click="Decrement">Decrement</button>
```

## Parameterized Callbacks

Callbacks can receive arguments from the template as `std::string` parameters.

```cpp
Import("RemoveItem", [this](std::string index_str) {
  size_t index = std::stoull(index_str);
  items.erase(items.begin() + index);
});
```

Binding in template:
```html
<button @click="RemoveItem({$index})">Remove</button>
```

## Two-Way Data Binding

The `<input>`, `<textarea>`, `<checkbox>`, and `<select>` elements support two-way data binding. When the user modifies the element, the bound C++ variable is updated automatically.

```cpp
class InputApp : public Component<InputApp> {
 public:
  std::string my_text = "Hello World";

  std::string_view view = R"html(
      <input value="{my_text}" />
      <div>Value: {my_text}</div>
    )html";

  InputApp() { Bind(my_text); }
};
```

<ExampleTabs src="/wasm/rtxui_example_input.js">
<template #source>

<<< @/../example/input.cpp

</template>
</ExampleTabs>
