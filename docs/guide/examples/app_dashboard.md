# A complete application, rather than a demo of one property

A complete application, rather than a demo of one property.

Everything here has appeared on its own elsewhere in example/ -- reactive state, computed values, `<for>` over a collection of structs, conditional rendering, flexbox, grid, transitions and custom properties. This file is about how they compose into something you would actually ship.

Try it: click a service row to select it, use the filter buttons to narrow the list, and press Restart to watch a row transition back to healthy.

<ExampleTabs src="/wasm/rtxui_example_app_dashboard.js" :cols="100" :rows="28">
<template #source>

<<< @/../example/app_dashboard.cpp

</template>
</ExampleTabs>

---

* Source file: [`example/app_dashboard.cpp`](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/app_dashboard.cpp)
* Guide: [Relevant Documentation](/reactivity)
* [← Back to Examples Index](/guide/examples)
