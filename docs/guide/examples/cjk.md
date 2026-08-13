# Double-width text

CJK ideographs and emoji occupy two terminal cells. Layout measures text in cells, not code points, so alignment holds for mixed-width content.

<ExampleTabs src="/wasm/rtxui_example_cjk.js" :cols="80" :rows="24">
<template #source>

<<< @/../example/cjk.cpp

</template>
</ExampleTabs>

---

* Source file: [`example/cjk.cpp`](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/cjk.cpp)
* Guide: [Relevant Documentation](/guide/unicode)
* [← Back to Examples Index](/guide/examples)
