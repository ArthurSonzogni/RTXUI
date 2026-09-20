# Value Interpolation & Data Binding

RTXUI matches standard web templates by using compile-time reflection to bind C++ variables directly into your HTML templates.

---

## 1. Text Interpolation

To project dynamic content into your text nodes, wrap the C++ variable or member name in curly braces `{}`. During compilation and render ticks, the values are automatically formatted as strings and injected into the DOM:

```html
<span>Welcome back, {username}!</span>
<div>Current Score: {score}</div>
```

---

## 2. Binding Member Fields

All public member variables inside a component class are registered using the `Bind()` method in the constructor:

```cpp
#include <rtxui/rtxui.hpp>

class ProfileApp : public rtxui::Component<ProfileApp> {
 public:
  std::string username = "Alice";
  int score = 42;

  ProfileApp() {
    Bind(username);
    Bind(score);
  }

  std::string_view Setup() override {
    return R"html(
      <div class="card">
        <span>Username: {username}</span>
        <span>Score: {score}</span>
      </div>
    )html";
  }
};
```

---

## 3. Computed Properties & Methods

You can also bind dynamic evaluations to C++ member functions. This is useful for derived data or complex logic that shouldn't live in the template. Use the same `Bind()` method to register a `const` member function:

```cpp
class CounterApp : public rtxui::Component<CounterApp> {
 public:
  int count = 10;

  int double_count() const { return count * 2; }

  CounterApp() {
    Bind(count);
    Bind(double_count);
  }

  std::string_view Setup() override {
    return R"html(
      <div>
        <span>Count: {count}</span>
        <span>Double: {double_count}</span>
      </div>
    )html";
  }
};
```

---

## 4. Attribute Interpolation

You can bind dynamic variables to HTML attributes (such as element classes, check statuses, or dimensions) in two styles:

### React-style Binding
Include curly braces around the C++ variable name inside the attribute's double-quotes:
```html
<div class="box {color_class}"></div>
<checkbox checked="{is_active}">Active Option</checkbox>
```

### Vue-style Binding
Prefix the attribute name with a colon `:` and pass the raw variable name directly as the value:
```html
<div :title="color_class"></div>
<checkbox :checked="is_active">Active Option</checkbox>
```

> [!NOTE]
> `:class` and `:id` are not supported using Vue-style binding. For classes and IDs, use React-style interpolation instead: `class="{variable}"`.

Both styles perform the same underlying reactive linking, so you can choose the format you prefer.

---

## Interactive Demo

Below is the interactive tab view for a live counter demonstrating simple value binding:

<ExampleTabs src="/wasm/rtxui_example_counter.js">
<template #source>

<<< @/../example/counter.cpp

</template>
</ExampleTabs>
