# Value Interpolation

RTXUI uses compile-time reflection to track class variables as reactive state. These variables are referenced in HTML templates using curly braces `{}`.

## State Variables

Public members in a component class are registered using the `Bind()` macro in the constructor.

```cpp
class CounterApp : public Component<CounterApp> {
 public:
  int count = 0;

  CounterApp() {
    Bind(count);
  }

  std::string_view Setup() override {
    return R"html(
      <div>Count: {count}</div>
    )html";
  }
};
```

## Computed Properties

You can also bind to C++ methods. This is useful for derived data or complex logic that shouldn't live in the template. Use the `Bind()` macro to register a `const` member function.


```cpp
class CounterApp : public Component<CounterApp> {
 public:
  int count = 0;

  CounterApp() {
    Bind(count);
    Bind(double_count);
  }

  int double_count() const { return count * 2; }

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

## Attribute Interpolation

Interpolation works inside attribute values as well. This is the **React-style** of data binding.

```html
<div class="box {color_class}"></div>
<div border-color="{is_active ? 'blue' : 'gray'}"></div>
```

Alternatively, you can use the **Vue-style** shorthand:

```html
<div :class="color_class"></div>
```

<ExampleTabs src="/wasm/rtxui_example_counter.js">
<template #source>

<<< @/../example/counter.cpp

</template>
</ExampleTabs>
