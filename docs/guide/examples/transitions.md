# CSS transitions

CSS transitions.

`transition` interpolates a property between its old and new computed value whenever a rule stops or starts matching -- here, when :hover applies. Each property can carry its own duration and easing function.

Try it: hover each card and watch them settle at different speeds.

<ExampleTabs src="/wasm/rtxui_example_transitions.js" :cols="80" :rows="24">
<template #source>

<<< @/../example/transitions.cpp

</template>
</ExampleTabs>

---

* Source file: [`example/transitions.cpp`](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/transitions.cpp)
* Standalone terminal: <a href="/RTXUI/terminal.html?src=%2FRTXUI%2Fwasm%2Frtxui_example_transitions.js&fullscreen=1" target="_blank" rel="noopener noreferrer">⛶ Open Fullscreen</a>
* Guide: [Relevant Documentation](/guide/css/animations)
* [← Back to Examples Index](/guide/examples)
