# A tree view

A tree view.

Each `<tree-item>` is a focusable row with a `label`, and the `<tree-item>`s nested inside it are its children, shown indented while it is `open`. Clicking a branch, or pressing Enter on it, toggles it; Right opens and Left closes the focused branch. A leaf's own `onclick` runs when it is picked.

Try it: Tab into the tree, then use the arrows and Enter.

<ExampleTabs src="/wasm/rtxui_example_tree.js" :cols="80" :rows="24">
<template #source>

<<< @/../example/tree.cpp

</template>
</ExampleTabs>

---

* Source file: [`example/tree.cpp`](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/tree.cpp)
* Standalone terminal: <a href="/RTXUI/terminal.html?src=%2FRTXUI%2Fwasm%2Frtxui_example_tree.js&fullscreen=1" target="_blank" rel="noopener noreferrer">⛶ Open Fullscreen</a>
* Guide: [Relevant Documentation](/html_reference)
* [← Back to Examples Index](/guide/examples)
