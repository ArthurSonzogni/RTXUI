# The smallest possible RTXUI application

A component is a class deriving from Component&amp;lt;Derived&amp;gt; whose `view` member holds an HTML template plus a &amp;lt;style&amp;gt; block. `self` selects the component's own root element, and custom properties declared there (--bg, --accent, ...) inherit into every descendant, so a single palette styles the whole tree.

<ExampleTabs src="/wasm/rtxui_example_helloworld.js" :cols="80" :rows="24">
<template #source>

<<< @/../example/helloworld.cpp

</template>
</ExampleTabs>

---

* Source file: [`example/helloworld.cpp`](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/helloworld.cpp)
* Guide: [Relevant Documentation](/guide/hello-world)
* [← Back to Examples Index](/guide/examples)
