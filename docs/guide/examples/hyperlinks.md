# Terminal hyperlinks

Terminal hyperlinks: an `<a href>` to a URL becomes a clickable link.

RTXUI marks the link's text with the OSC 8 escape sequence. kitty, WezTerm, iTerm2, foot, GNOME Terminal and Windows Terminal open it the way each does for links, usually Ctrl- or Cmd-click (plus Shift in some, since the app reads the mouse). Other terminals show the text as usual.

Try it: Ctrl-click a link, or Tab to one and look at the focus style.

<ExampleTabs src="/wasm/rtxui_example_hyperlinks.js" :cols="80" :rows="24">
<template #source>

<<< @/../example/hyperlinks.cpp

</template>
</ExampleTabs>

---

* Source file: [`example/hyperlinks.cpp`](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/hyperlinks.cpp)
* Standalone terminal: <a href="/RTXUI/terminal.html?src=%2FRTXUI%2Fwasm%2Frtxui_example_hyperlinks.js&fullscreen=1" target="_blank" rel="noopener noreferrer">⛶ Open Fullscreen</a>
* Guide: [Relevant Documentation](/html_reference)
* [← Back to Examples Index](/guide/examples)
