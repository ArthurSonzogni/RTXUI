# Focus styling

Focus styling: :focus-within, :focus-visible and currentColor.

A group lights up while any field inside it has the focus (:focus-within). The buttons draw a focus ring when Tab reaches them, but not when they are clicked (:focus-visible). Every border is `currentColor`, so it follows the text color through each state and transition without repeating it.

Try it: Tab through the fields and buttons, then click a button.

<ExampleTabs src="/wasm/rtxui_example_focus_states.js" :cols="80" :rows="24">
<template #source>

<<< @/../example/focus_states.cpp

</template>
</ExampleTabs>

---

* Source file: [`example/focus_states.cpp`](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/focus_states.cpp)
* Standalone terminal: <a href="/RTXUI/terminal.html?src=%2FRTXUI%2Fwasm%2Frtxui_example_focus_states.js&fullscreen=1" target="_blank" rel="noopener noreferrer">⛶ Open Fullscreen</a>
* Guide: [Relevant Documentation](/guide/css/basics)
* [← Back to Examples Index](/guide/examples)
