# Inspecting the input events

Inspecting the input events: every key and mouse action the terminal sends.

The root component overrides OnEvent(), which receives each event no focused element consumed. It reads each one as an Event::Keyboard or an Event::Mouse, shows the fields of the latest one, and lists the previous ones, newest first. It then passes the event on, so Escape and Ctrl+C still quit.

Try it: type, press arrows and function keys, move the mouse, click, scroll.

<ExampleTabs src="/wasm/rtxui_example_events.js" :cols="100" :rows="28">
<template #source>

<<< @/../example/events.cpp

</template>
</ExampleTabs>

---

* Source file: [`example/events.cpp`](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/events.cpp)
* Standalone terminal: <a href="/RTXUI/terminal.html?src=%2FRTXUI%2Fwasm%2Frtxui_example_events.js&fullscreen=1" target="_blank" rel="noopener noreferrer">⛶ Open Fullscreen</a>
* Guide: [Relevant Documentation](/guide/cpp/lifecycle)
* [← Back to Examples Index](/guide/examples)
