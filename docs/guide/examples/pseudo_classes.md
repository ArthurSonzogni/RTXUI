# The interactive pseudo-classes: :hover, :focus and :active

Rules can be nested inside their parent with `&`, exactly as in modern CSS, so an element's interactive states live next to its base declarations.

Try it: move the mouse over the buttons, Tab between them, and hold the mouse button down to see all three states.

<ExampleTabs src="/wasm/rtxui_example_pseudo_classes.js" :cols="80" :rows="24">
<template #source>

<<< @/../example/pseudo_classes.cpp

</template>
</ExampleTabs>

---

* Source file: [`example/pseudo_classes.cpp`](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/pseudo_classes.cpp)
* Guide: [Relevant Documentation](/guide/css/animations)
* [← Back to Examples Index](/guide/examples)
