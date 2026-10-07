# Recipe

Recipe: a command palette.

Ctrl+P opens a `<dialog>` holding an `<input>` that takes the focus through `autofocus`. Each keystroke filters the commands by fuzzy match; Up and Down move the highlight, Enter runs the highlighted command, Escape closes.

Try it: press Ctrl+P (or click the button), type "th", then press Enter.

<ExampleTabs src="/wasm/rtxui_example_cookbook_command_palette.js" :cols="80" :rows="24">
<template #source>

<<< @/../example/cookbook_command_palette.cpp

</template>
</ExampleTabs>

---

* Source file: [`example/cookbook_command_palette.cpp`](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/cookbook_command_palette.cpp)
* Standalone terminal: <a href="/RTXUI/terminal.html?src=%2FRTXUI%2Fwasm%2Frtxui_example_cookbook_command_palette.js&fullscreen=1" target="_blank" rel="noopener noreferrer">⛶ Open Fullscreen</a>
* Guide: [Relevant Documentation](/guide/cookbook)
* [← Back to Examples Index](/guide/examples)
