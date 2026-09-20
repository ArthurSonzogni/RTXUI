# Looping over a collection of structs

Looping over a collection of structs.

A mapper exposes each struct's fields to the template, which reads them with dot notation on the loop variable.

Try it: toggle a task to see only that row re-render.

<ExampleTabs src="/wasm/rtxui_example_loop_complex.js" :cols="80" :rows="24">
<template #source>

<<< @/../example/loop_complex.cpp

</template>
</ExampleTabs>

---

* Source file: [`example/loop_complex.cpp`](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/loop_complex.cpp)
* Guide: [Relevant Documentation](/guide/loops)
* [← Back to Examples Index](/guide/examples)
