# Selector lists

Selector lists: :is() and :where().

`:is(.info, .ok, .warn, .error)` writes one rule for four kinds of badge. `:where()` matches the same way but weighs nothing, so the defaults it sets lose to any other rule: `.custom` overrides `:where(.badge)` although it comes first in the stylesheet, where it would otherwise lose the tie.

Try it: hover the badges and Tab between them.

<ExampleTabs src="/wasm/rtxui_example_selector_lists.js" :cols="80" :rows="24">
<template #source>

<<< @/../example/selector_lists.cpp

</template>
</ExampleTabs>

---

* Source file: [`example/selector_lists.cpp`](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/selector_lists.cpp)
* Standalone terminal: <a href="/RTXUI/terminal.html?src=%2FRTXUI%2Fwasm%2Frtxui_example_selector_lists.js&fullscreen=1" target="_blank" rel="noopener noreferrer">⛶ Open Fullscreen</a>
* Guide: [Relevant Documentation](/guide/css/basics)
* [← Back to Examples Index](/guide/examples)
