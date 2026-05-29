# RTXUI Design: Reactive Terminal UI

This document describes the design for RTXUI, a framework for building reactive terminal user interfaces using C++26 reflection.

## 1. The Unified Component Model
In RTXUI, there is only one way to define a component. You do not need to choose between a "normal" and a "transparent" component. Every component is "transparent" by default.

```cpp
class MyComponent : public rtxui::Component<MyComponent> {
 public:
  // Standard C++ members are automatically treated as reactive state.
  int count = 0;
  std::string label = "Counter";

  // Standard methods are automatically treated as computed state.
  int double_count() const { return count * 2; }

  // The 'view' field defines the UI structure.
  std::string_view view = R"html(
    <div class="container">
      <span>{label}: {count}</span>
      <span>Double: {double_count}</span>
      <button onclick="count++">Increment</button>
    </div>
  )html";
};
```

## 2. Why CRTP? (`Component<Derived>`)
We use the Curiously Recurring Template Pattern (CRTP) for one specific reason: **Reflection access.**

Because `Component` knows the type of the `Derived` class, it can perform compile-time reflection (`std::meta::members_of(^^Derived)`) to discover:
1.  Which data members exist (`count`, `label`).
2.  Which methods exist (`double_count`).
3.  How to bind them to the expressions found in the `view` string.

## 3. Implementation: The Snapshot Mechanism

To avoid the overhead of "Observables" or "Signals" (which require wrapping every variable), RTXUI uses a **Snapshot** approach:

1.  **Initial Snapshot**: When the component is mounted, the framework takes a bit-for-bit copy of the reactive members and stores them in a internal buffer (e.g., a `std::tuple` or `std::vector<std::any>` generated via reflection).
2.  **The Digest Cycle**: Whenever an event occurs (a key press, a timer, or a network callback), the framework runs a `Digest()`.
3.  **Comparison**: The framework reflects over the members again, comparing their current values to the values in the snapshot.
4.  **Propagation**: 
    - If `count` has changed, the framework knows exactly which DOM nodes in the `view` depend on `{count}`.
    - It updates only those specific text nodes or attributes.
    - It then updates the snapshot to match the new current state.

## 4. Parent-Child Communication: `struct Props`

Components can expose reactive input properties (attributes/props) that the parent component can set inside its template view. They are declared in a nested `struct Props` inside the child component class:

```cpp
class MyChild : public rtxui::Component<MyChild> {
 public:
  struct Props {
    std::string title;
    int count = 0;
  } props;

  void InitReflection() override {
    Bind(props.title);
    Bind(props.count);
    rtxui::Component<MyChild>::InitReflection();
  }

  std::string_view view = R"html(
    <div>Title: {props.title}, Count: {props.count}</div>
  )html";
};
```

The parent component can pass these attributes in its template view:
```html
<MyChild props.title="{parent_title}" props.count="{parent_count}" />
<!-- Or using the short name syntax: -->
<MyChild title="{parent_title}" count="{parent_count}" />
```

During parent rendering:
1. The parent interpolates the attribute values in the parent's context.
2. The values are mapped to the child component's `props` member variables (using type-safe parsing from string).
3. The child component is re-rendered to propagate changes.

## 5. Benefits of a Single Class
- **Simplicity**: No need to decide which base class to use.
- **Zero Boilerplate**: You write a standard C++ class. The framework handles the "magic" of connecting it to the UI.
- **Performance**: Reflection happens at compile-time. At runtime, the framework only performs a few comparisons to see if anything changed.

<WasmTerminal src="/wasm/rtxui_example_demo.js" :cols="80" :rows="34" />
