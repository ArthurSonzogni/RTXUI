# Event Handlers & Input Bindings

RTXUI maps terminal interactions (such as mouse clicks, keyboard presses, or input changes) to registered C++ callback functions.

---

## 1. Registering Callbacks in C++

To bind a member function to template events, register it in your component constructor using the `Bind()` method. To register lambda expressions or standalone functions, import them using the `Import()` method:

```cpp
struct ClickApp : public rtxui::Component<ClickApp> {
  int clicks = 0;

  void Increment() { clicks++; }

  ClickApp() {
    Bind(clicks);
    
    // Bind member function callback
    Bind(Increment);
    
    // Import a lambda with parameters
    Import("Reset", [this]() { clicks = 0; });
  }

  std::string_view Setup();
};
```

---

## 2. Event Attributes

Once registered, a callback is wired to an element with an `on<event>`
attribute, as in HTML. The attribute value is the name of a bound callback:
```html
<button onclick="Increment">Increment</button>
```

---

## 3. Right Click

`onclick` takes the left click. `oncontextmenu` takes the right click:

```html
<button oncontextmenu="ShowContextMenu">Options</button>
```

---

## 4. Parameterized Callbacks

Callbacks can accept arguments from templates. The arguments are received by the C++ lambda as `std::string` parameters. This is extremely useful for index tracking in loops:

```cpp
// Registered in constructor
Import("RemoveItem", [this](std::string index_str) {
  size_t index = std::stoull(index_str);
  items.erase(items.begin() + index);
});
```

To invoke a parameterized callback in a template, pass a value or an
interpolated loop variable in parentheses. Inside a
[`<for>` loop](/guide/loops), `{$index}` interpolates to the current item's
index:
```html
<ul>
  <for each="{todo_list}" as="todo">
    <li>
      <span>{todo}</span>
      <button onclick="RemoveItem({$index})">Delete</button>
    </li>
  </for>
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
