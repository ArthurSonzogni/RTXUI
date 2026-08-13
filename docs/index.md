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
      link: /guide/examples/playground

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
    <div class="panel">
      <span>{label}: {count}</span>
      <span>Double: {double_count}</span>
      <button onclick="Increment">Increment</button>
    </div>

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
  <h2>Interactive Playground</h2>
  <p class="demo-desc">
    This live playground compiles RTXUI to WebAssembly and runs it right here in your browser. Edit the HTML and CSS
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
