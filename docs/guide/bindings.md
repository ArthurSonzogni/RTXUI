# Event Bindings

RTXUI allows you to bind user interactions (like mouse clicks or keyboard events) to C++ callback functions.

## Binding Callbacks

Member functions can be bound using the unified `Bind()` macro. You can also import standard functions or lambda expressions using `Import()`.

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

```html
<button onclick="Increment">Increment</button>
```

### 2. Vue-style (`@click`, `@change`, etc.)

```html
<button @click="Decrement">Decrement</button>
```

## Parameterized Callbacks

You can pass arguments to your callbacks from the template. The argument is received as a `std::string`.

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
  InputApp() { Bind(my_text); }

  std::string_view Setup() override {
    return R"html(
      <input value="{my_text}" />
      <div>Value: {my_text}</div>
    )html";
  }
};
```

<ExampleTabs src="/wasm/rtxui_example_input.js">
<template #source>

<<< @/../example/input.cpp

</template>
</ExampleTabs>
