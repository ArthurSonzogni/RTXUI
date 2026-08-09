# Reactivity

RTXUI keeps the terminal display synchronized with plain C++ state. There are
no observable wrappers, signal types, or setters: component members are
ordinary variables, and the engine detects changes by comparing values
between frames. This page explains the model precisely — what triggers an
update, when computed values refresh, and how data flows between components.

## A reactive component

Components derive from `rtxui::Component<Derived>`. Ordinary data members
hold state, `const` member functions expose derived values, and non-`const`
member functions act as event handlers. Each one is registered with `Bind()`
so the template can refer to it by name:

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
  )html";

  Counter() {
    Bind(count);
    Bind(label);
    Bind(double_count);
    Bind(Increment);
  }
};
```

`Bind()` dispatches on the member's kind:

| Member | Effect of `Bind()` |
| :--- | :--- |
| Data member | Reactive state: snapshotted and compared each digest. |
| `const` method | Computed value: interpolated on render, no snapshot. |
| Non-`const` method | Event handler callable from `onclick`, `@change`, ... |
| Method taking `std::string` | Parameterized handler, e.g. `onclick="Select(home)"`. |
| Container (e.g. `std::vector`) | Collection for `<for>` loops. |

`Import("name", lambda)` registers a lambda or free function under an
explicit name, and `Import<rtxui::button>()` makes a component type usable
as a tag in the template.

## The update cycle

An RTXUI frame is driven by events, not by a fixed tick. When an event
arrives (a keypress, a mouse click, or a task posted from another thread),
the screen dispatches it and then runs a **digest**:

1. **Compare.** For every bound data member, the engine holds a snapshot — a
   typed copy taken at registration. `Digest()` compares each member against
   its snapshot with `operator!=` and updates the snapshot when they differ.
   Collections are compared element-wise.
2. **Re-render changed components.** If any bound member of a component
   changed, that component re-renders: its template is re-evaluated and
   **reconciled** against the existing element tree, reusing and patching
   elements rather than rebuilding them. Child components digest recursively.
3. **Layout and paint.** The (possibly updated) element tree is laid out,
   painted into a cell buffer, and diffed against the previous frame so only
   changed cells are written to the terminal.

Nothing happens between events: an idle application performs no work.

## What the model implies

**Mutate freely inside handlers.** Any change to bound members during an
event handler is picked up by the digest that follows it. There is no
`SetState`-style API to call.

**Computed values depend on bound state.** A `const` method has no snapshot
of its own; it is simply re-evaluated when its component re-renders. If it
reads only bound members, it can never be stale. If it reads data that is
*not* bound — a global, a clock, a file — nothing triggers a re-render when
that data changes, and the display will not update. Bind the underlying
state, or update a bound member when the external data changes.

**Change detection needs `operator!=`.** Bound types must be comparable.
For a struct, either define equality or bind its fields separately.

**Cross-thread updates go through the task runner.** Only the main thread
may touch component state. From a worker thread, post a lambda with
`task::TaskRunner::Current()->PostTask(...)`; it runs on the UI loop, and
its state changes are digested like any event
(see the [cookbook recipe](/guide/cookbook#background-work-without-freezing-the-ui)).

## Passing data between components

A child component declares a public `Props` struct; the parent sets those
fields as attributes, with interpolation evaluated in the parent's context:

```cpp
class TodoItem : public rtxui::Component<TodoItem> {
 public:
  struct Props {
    std::string task_text;
    bool completed = false;
  } props;

  std::string completion_class() const {
    return props.completed ? "done" : "";
  }

  std::string_view view = R"html(
    <div class="todo-row">
      <span class="{completion_class}">{props.task_text}</span>
    </div>
  )html";

  TodoItem() {
    Bind(props.task_text);
    Bind(props.completed);
    Bind(completion_class);
  }
};
```

```html
<TodoItem props.task_text="{item.text}" props.completed="{item.done}" />
<!-- The props. prefix may be omitted: -->
<TodoItem task_text="{item.text}" completed="{item.done}" />
```

When the parent re-renders, the evaluated attribute values are written into
the child's `props` members; the child's own digest then notices the change
and refreshes its view. Data flows one way — parent to child. For
child-to-parent communication, pass a callback name or let the child call a
handler bound on an ancestor (event handlers propagate up the component
tree until one component handles them).

## Where C++26 reflection fits

When the compiler supports standard C++26 reflection (CMake detects this and
defines `RTXUI_HAS_REFLECTION`), struct fields inside bound collections are
readable from templates directly — `{task.name}` works without writing a
mapper. Without it, provide the mapper shown in the
[loops guide](/guide/loops#collections-of-structs). Binding itself is
explicit in either case: components list their reactive members in their
constructor.

## Demos

The smallest complete picture: a bound `int`, a computed value derived from
it, and two handlers that mutate it.

<ExampleTabs src="/wasm/rtxui_example_counter.js">
<template #source>

<<< @/../example/counter.cpp

</template>
</ExampleTabs>

The same machinery at application scale — a bound collection of structs,
several computed values, and conditional rendering driven by the selection:

<ExampleTabs src="/wasm/rtxui_example_app_dashboard.js" :cols="100" :rows="28">
<template #source>

<<< @/../example/app_dashboard.cpp

</template>
</ExampleTabs>

For a single program exercising most of the library at once, see
[demo.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/demo.cpp).
