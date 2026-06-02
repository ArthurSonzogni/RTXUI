# Conditional Rendering

Conditional rendering is supported through specialized logic tags or the `if` attribute on standard elements.

## Logic Tags: `<if>`, `<elif>`, `<else>`

You can group multiple elements inside conditional blocks. These blocks must be consecutive (ignoring whitespace and comments) to form a chain.


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
