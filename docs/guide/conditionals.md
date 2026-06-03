# Conditional Rendering

In RTXUI, you can display or hide layout elements dynamically based on your component's reactive state. You can control rendering using **block-level logic tags** or **inline attributes**.

---

## 1. Logic Tags: `<if>`, `<elif>`, `<else>`

For block-level conditional splits, wrap your layout nodes inside consecutive conditional tags. These tags form a conditional chain (ignoring any formatting whitespace or comments between them):

```html
<if condition="{status == 'loading'}">
  <progress value="50" />
</if>
<elif condition="{status == 'error'}">
  <span class="text-error">Failed to fetch profile details.</span>
</elif>
<else>
  <span>Successfully loaded database!</span>
</else>
```

---

## 2. Inline Attribute Toggle (`if` / `:if`)

To toggle a single element without wrapping it in a logic tag, use the `if` attribute directly on that tag. The element will only be constructed and rendered if the expression evaluates to `true` (or a non-zero integer).

### React-style inline toggle
```html
<span if="{is_admin}">Delete Profile</span>
```

### Vue-style inline toggle
```html
<span :if="is_admin">Delete Profile</span>
```

---

## Interactive Demo

Below is the interactive tab view demonstrating reactive conditional visibility triggers:

<ExampleTabs src="/wasm/rtxui_example_conditional.js">
<template #source>

<<< @/../example/conditional.cpp

</template>
</ExampleTabs>
