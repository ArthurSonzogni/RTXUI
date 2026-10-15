# Loops & Lists

The `<for>` tag renders its children once per item of a bound C++ container,
such as a `std::vector`.

## The `<for>` tag

Name the bound collection with `each`, and choose the loop variable's name
with `as`:

```cpp
// Bound in the constructor:
std::vector<std::string> fruits = {"Apple", "Banana", "Cherry"};
Bind(fruits);
```

```html
<ul>
  <for each="{fruits}" as="fruit">
    <li>{fruit} (index: {$index})</li>
  </for>
</ul>
```

Inside the loop body:

- `{fruit}` interpolates the item, converted to a string.
- `{$index}` interpolates the zero-based index. It is most useful for
  passing to a parameterized callback, e.g.
  `<button onclick="RemoveItem({$index})">`
  (see [Event Handlers](/guide/bindings)).

When the collection changes — items added, removed, or mutated — the loop's
rendered children are reconciled on the next digest.

This demo loops over a `std::vector<std::string>`, appending with a bound
`<input>` and removing by `{$index}`:

<ExampleTabs src="/wasm/rtxui_example_loop_simple.js">
<template #source>

<<< @/../example/loop_simple.cpp

</template>
</ExampleTabs>

## Keyed loops

By default, collection children reconcile by **position**: the DOM node at index `i` is reused for the `i`-th item. When items reorder or shift, element-local state (keyboard focus, scroll offsets, active CSS transitions) stays locked to the index rather than tracking the item.

Give the loop a `key` to identify items instead:

```html
<for each="{tasks}" as="task" key="{task.id}">
  <div tabindex="0">{task.name}</div>
</for>
```

The key is interpolated per item, exactly like the loop body, so it can be any
expression that names the item — an id field is the usual choice. On the next
digest each item's existing elements are moved to the item's new position and
reused there, carrying their state with them. Focus stays on the task the user
had focused, even if it shifts three rows down.

Keys must be unique within the loop, and stable across frames: keying by
`{$index}` is the same as not keying at all. A loop without `key` keeps the
position matching described above, which stays the cheaper option for a list
that never reorders.

## What a change renders

When the component renders again, an item whose interpolations all give what
they gave last time (its own value, `{$index}`, and anything else it reads) is
left as it is: changing one item of a long list renders that item only. Its
components still pick up their own changes.

An item renders every time when its output depends on more than what it
interpolates: when it holds a nested `<for>` or a `<slot>`, or passes a value
to a component's bound property, such as `<input value="{item.name}">`.

## Long lists: `virtual`

A loop renders every item, and every item costs something each frame: a few
thousand are fine, a hundred thousand are not. Inside a scroll container, mark
the loop `virtual` to render only the items around what the container shows:

```html
<div class="log">
  <for each="{lines}" as="line" virtual="">
    <div>{$index}: {line}</div>
  </for>
</div>
<style>
  .log {
    height: 20;
    overflow-y: scroll;
  }
</style>
```

The loop renders the items in view and a screenful more on either side, and
two empty blocks stand in for the rest, so the scroll range, the scrollbar and
`{$index}` are those of the whole collection. When scrolling moves past what
was rendered, the loop renders again at the new position; scrolling within a
screenful needs no render at all. A 100,000-item list costs about what one
screen of items does.

- Every item is assumed to be the same height: one row, or `item-height="N"`
  rows. An item of a different height puts the rows the container shows out of
  step with the items rendered.
- The scroll container is the nearest ancestor with an `overflow-y` other than
  `visible`. A virtual loop without one is reported as a diagnostic.
- Only the rendered items are in the DOM: an item scrolled away loses its
  element state (focus, its own scroll), and Tab does not reach the items not
  rendered. Scrolling the container brings them in.
- Spacers are block-level, so the container should lay out its items as a
  column: a block, or a `flex-direction: column` flex box.

<ExampleTabs src="/wasm/rtxui_example_virtual_list.js">
<template #source>

<<< @/../example/virtual_list.cpp

</template>
</ExampleTabs>

## Collections of structs

A collection of plain values stringifies each item directly. For a collection
of structs, say once what a template reads of the type, with a `Fields()`
function next to it. `Bind(tasks)` then needs nothing more:

```cpp
struct Task {
  std::string name;
  bool completed;
};

// What a template reads of a Task: {task.name} and {task.status}.
void Fields(const Task& task, FieldSink& out) {
  out("name", task.name);
  out("status", task.completed ? "Done" : "Pending");
}

class TaskList : public Component<TaskList> {
 public:
  std::vector<Task> tasks{{"Write docs", true}, {"Ship", false}};
  TaskList() { Bind(tasks); }

  std::string_view view = R"html(
    <ul>
      <for each="{tasks}" as="task"><li>{task.name} — {task.status}</li></for>
    </ul>
  )html";
};
```

- `out(name, value)` converts the value to text as `{name}` would: no
  `std::to_string` needed.
- Declare `Fields()` in the same namespace as the type, before the component
  that binds the collection: it is found the way `operator<<` is.
- A field the type does not give reads as empty.

### Fields that depend on the component

`Fields()` sees only the item. When what a template shows also depends on the
component, like a row hidden by a filter, give the binding its own fields,
with a `const` member function of the same shape. It takes precedence over
the type's `Fields()`:

<!-- snippet: fragment -->
```cpp
std::string filter = "all";

TaskList() { Bind(tasks, &TaskList::TaskFields); }

void TaskFields(const Task& task, FieldSink& out) const {
  out("name", task.name);
  out("hidden", filter == "done" && !task.completed ? "hidden" : "");
}
```

A lambda of the same shape works too: `Bind(tasks, [](const Task& task,
FieldSink& out) { ... })`. `BindCollection("name", &collection, fields)` is
the same, for a template name that differs from the member name.

### Changes

A collection is compared with its last render to find what changed. When the
type has an `operator==`, items are compared with it, which is the cheapest.
Without one, the fields from `Fields()` or the binding are compared
instead.

### With C++26 reflection

When your application is compiled with C++26 static reflection (for example
GCC 16 with `-std=c++26 -freflection`), a struct
without `Fields()` exposes its members under their own names: `Bind(tasks)`
alone makes `{task.name}` work. A `Fields()` function, or the binding's own
fields, still decide when there is one: to rename, format or compute a field
such as `status` above.

This demo loops over a `std::vector<Task>` described by `Fields()`, toggling each
item's `completed` field from the template:

<ExampleTabs src="/wasm/rtxui_example_loop_complex.js">
<template #source>

<<< @/../example/loop_complex.cpp

</template>
</ExampleTabs>
