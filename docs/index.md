---
layout: home

hero:
  name: RTXUI
  text: Terminal UI, using C++/HTML/CSS
  image:
    light: /logo-light.png
    dark: /logo-dark.png
    alt: RTXUI Logo
  actions:
    - theme: brand
      text: Get Started
      link: /guide/getting-started
    - theme: alt
      text: Try Playground
      link: /playground

features:
  - title: A real layout engine
    details: Block, inline, flexbox, and grid layout; borders, padding, scrolling, positioning, and transitions — computed in terminal cells by a browser-style pipeline.
  - title: Dead simple C++ reactivity
    details: Component members are ordinary variables. Mutate them in an event handler; the engine detects the change and repaints only what differs. No wrapper types, no setters.
  - title: Built for the terminal
    details: Keyboard navigation (Tab, arrows, Home/End, Escape), mouse and wheel support, Unicode and CJK text, and hot reload of templates while the program runs.
---

<div class="demo-section">
  <h2>UI Components in Plain C++</h2>
  <p class="demo-desc">
    Components inherit from <code>Component&lt;Derived&gt;</code> and declare their UI with HTML templates and CSS styles.
    State members and methods bind directly to template interpolations and event handlers.
  </p>

```cpp
#include <rtxui/rtxui.hpp>

class Counter : public rtxui::Component<Counter> {
 public:
  // State.
  int count = 0;
  std::string label = "Clicks";

  // Derived value: re-evaluated whenever the component re-renders.
  int double_count() const { return count * 2; }

  // Event handler.
  void Increment() { count++; }

  std::string_view view = R"html(
    <span>{label}: {count}</span>
    <span>Double: {double_count}</span>
    <button onclick="Increment">Increment</button>

    <style>
      self {
        display: flex;
        flex-direction: column;
        align-items: center;
        background-color: rgb(22, 27, 34);
        border: solid;
        border-color: rgb(48, 54, 61);
        padding: 1 2;
        gap: 1;
      }
    </style>
  )html";

  Counter() {
    // Explicit bindings (optional when C++26 reflection is enabled).
    Bind(count);
    Bind(label);
    Bind(double_count);
    Bind(Increment);
  }
};
```

</div>

<div class="demo-section">
  <h2 id="interactive-playground">Interactive Playground</h2>
  <p class="demo-desc">
    This live playground compiles RTXUI to WebAssembly and runs it right here in your browser. Edit the HTML and CSS
    in the left-hand editor — every keystroke reparses the template and re-renders the preview on the
    right, using the same engine that powers every RTXUI app.
  </p>
<div class="playground-cta">
  <a href="/RTXUI/terminal.html?src=%2FRTXUI%2Fwasm%2Frtxui_example_playground.js&fullscreen=1" class="playground-btn" target="_blank" rel="noopener noreferrer">⛶ Open Full-Screen Playground &rarr;</a>
</div>

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
  margin-bottom: 1.25rem;
  text-align: center;
  line-height: 1.6;
}
.playground-cta {
  display: flex;
  justify-content: center;
  margin-bottom: 1.5rem;
}
.playground-cta a,
.playground-btn {
  display: inline-flex;
  align-items: center;
  gap: 0.5rem;
  background-color: var(--vp-c-brand-1);
  color: white !important;
  font-weight: 600;
  font-size: 0.95rem;
  padding: 0.6rem 1.4rem;
  border-radius: 8px;
  text-decoration: none;
  transition: background-color 0.2s ease, transform 0.1s ease;
}
.playground-cta a:hover,
.playground-btn:hover {
  background-color: var(--vp-c-brand-2);
  transform: translateY(-1px);
}
</style>
