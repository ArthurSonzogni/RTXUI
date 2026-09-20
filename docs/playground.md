---
layout: page
title: Interactive Playground
---

<div class="playground-page-wrapper">

<div class="playground-hero">
  <h1>Interactive Playground</h1>
  <p>
    An interactive WebAssembly environment running the complete RTXUI C++ engine.
    Edit HTML templates and CSS styles in the left-hand editor; the right-hand pane reparses, diffs, and repaints the terminal output in real time.
  </p>
</div>

<PlaygroundFullscreen />

</div>

<style>
.playground-page-wrapper {
  max-width: 100%;
  padding: 1rem 2rem 4rem 2rem;
  margin: 0 auto;
}

.playground-hero {
  margin-bottom: 1rem;
}

.playground-hero h1 {
  font-size: 2.25rem;
  font-weight: 700;
  color: var(--vp-c-text-1);
  letter-spacing: -0.02em;
  margin-bottom: 0.5rem;
}

.playground-hero p {
  font-size: 1.05rem;
  color: var(--vp-c-text-2);
  max-width: 960px;
  line-height: 1.6;
}

@media (max-width: 768px) {
  .playground-page-wrapper {
    padding: 1rem 1rem 3rem 1rem;
  }
  .playground-hero h1 {
    font-size: 1.75rem;
  }
}
</style>
