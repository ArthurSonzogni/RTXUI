# Recipe: updating the UI from a worker thread

The UI is single-threaded. A background thread must hand results back through the task runner, which applies them between frames.

<ExampleTabs src="/wasm/rtxui_example_cookbook_async.js" :cols="80" :rows="24">
<template #source>

<<< @/../example/cookbook_async.cpp

</template>
</ExampleTabs>

---

* Source file: [`example/cookbook_async.cpp`](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/cookbook_async.cpp)
* Guide: [Relevant Documentation](/guide/cookbook)
* [← Back to Examples Index](/guide/examples)
