# Reactivity & State Reconciliation Specification

This specification documents the reactivity model, snapshot-based dirty detection algorithms, two-way binding propagation, and collection reconciliation semantics in RTXUI.

---

## 1. System Invariants

1. **Zero Runtime Wrappers**: Reactive state is stored in standard C++ member variables (`int`, `std::string`, custom structs). RTXUI requires no special signal types, proxy wrappers, or accessor boilerplate.
2. **Snapshot-Driven Change Detection**: Changes are detected by comparing member values against typed snapshots captured during the preceding frame reconciliation.
3. **Discrete Event-Driven Execution**: State evaluation and DOM reconciliation are strictly event-driven. In the absence of terminal input or queued asynchronous tasks, the engine performs zero computation.

---

## 2. Binding Classification & Contracts

Members are registered via `Bind()` within the component constructor or `InitReflection()`:

```cpp
class Counter : public rtxui::Component<Counter> {
 public:
  int count = 0;
  std::string label = "Items";

  int double_count() const { return count * 2; }
  void Increment() { count++; }

  std::string_view view = R"html(
    <div class="panel">
      <span>{label}: {count} (Double: {double_count})</span>
      <button onclick="Increment">+1</button>
    </div>
  )html";

  Counter() {
    Bind(count);
    Bind(label);
    Bind(double_count);
    Bind(Increment);
  }
};
```

### 2.1 Member Classification Matrix
| Member Category | Type Signature | Snapshot Storage | Evaluation Stage | Semantic Contract |
| :--- | :--- | :--- | :--- | :--- |
| **Mutable State** | `T member;` | Yes (`T snapshot_`) | `Digest()` | Must satisfy `std::equality_comparable` and `std::is_copy_constructible`. Evaluated with `!=`. |
| **Computed Property** | `T method() const;` | No | Template Render | Invoked strictly on-demand during template string expansion. Must be side-effect free. |
| **Event Handler** | `void method();` | No | Event Dispatch | Invoked upon matching event trigger. May freely mutate bound state. |
| **Parameterized Handler**| `void method(std::string);` | No | Event Dispatch | Invoked with string argument parsed from template attribute call syntax. |
| **Repetition Collection** | `std::vector<T>` | Yes (Deep copy) | `Digest()` | Element-wise equality comparison. Triggers `<for>` reconciliation on size or content mutation. |

---

## 3. The Frame Reconciliation Lifecycle

When an input event or worker task executes, `Screen::Step()` coordinates state reconciliation via `Digest()`:

```
[State Mutation in Handler / Task]
                │
                ▼
1. Snapshot Diffing (Component::Digest)
   ├── Evaluate `member != snapshot_` for each registered data member
   └── If changed: update `snapshot_ = member` and flag component as dirty
                │
                ▼
2. Template Expansion (if dirty)
   ├── Interpolate `{member}` and `{computed_method}` into XML string
   └── Parse into AST via `rtxui::xml::Parse()`
                │
                ▼
3. Virtual DOM Reconciliation
   ├── Patch existing Element hierarchy in-place
   ├── Recycle unchanged child elements
   └── Route slotted content to `<slot>` targets
                │
                ▼
4. Recursive Descendant Digest
   └── Propagate `Digest()` down active child components
```

### Snapshot Invariant
Snapshots are updated atomically as each divergence is verified. If no bound members differ, `Digest()` returns `false` in $O(K)$ time (where $K$ is the number of bound properties), bypassing XML parsing, DOM traversal, and layout invalidation.

---

## 4. Two-Way Data Binding Protocol

Composite controls (such as `<input>`, `<checkbox>`, `<radio>`, `<select>`, and `<slider>`) manage internal interactive state while reflecting updates back to parent variables:

### 4.1 Parent-to-Child Downstream Flow
When a parent component passes a bound variable as an attribute:
```xml
<input value="{username}" />
```
The evaluated string is written to the child component's matching property during reconciliation.

### 4.2 Child-to-Parent Upstream Flow
When user interaction modifies the child control:
1. The child mutates its internal state member (e.g. `checkbox::checked = true`).
2. The child dispatches `PropagateBinding("checked", "true")`.
3. The parent reconciler resolves the source variable bound to the attribute and assigns the new value directly to the parent's C++ member.
4. Subsequent digest passes observe the updated value across both components in perfect synchronization.

---

## 5. Collection Keying & DOM Reconciliation

When rendering collections with `<for each="item in items">`, the reconciler maps collection items to active DOM subtrees.

### 5.1 Positional Reconciliation (Unkeyed)
```xml
<for each="item in items">
  <div class="row">
    <input value="{item.name}" />
  </div>
</for>
```
Without an explicit `key`, elements are matched purely by collection index:
- Adding or removing items at the beginning or middle causes in-place mutations across all subsequent elements.
- Ephemeral element states (cursor position, scroll offset, running CSS transitions) remain pinned to the physical index, rather than following the logical entity.

### 5.2 Associative Identity Reconciliation (Keyed)
```xml
<for each="item in items" key="item.id">
  <div class="row">
    <input value="{item.name}" />
  </div>
</for>
```
When `key` is specified:
- Each item is assigned an identity based on the named struct field.
- When the collection is sorted, filtered, or reordered, existing DOM nodes are repositioned rather than reconstructed.
- Input focus, active text selection, and running CSS transitions stay anchored to the specific item.

---

## 6. Asynchronous Background State Synchronization

State mutations must not occur on worker threads. To update reactive state from background operations:

```cpp
void FetchDataAsync() {
  std::thread([this]() {
    std::string result = BackgroundHttpCall();
    
    // Dispatch state update to UI loop:
    rtxui::task::TaskRunner::Current()->PostTask([this, result]() {
      this->status_text = result;
      // Screen event loop automatically digests and renders changes.
    });
  }).detach();
}
```

The UI loop executes the posted lambda, detects the mutation during the subsequent digest phase, and paints the updated frame atomically.

---

## 7. Interactive Demos

### State Binding and Computed Values
<ExampleTabs src="/wasm/rtxui_example_counter.js">
<template #source>

<<< @/../example/counter.cpp

</template>
</ExampleTabs>

### Collection Reactivity and Selection
<ExampleTabs src="/wasm/rtxui_example_app_dashboard.js" :cols="100" :rows="28">
<template #source>

<<< @/../example/app_dashboard.cpp

</template>
</ExampleTabs>
