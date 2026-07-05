# Hello World

A complete RTXUI program is a component class and three lines of `main()`.
This page walks through the smallest one, piece by piece. You can try the
result below — it is this exact program compiled to WebAssembly.

<ExampleTabs src="/wasm/rtxui_example_helloworld.js">
<template #source>

<<< @/../example/helloworld.cpp

</template>
</ExampleTabs>

## The component

```cpp
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class HelloWorldApp : public Component<HelloWorldApp> {
 public:
  std::string_view view = R"html(
      <div class="card">
        Hello World from RTXUI!
      </div>
      <style>
        self {
          display: block;
          padding: 1;
          background-color: rgb(18, 18, 18);
        }
        .card {
          border: tall;
          border-color: rgb(59, 130, 246);
          padding: 1;
          color: rgb(241, 245, 249);
        }
      </style>
    )html";
};
```

Every piece of an RTXUI interface is a **component**: a class deriving from
`Component<Derived>` (the class passes itself as the template argument, which
is how the engine gains typed access to its members).

A component describes its appearance in a `view` member: an HTML-like
template held in a raw string literal. The `R"html( ... )html"` delimiters
let the template span lines without escaping; editors and the
[hot-reload](/guide/hot-reload) tooling recognize the `html` tag.

The template here contains two parts:

- **Structure** — a single `<div>` with a text child. Tags are components
  too: `<div>`, `<button>`, `<input>` and the rest of the
  [built-in elements](/html_reference) can be composed freely with your own.
- **Style** — a `<style>` block using CSS syntax. The `self` selector styles
  the component's own root. Lengths are in terminal cells, so `padding: 1`
  is one character cell of padding; colors accept `rgb()`, hex, and named
  forms. `border: tall` picks one of the terminal-friendly
  [border styles](/css_reference).

The scoping rule: a component's `<style>` block applies to that component's
own template, so class names like `card` cannot collide across components.

This component has no state and no event handlers, so it needs no
constructor. When it has some, they are registered there — the
[reactivity page](/reactivity) covers that next step.

## The main function

```cpp
int main() {
  auto app = Ref<HelloWorldApp>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
```

- `Ref<HelloWorldApp>::New()` creates the root component. `Ref<T>` is
  RTXUI's reference-counted handle; components are always held through it.
- `Screen screen(app)` connects the component to the terminal. The screen
  owns the render loop: it puts the terminal into raw mode, tracks resizes,
  and restores the terminal state on exit.
- `screen.Loop()` runs until the user quits (<kbd>Ctrl-C</kbd>, or
  <kbd>Escape</kbd> outside of any open dialog). The loop is event-driven:
  it sleeps until input or posted work arrives, digests state changes, and
  repaints only the cells that changed.

## Build and run

With RTXUI [set up](/guide/getting-started), the program builds like any
C++ target linking the `rtxui` library:

```cmake
add_executable(hello hello.cpp)
target_link_libraries(hello PRIVATE rtxui)
```

Run it from a terminal. The bordered card appears at the top-left; resize
the terminal and the layout follows. Press <kbd>Ctrl-C</kbd> to quit — the
terminal is restored to its normal state.

## Where to go next

- Add state and buttons: [Reactivity](/reactivity)
- Learn the template syntax: [Interpolation](/guide/interpolation),
  [Event handlers](/guide/bindings)
- Style it: [Styling basics](/guide/css/basics)
