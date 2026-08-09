---
layout: home

hero:
  name: RTXUI
  text: Terminal interfaces, written like web pages
  tagline: Describe your UI with HTML templates and CSS. Drive it with plain C++ state. RTXUI renders it in the terminal — or, via WebAssembly, in the browser.
  actions:
    - theme: brand
      text: Get Started
      link: /guide/getting-started
    - theme: alt
      text: Hello World
      link: /guide/hello-world

features:
  - title: A real layout engine
    details: Block, inline, flexbox, and grid layout; borders, padding, scrolling, positioning, and transitions — computed in terminal cells by a browser-style pipeline.
  - title: Plain C++ state
    details: Component members are ordinary variables. Mutate them in an event handler; the engine detects the change and repaints only what differs. No wrapper types, no setters.
  - title: Built for the terminal
    details: Keyboard navigation (Tab, arrows, Home/End, Escape), mouse and wheel support, Unicode and CJK text, and hot reload of templates while the program runs.
---

<div class="demo-section">
  <h2>A Real Application</h2>
  <p class="demo-desc">
    A service-health dashboard in a single file — a bound <code>std::vector</code> of structs,
    computed values, conditional rendering, flex layout and transitions. Click a row to select it,
    filter the list, and restart an unhealthy service.
  </p>

<ExampleTabs src="/wasm/rtxui_example_app_dashboard.js" :cols="100" :rows="28">
<template #source>

<<< @/../example/app_dashboard.cpp

</template>
</ExampleTabs>

</div>

<div class="demo-section">
  <h2>Try RTXUI in your Browser</h2>
  <p class="demo-desc">
    This live playground compiles RTXUI to WebAssembly and runs it right here. Edit the HTML and CSS
    in the left-hand editor — every keystroke reparses the template and re-renders the preview on the
    right, using the same engine that powers every RTXUI app.
  </p>

<ExampleTabs src="/wasm/rtxui_example_playground.js" :cols="120" :rows="32">
<template #source>

<<< @/../example/playground.cpp

</template>
</ExampleTabs>

</div>

<style>
.demo-section {
  max-width: 1120px;
  margin: 4rem auto 0 auto;
  padding: 0 1.5rem;
}
.demo-section h2 {
  font-size: 1.8rem;
  font-weight: 600;
  margin-bottom: 0.5rem;
  text-align: center;
  color: var(--vp-c-text-1);
}
.demo-section .demo-desc {
  font-size: 1rem;
  color: var(--vp-c-text-2);
  margin-bottom: 2rem;
  text-align: center;
  line-height: 1.6;
}
</style>
