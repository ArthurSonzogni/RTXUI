# RTXUI (Reactive Terminal User Interface)

RTXUI is a C++ library designed for building reactive terminal user interfaces. It takes inspiration from modern web frameworks, employing a component-based architecture, CSS-like styling, and an XML/HTML-like template system.

## Key Features

- **Component-Based Architecture**: UI is built using reusable components, similar to React or Vue.
- **Reactive State Management**: Uses a "Cell" system (e.g., `TypedCell`, `ComputedTypedCell`) for automatic UI updates when state changes.
- **HTML/XML Templates**: Components define their structure using a familiar HTML-like syntax embedded in C++.
- **CSS-Like Styling**: Supports styling through `<style>` tags within components, including properties like:
  - Colors (RGB, named colors)
  - Borders (tall, etc.)
  - Padding, Margin, Gap
  - Layout modes (Flexbox, Block, Inline)
  - Width and Height (percentages, fixed values)
- **Advanced Layout Engine**: Implements a layout system that supports Flexbox-like positioning and sizing.
- **Built-in Components**: Provides standard components like `div`, `span`, and `slot` for content projection.

## Project Structure

- `src/component/`: Core component logic and base classes.
- `src/cell/`: Reactive state management system.
- `src/dom/`: DOM-like tree structure representing the UI elements.
- `src/layout/`: Layout engine for calculating element positions and sizes.
- `src/paint/`: Rendering logic for drawing the UI to a terminal-like texture.
- `src/style/`: CSS-style parsing and application logic.
- `src/xml/`: XML parser for component templates.
- `src/terminal/`: Terminal input parsing and event handling.

## Build System

The project uses **CMake** and supports **Bazel**. It requires **Clang 18** and **C++23**.

## Usage Example

```cpp
RTXUI_COMPONENT(MyComponent) {
  auto count = State(0);
  
  return R"html(
    <div class="container">
      <button onclick="count(count() + 1)">Increment</button>
      <span>Count: {count()}</span>
    </div>
    
    <style>
      .container {
        display: flex;
        gap: 1;
        border: tall;
      }
    </style>
  )html";
}
```
