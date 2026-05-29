# RTXUI Tutorial: Building Reactive Terminal Apps

Welcome to RTXUI! This tutorial guides you through building a reactive terminal application from scratch.

---

## 1. Minimal Application Setup

Every RTXUI application begins with a main component and a `Screen` runner. Let's create a minimal component that prints a greeting:

```cpp
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class HelloWorldApp : public Component<HelloWorldApp> {
 public:
  std::string_view Setup() override {
    Import<div>();
    return R"html(
      <div class="card">
        Hello World from RTXUI!
      </div>
      <style>
        self {
          display: block;
          padding: 1;
          background-color: rgb(15, 23, 42);
        }
        .card {
          border: solid;
          border-color: rgb(59, 130, 246);
          padding: 1;
          color: rgb(241, 245, 249);
        }
      </style>
    )html";
  }
};

int main() {
  auto app = Ref<HelloWorldApp>::New();
  Screen screen(app);
  screen.Loop(); // Starts interactive rendering
  return 0;
}
```

---

## 2. Adding Reactive State

RTXUI uses compile-time reflection to track class variables as reactive state. Simply define members on your component class, and reference them in curly braces `{}` inside your template:

```cpp
class CounterApp : public Component<CounterApp> {
 public:
  // State variables
  int count = 0;

  // Computed state method
  int double_count() const { return count * 2; }

  std::string_view Setup() override {
    Import<div>();
    Import<span>();
    Import<button>();
    return R"html(
      <div>
        <span>Count: {count}</span>
        <span>Double: {double_count}</span>
        <button onclick="count++">Increment</button>
        <button onclick="count--">Decrement</button>
      </div>
    )html";
  }
};
```
- **Reactivity Model**: Whenever a button is clicked, an event handler modifies `count`. The screen runs a Digest cycle, detects that `count` has changed, and updates the DOM elements.

---

## 3. Nesting and Scrolling Containers

When layouts contain lists or large blocks of content, they can overflow. RTXUI supports full horizontal and vertical scrolling with visual scrollbars.

```cpp
class ScrollBox : public Component<ScrollBox> {
 public:
  std::string_view Setup() override {
    Import<div>();
    return R"html(
      <div class="scroll-container">
        <div>Item 1</div>
        <div>Item 2</div>
        <div>Item 3</div>
        <div>Item 4</div>
        <div>Item 5</div>
        <div>Item 6</div>
        <div>Item 7</div>
        <div>Item 8</div>
        <div>Item 9</div>
        <div>Item 10</div>
      </div>
      <style>
        .scroll-container {
          display: block;
          height: 5;
          overflow-y: scroll;
          scrollbar-width: auto;
          border: wide;
          border-color: rgb(29, 78, 216);
        }
      </style>
    )html";
  }
};
```

By adding `overflow-y: scroll` and restricting `height`, RTXUI automatically crops overflowing elements and displays a modern, responsive scrollbar on the right. Scroll events bubble up nested containers when boundaries are reached.
