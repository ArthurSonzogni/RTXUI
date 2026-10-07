# A virtual list

A virtual list: 100,000 log lines.

`<for ... virtual="">` inside a scroll container renders only the lines around what the container shows, with two spacers standing in for the rest, and renders again as scrolling moves past them. A frame then costs what a screenful of lines does, however long the log.

Try it: scroll with the mouse wheel, or Tab to the log and use the arrows, PageUp/PageDown and Home/End.

<ExampleTabs src="/wasm/rtxui_example_virtual_list.js" :cols="80" :rows="24">
<template #source>

<<< @/../example/virtual_list.cpp

</template>
</ExampleTabs>

---

* Source file: [`example/virtual_list.cpp`](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/virtual_list.cpp)
* Standalone terminal: <a href="/RTXUI/terminal.html?src=%2FRTXUI%2Fwasm%2Frtxui_example_virtual_list.js&fullscreen=1" target="_blank" rel="noopener noreferrer">⛶ Open Fullscreen</a>
* Guide: [Relevant Documentation](/guide/loops)
* [← Back to Examples Index](/guide/examples)
