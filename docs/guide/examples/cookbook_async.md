# Recipe

Recipe: updating the UI from a worker thread.

The UI is single-threaded. A background thread must hand results back through the task runner, which applies them between frames.

<ExampleTabs src="/wasm/rtxui_example_cookbook_async.js" :cols="80" :rows="24">
<template #source>

<<< @/../example/cookbook_async.cpp

</template>
</ExampleTabs>

---

* Source file: [`example/cookbook_async.cpp`](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/cookbook_async.cpp)
* Standalone terminal: <a href="/RTXUI/terminal.html?src=%2FRTXUI%2Fwasm%2Frtxui_example_cookbook_async.js&fullscreen=1" target="_blank" rel="noopener noreferrer">⛶ Open Fullscreen</a>
* Guide: [Relevant Documentation](/guide/cookbook)
* [← Back to Examples Index](/guide/examples)
