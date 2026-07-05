# Screen & Lifecycle

`rtxui::Screen` connects a component tree to the terminal and owns the event
loop.

## Running an application

Create the root component through `Ref<T>::New()` and hand it to a `Screen`;
`Loop()` blocks until the user quits:

```cpp
#include <rtxui/rtxui.hpp>

int main() {
  auto root = rtxui::Ref<MyRootComponent>::New();
  rtxui::Screen screen(root);
  screen.Loop();
  return 0;
}
```

Constructing the screen mounts the component (parsing its template and
building the element tree), runs a first digest, and draws the initial
frame. Entering `Loop()` switches the terminal to raw mode and enables mouse
reporting; both are restored when the loop exits — on <kbd>Ctrl-C</kbd>, on
<kbd>Escape</kbd> when no dialog is open, or when input closes.

## What one loop iteration does

The loop is event-driven; it sleeps until something happens. Each iteration:

1. **Input.** Raw terminal bytes are parsed into `rtxui::Event` values
   (keys, mouse, resize) and dispatched: first to component `OnEvent`
   overrides, then to the built-in behaviors (focus traversal with
   <kbd>Tab</kbd>, spatial navigation with arrows, scrolling, click
   simulation with <kbd>Enter</kbd>/<kbd>Space</kbd>).
2. **Posted tasks.** Work queued with
   `task::TaskRunner::Current()->PostTask(...)` — typically results arriving
   from worker threads — runs on the loop thread. If any task ran, a digest
   follows automatically, so state mutated by tasks is repainted without
   waiting for further input.
3. **Digest and draw.** After events or tasks, bound state is compared to
   its snapshots; changed components re-render, layout runs, and only the
   cells that differ from the previous frame are written to the terminal.
4. **Animations.** While CSS transitions are active, the loop wakes at a
   steady cadence to advance them; otherwise it stays idle.

There is no manual invalidation API to call in normal use: mutate bound
state in an event handler or a posted task, and the frame follows.

## Finer control

For tests and embedding, `Screen` exposes the loop's pieces:

- `Step()` runs a single iteration (wait, dispatch, digest, draw).
- `Dispatch(event)` injects one event as if it came from the terminal.
- `Draw()` forces a render of the current state.
- `SetSmoothScrollEnabled(bool)` toggles scroll animation, useful for
  deterministic tests.

A second constructor argument accepts a custom `TerminalDevice`; the test
suite uses this with a mock device to run entire interfaces headlessly.
