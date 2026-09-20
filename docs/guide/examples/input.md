# The `<input>` component with two-way binding

The `<input>` component with two-way binding.

Editing the field writes straight back into the bound std::string, and the interpolated value below updates on the same frame.

<ExampleTabs src="/wasm/rtxui_example_input.js" :cols="80" :rows="24">
<template #source>

<<< @/../example/input.cpp

</template>
</ExampleTabs>

---

* Source file: [`example/input.cpp`](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/input.cpp)
* Guide: [Relevant Documentation](/guide/forms)
* [← Back to Examples Index](/guide/examples)
