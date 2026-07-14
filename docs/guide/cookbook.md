# Cookbook

Self-contained patterns for interface problems that come up in most terminal
applications. Each recipe is a complete program from the repository's
`example/` directory — it compiles as part of the project's example build, so
you can paste it into a project and adapt it.

The recipes rely on three RTXUI idioms worth internalizing:

- **State lives in C++ members; the template reads it.** Templates interpolate
  bound names — they do not evaluate expressions, contain statements, or
  perform assignments.
- **Logic is a computed method.** A `const` method returns a class string or a
  boolean; the template binds it with `class="{method_name}"` or
  `condition="{method_name}"`. Ternaries and comparisons live in C++.
- **Event handlers name a bound method**, optionally with one argument:
  `onclick="Select(home)"` invokes `void Select(std::string)` with `"home"`.

---

## Tabbed navigation

Track the active tab in a state variable. Buttons call a single parameterized
method; each button's class and each pane's visibility come from computed
methods. The `<if>`/`<elif>`/`<else>` chain shows one pane at a time.

<ExampleTabs src="/wasm/rtxui_example_cookbook_tabs.js">
<template #source>

<<< @/../example/cookbook_tabs.cpp

</template>
</ExampleTabs>

For most tabbed interfaces the built-in
[`<tabs>`/`<tab-pane>` components](/html_reference) are enough; this recipe
is the underlying pattern for when you need full control over the header.

---

## Background work without freezing the UI

Callbacks run on the main thread, so blocking inside one (network requests,
large file reads) freezes rendering. Run the work on a `std::thread` and post
the result back to the main loop with `task::TaskRunner`, which may be called
from other threads. State mutations posted this way are picked up by the
normal digest cycle.

<ExampleTabs src="/wasm/rtxui_example_cookbook_async.js">
<template #source>

<<< @/../example/cookbook_async.cpp

</template>
</ExampleTabs>

A detached thread must not outlive the component it captures. In real
applications, join or signal worker threads before the component is
destroyed, or route results through state that survives the component.

---

## Confirmation dialog

The built-in [`<dialog>`](/html_reference) element renders centered above the
rest of the interface with a dimmed backdrop, and closes on
<kbd>Escape</kbd>. Bind its `open` attribute to a boolean and flip that
boolean from methods.

<ExampleTabs src="/wasm/rtxui_example_cookbook_dialog.js">
<template #source>

<<< @/../example/cookbook_dialog.cpp

</template>
</ExampleTabs>

To build an overlay by hand instead — a dropdown, a toast, a context menu —
use `position: absolute` (or `fixed`) with `z-index` inside a
`position: relative` ancestor, and gate it behind an `<if>`. The
[positioning guide](/guide/css/positioning) covers the mechanics.
