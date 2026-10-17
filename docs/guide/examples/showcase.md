# A tour of RTXUI in one application

A tour of RTXUI in one application.

Four pages behind a sidebar: a live dashboard laid out by a CSS grid that reflows with the terminal, a gallery of transitions and keyframe animations, form widgets bound to C++ state, and rich text. The data on the dashboard is fed by a timer; everything that moves on screen is CSS.

Try it: resize the terminal on the Overview page, hover the tiles on Motion, drag the slider on Widgets, select text with the mouse on Text.

<ExampleTabs src="/wasm/rtxui_example_showcase.js" :cols="120" :rows="36">
<template #source>

<<< @/../example/showcase.cpp

</template>
</ExampleTabs>

---

* Source file: [`example/showcase.cpp`](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/showcase.cpp)
* Standalone terminal: <a href="/RTXUI/terminal.html?src=%2FRTXUI%2Fwasm%2Frtxui_example_showcase.js&fullscreen=1" target="_blank" rel="noopener noreferrer">⛶ Open Fullscreen</a>
* Guide: [Relevant Documentation](/guide/css/grid)
* [← Back to Examples Index](/guide/examples)
