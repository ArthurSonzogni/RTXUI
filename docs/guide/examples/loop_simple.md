# Looping over a std

Looping over a `std::vector<std::string>` with `<for>`.

{$index} gives the current position, which is how a row passes its identity to a parameterized callback.

Try it: add a fruit, then remove one.

<ExampleTabs src="/wasm/rtxui_example_loop_simple.js" :cols="80" :rows="24">
<template #source>

<<< @/../example/loop_simple.cpp

</template>
</ExampleTabs>

---

* Source file: [`example/loop_simple.cpp`](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/loop_simple.cpp)
* Standalone terminal: <a href="/RTXUI/terminal.html?src=%2FRTXUI%2Fwasm%2Frtxui_example_loop_simple.js&fullscreen=1" target="_blank" rel="noopener noreferrer">⛶ Open Fullscreen</a>
* Guide: [Relevant Documentation](/guide/loops)
* [← Back to Examples Index](/guide/examples)
