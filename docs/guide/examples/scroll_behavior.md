# scroll-behavior

scroll-behavior: smooth versus auto.

scroll-behavior decides how a box scrolls to show something it was asked to: an anchor link's target, or an element focused from the keyboard. `smooth` glides there, `auto` jumps. The mouse wheel and the scrolling keys always scroll at once, whatever the box says.

Try it: click the jump links above each box, or Tab through the items, and compare how each column travels.

<ExampleTabs src="/wasm/rtxui_example_scroll_behavior.js" :cols="80" :rows="24">
<template #source>

<<< @/../example/scroll_behavior.cpp

</template>
</ExampleTabs>

---

* Source file: [`example/scroll_behavior.cpp`](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/scroll_behavior.cpp)
* Standalone terminal: <a href="/RTXUI/terminal.html?src=%2FRTXUI%2Fwasm%2Frtxui_example_scroll_behavior.js&fullscreen=1" target="_blank" rel="noopener noreferrer">⛶ Open Fullscreen</a>
* Guide: [Relevant Documentation](/guide/scrolling)
* [← Back to Examples Index](/guide/examples)
