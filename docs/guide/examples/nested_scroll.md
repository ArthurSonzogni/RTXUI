# Nested scroll containers and scroll chaining

An inner container consumes wheel events until it reaches its end, then the event propagates to its parent.

<ExampleTabs src="/wasm/rtxui_example_nested_scroll.js" :cols="80" :rows="24">
<template #source>

<<< @/../example/nested_scroll.cpp

</template>
</ExampleTabs>

---

* Source file: [`example/nested_scroll.cpp`](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/nested_scroll.cpp)
* Guide: [Relevant Documentation](/guide/scrolling)
* [← Back to Examples Index](/guide/examples)
