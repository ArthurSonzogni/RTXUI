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
* Standalone terminal: <a href="/RTXUI/terminal.html?src=%2FRTXUI%2Fwasm%2Frtxui_example_input.js&fullscreen=1" target="_blank" rel="noopener noreferrer">⛶ Open Fullscreen</a>
* Guide: [Relevant Documentation](/guide/forms)
* [← Back to Examples Index](/guide/examples)
