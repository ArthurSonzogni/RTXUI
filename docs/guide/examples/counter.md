# Reactive state and `{...}` interpolation

Reactive state and `{...}` interpolation.

Bind(count) registers a member as reactive state: the DOM is patched whenever it changes. Bind() on a const method registers a computed value that is re-evaluated from that state, and Bind() on a plain method registers an `onclick` handler.

Try it: click the buttons, or Tab to them and press Enter.

<ExampleTabs src="/wasm/rtxui_example_counter.js" :cols="80" :rows="24">
<template #source>

<<< @/../example/counter.cpp

</template>
</ExampleTabs>

---

* Source file: [`example/counter.cpp`](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/counter.cpp)
* Standalone terminal: <a href="/RTXUI/terminal.html?src=%2FRTXUI%2Fwasm%2Frtxui_example_counter.js&fullscreen=1" target="_blank" rel="noopener noreferrer">⛶ Open Fullscreen</a>
* Guide: [Relevant Documentation](/guide/interpolation)
* [← Back to Examples Index](/guide/examples)
