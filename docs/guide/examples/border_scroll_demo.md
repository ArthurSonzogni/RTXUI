# Borders and scrollbars sharing an edge

A scrollbar is laid out inside the border box, so it has to coexist with the border. Switch border styles to see each combination.

<ExampleTabs src="/wasm/rtxui_example_border_scroll_demo.js" :cols="80" :rows="24">
<template #source>

<<< @/../example/border_scroll_demo.cpp

</template>
</ExampleTabs>

---

* Source file: [`example/border_scroll_demo.cpp`](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/border_scroll_demo.cpp)
* Guide: [Relevant Documentation](/guide/css/box-model)
* [← Back to Examples Index](/guide/examples)
