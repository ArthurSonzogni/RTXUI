# CSS keyframe animations

CSS keyframe animations.

`@keyframes` names a sequence of styles, and `animation` plays one on an element: for how long, how many times, in which direction, and with which easing. Unlike a transition, it needs no change of state to start, which suits indicators that move on their own: a pulsing status light, a loading bar, a blinking cursor.

Try it: click the button, or Tab to it and press Enter, to pause and resume every animation.

<ExampleTabs src="/wasm/rtxui_example_keyframes.js" :cols="80" :rows="24">
<template #source>

<<< @/../example/keyframes.cpp

</template>
</ExampleTabs>

---

* Source file: [`example/keyframes.cpp`](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/keyframes.cpp)
* Standalone terminal: <a href="/RTXUI/terminal.html?src=%2FRTXUI%2Fwasm%2Frtxui_example_keyframes.js&fullscreen=1" target="_blank" rel="noopener noreferrer">⛶ Open Fullscreen</a>
* Guide: [Relevant Documentation](/guide/css/animations)
* [← Back to Examples Index](/guide/examples)
