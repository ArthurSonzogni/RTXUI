# An inline prompt that stays

An inline prompt that stays: pick dependencies to add, one after the other, below the command that started it, as `cargo add` or `npm install` would.

The prompt is drawn inline, as a Screen is by default, as tall as the list under it, which grows and shrinks as the query filters it. Each pick is written above it with Screen::Print(), where it stays in the terminal's history, and the prompt is ready for the next one. Escape or Ctrl+C ends it with a summary line.

Try it: type "re", choose with Up and Down or the mouse, press Enter. Again.

<ExampleTabs src="/wasm/rtxui_example_inline_prompt.js" :cols="100" :rows="24">
<template #source>

<<< @/../example/inline_prompt.cpp

</template>
</ExampleTabs>

---

* Source file: [`example/inline_prompt.cpp`](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/inline_prompt.cpp)
* Standalone terminal: <a href="/RTXUI/terminal.html?src=%2FRTXUI%2Fwasm%2Frtxui_example_inline_prompt.js&fullscreen=1" target="_blank" rel="noopener noreferrer">⛶ Open Fullscreen</a>
* Guide: [Relevant Documentation](/guide/inline)
* [← Back to Examples Index](/guide/examples)
