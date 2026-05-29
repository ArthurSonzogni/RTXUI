---
layout: home

hero:
  name: RTXUI
  text: Reactive Terminal UI
  tagline: Modern C++26 reactive rendering engine for terminals with HTML/CSS-like syntax.
  actions:
    - theme: brand
      text: Get Started
      link: /tutorial
    - theme: alt
      text: C++ API Reference
      link: /cpp_api

features:
  - icon: 🚀
    title: Modern Reactivity
    details: Transparent state observation and computed values via compile-time reflection.
  - icon: 🎨
    title: CSS-like Styling
    details: Native Box model, flex direction, background-color, borders, and scrolling.
  - icon: 💻
    title: WebAssembly Support
    details: Easily compile applications to WebAssembly to run interactive terminal demos in the browser.
---

<div class="demo-section">
  <h2>Try RTXUI in your Browser</h2>
  <p class="demo-desc">
    This live interactive WebAssembly terminal showcases RTXUI's reflection-based reactivity, flexbox layouts, borders, and nested scrolling. 
    Click the "Clicks" button to increment, right-click to decrement, or click the scrollable box and use your keyboard or mouse wheel.
  </p>
  <WasmTerminal src="/wasm/rtxui_example_demo.js" :cols="80" :rows="34" />
</div>

<style>
.demo-section {
  max-width: 860px;
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
