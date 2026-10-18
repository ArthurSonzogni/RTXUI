# An inline application

An inline application: a package installer drawn below the command that started it, as `apt install` is, rather than over the whole terminal.

Drawn inline, as a Screen is by default, the interface keeps the shell's lines above it and is only as tall as its content. Screen::Print() writes each installed package above it, where it stays in the terminal's history. Once done, the interface is a one-line summary, left behind for the shell to go on below.

Try it: press p to pause, q to cancel, or click them.

<ExampleTabs src="/wasm/rtxui_example_inline_install.js" :cols="100" :rows="24">
<template #source>

<<< @/../example/inline_install.cpp

</template>
</ExampleTabs>

---

* Source file: [`example/inline_install.cpp`](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/inline_install.cpp)
* Standalone terminal: <a href="/RTXUI/terminal.html?src=%2FRTXUI%2Fwasm%2Frtxui_example_inline_install.js&fullscreen=1" target="_blank" rel="noopener noreferrer">⛶ Open Fullscreen</a>
* Guide: [Relevant Documentation](/guide/inline)
* [← Back to Examples Index](/guide/examples)
