# Value Interpolation & Data Binding

RTXUI binds C++ members into HTML templates, as web frameworks bind their state: register a member with `Bind()`, then use it by name in the template.

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

  std::string_view Setup() {
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

  std::string_view Setup() {
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

Bound values are written into attributes (classes, check states, dimensions)
with the same braces as in text, and can be mixed with literal text:
```html
<div class="box {color_class}"></div>
<checkbox checked="{is_active}">Active Option</checkbox>
```

An attribute whose whole value is one `{name}` is also bound in the other
direction where the element supports it: see
[Two-Way Data Binding](/guide/bindings#_5-two-way-data-binding).

---

## Interactive Demo

Below is the interactive tab view for a live counter demonstrating simple value binding:

<ExampleTabs src="/wasm/rtxui_example_counter.js">
<template #source>

<<< @/../example/counter.cpp

</template>
</ExampleTabs>
