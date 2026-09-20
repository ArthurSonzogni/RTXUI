# A complete application driven by the keyboard

A complete application driven by the keyboard.

Where app_dashboard.cpp is about composing layout and state, this one is about the two things a terminal UI lives or dies by: scrolling a list that is longer than the viewport, and moving a selection through it without a mouse. Rows carry `tabindex`, so RTXUI's built-in focus navigation walks them and scrolls the focused one into view; the preview pane re-renders from computed values as the selection moves.

Try it: move with the arrow keys (or j/k via the buttons), Enter to open a directory, Backspace to go up. The list scrolls to follow the selection.

<ExampleTabs src="/wasm/rtxui_example_app_filebrowser.js" :cols="100" :rows="28">
<template #source>

<<< @/../example/app_filebrowser.cpp

</template>
</ExampleTabs>

---

* Source file: [`example/app_filebrowser.cpp`](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/app_filebrowser.cpp)
* Guide: [Relevant Documentation](/guide/scrolling)
* [← Back to Examples Index](/guide/examples)
