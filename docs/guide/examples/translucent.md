# Translucent colors over the terminal's own background

Translucent colors over the terminal's own background.

Nothing here paints a solid background: the panels are tints with an alpha channel. RTXUI asks the terminal for its background color on startup (OSC 11) and blends the tints with it, so they look right on a light theme as on a dark one. The rest of the screen is left to the terminal, so a translucent terminal window still shows through.

Try it: run it in a dark and in a light terminal theme.

<ExampleTabs src="/wasm/rtxui_example_translucent.js" :cols="80" :rows="24">
<template #source>

<<< @/../example/translucent.cpp

</template>
</ExampleTabs>

---

* Source file: [`example/translucent.cpp`](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/translucent.cpp)
* Standalone terminal: <a href="/RTXUI/terminal.html?src=%2FRTXUI%2Fwasm%2Frtxui_example_translucent.js&fullscreen=1" target="_blank" rel="noopener noreferrer">⛶ Open Fullscreen</a>
* Guide: [Relevant Documentation](/css_reference)
* [← Back to Examples Index](/guide/examples)
