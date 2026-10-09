# Toast notifications

Toast notifications.

A `<toast>` shows over everything else in a corner of the screen, and closes itself after `duration` milliseconds, or when clicked. Its `open` attribute is two-way bound: the application raises its flag to show it, and the flag falls back to false when the toast closes.

A toast slides in from its side of the screen and back out when it closes. The error toast replaces both animations with its own: it shakes as it arrives, and fades out.

Try it: press Save or Fail, then wait, or click a toast to dismiss it.

<ExampleTabs src="/wasm/rtxui_example_toast.js" :cols="80" :rows="24">
<template #source>

<<< @/../example/toast.cpp

</template>
</ExampleTabs>

---

* Source file: [`example/toast.cpp`](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/toast.cpp)
* Standalone terminal: <a href="/RTXUI/terminal.html?src=%2FRTXUI%2Fwasm%2Frtxui_example_toast.js&fullscreen=1" target="_blank" rel="noopener noreferrer">⛶ Open Fullscreen</a>
* Guide: [Relevant Documentation](/html_reference)
* [← Back to Examples Index](/guide/examples)
