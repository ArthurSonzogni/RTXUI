# Conditional Rendering

Conditional rendering is supported through specialized logic tags or the `if` attribute on standard elements.

## Logic Tags: `<if>`, `<elif>`, `<else>`

You can group multiple elements inside conditional blocks. These blocks must be consecutive (ignoring whitespace and comments) to form a chain.

```cpp
class ConditionalApp : public Component<ConditionalApp> {
 public:
  int mode = 0; // 0: Home, 1: Settings, 2: About

  // Computed properties
  bool is_home() const { return mode == 0; }
  bool is_settings() const { return mode == 1; }

  std::string_view view = R"html(
      <div>
        <if condition="{is_home}">
          <h1>Welcome Home!</h1>
        </if>
        <elif condition="{is_settings}">
          <h1>Settings</h1>
        </elif>
        <else>
          <h1>About</h1>
        </else>
      </div>
    )html";

  ConditionalApp() {
    Bind(mode);
    Bind(is_home);
    Bind(is_settings);
  }
};
```

## The `if` Attribute

The `if` attribute specifies conditions for single elements. Elements are rendered when the expression evaluates to `true` (or `1`).

```html
<span if="{is_home}">Home Page Footer</span>
```

Alternatively, you can use the **Vue-style** shorthand:

```html
<span :if="is_home">Home Page Footer</span>
```

<ExampleTabs src="/wasm/rtxui_example_conditional.js">
<template #source>

<<< @/../example/conditional.cpp

</template>
</ExampleTabs>
