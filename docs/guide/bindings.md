# Event Handlers & Input Bindings

RTXUI maps terminal interactions (such as mouse clicks, keyboard presses, or input changes) to registered C++ callback functions.

---

## 1. Registering Callbacks in C++

To bind a member function to template events, register it in `InitReflection()` using the `Bind()` method. To register lambda expressions or standalone functions, import them using the `Import()` method:

```cpp
struct ClickApp : public rtxui::Component<ClickApp> {
  int clicks = 0;

  void Increment() { clicks++; }

  void InitReflection() override {
    ComponentBase::InitReflection();
    Bind(clicks);
    
    // Bind member function callback
    Bind(Increment);
    
    // Import a lambda with parameters
    Import("Reset", [this]() { clicks = 0; });
  }

  std::string_view Setup() override;
};
```

---

## 2. Event Binding Syntaxes

Once registered, you can wire callbacks to layout components in two styles:

### React-style (`onclick`, `onchange`)
Use the event attribute name with curly braces enclosing the callback name:
```html
<button onclick="{Increment}">Increment</button>
```

### Vue-style (`@click`, `@change`)
Use the `@` shorthand prefix followed by the callback name:
```html
<button @click="Increment">Increment</button>
```

---

## 3. Event Modifiers

To intercept specific mouse behaviors (like context menu clicks), you can append a modifier suffix:

```html
<!-- Triggered only on mouse right-click events -->
<button @click.right="ShowContextMenu">Options</button>
```
*   `@click` or `@click.left`: Main trigger callback.
*   `@click.right`: Secondary/context menu trigger.

---

## 4. Parameterized Callbacks

Callbacks can accept arguments from templates. The arguments are received by the C++ lambda as `std::string` parameters. This is extremely useful for index tracking in loops:

```cpp
// Registered in InitReflection
Import("RemoveItem", [this](std::string index_str) {
  size_t index = std::stoull(index_str);
  items.erase(items.begin() + index);
});
```

To invoke a parameterized callback in a template, pass the values or loop variables in parentheses:
```html
<ul>
  <li for="{todo in todo_list}">
    <span>{todo}</span>
    <button @click="RemoveItem({$index})">Delete</button>
  </li>
</ul>
```

---

## 5. Two-Way Data Binding

For interactive input widgets, RTXUI supports automatic two-way data bindings. When a user types text or checks a box, the bound C++ member variable is updated instantly, and programmatically changing the C++ value repaints the input field:

*   **`<input>` / `<textarea>`**: Bind the `value` attribute to a reactive string.
*   **`<checkbox>`**: Bind the `checked` attribute to a reactive boolean.
*   **`<select>`**: Bind the `value` attribute to a reactive string matching the selected option.

```html
<input value="{search_query}" />
<checkbox checked="{is_enabled}">Toggle Features</checkbox>
```

---

## Interactive Demo

Below is the interactive tab view for two-way input field bindings:

<ExampleTabs src="/wasm/rtxui_example_input.js">
<template #source>

<<< @/../example/input.cpp

</template>
</ExampleTabs>
