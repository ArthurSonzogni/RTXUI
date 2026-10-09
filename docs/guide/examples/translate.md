# Moving boxes with translate

Moving boxes with translate.

`translate` moves a box as it is drawn and clicked, after layout, so nothing around it moves, and its percentages are of the box's own size. The cards lift a cell when hovered or focused, through a transition. Each task slides out of its list once done, an animation whose end, `onanimationend`, removes it.

Try it: hover or Tab through the cards and click one, then click the tasks, or focus one and press Enter.

<ExampleTabs src="/wasm/rtxui_example_translate.js" :cols="80" :rows="24">
<template #source>

<<< @/../example/translate.cpp

</template>
</ExampleTabs>

---

* Source file: [`example/translate.cpp`](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/translate.cpp)
* Standalone terminal: <a href="/RTXUI/terminal.html?src=%2FRTXUI%2Fwasm%2Frtxui_example_translate.js&fullscreen=1" target="_blank" rel="noopener noreferrer">⛶ Open Fullscreen</a>
* Guide: [Relevant Documentation](/guide/css/animations)
* [← Back to Examples Index](/guide/examples)
