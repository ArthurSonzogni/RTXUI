# C++ API Reference

The public C++ surface of RTXUI. Everything below is available through a
single include:

```cpp
#include <rtxui/rtxui.hpp>
```

## `rtxui::Ref<T>`

Reference-counted handle used for components and DOM elements. Create
instances with the static factory:

| Member | Description |
| :--- | :--- |
| `Ref<T>::New(args...)` | Constructs a `T` and returns a `Ref<T>` owning it. |
| `operator->`, `get()` | Access the underlying object. |

## `rtxui::Component<Derived>`

Base class for user components (CRTP: the class passes itself as the
template argument). A component provides its template either as a `view`
data member (enables [hot reload](/guide/hot-reload)) or by overriding
`Setup()`:

```cpp
class MyCard : public rtxui::Component<MyCard> {
 public:
  std::string_view view = R"html(...)html";
  // — or —
  std::string_view Setup() override { return R"html(...)html"; }
};
```

### Registration

Members become visible to the template after registration, done in the component constructor:

| Call | Registers |
| :--- | :--- |
| `Bind(member)` | A data member as reactive state, a `const` method as a computed value, a non-`const` method as an event handler, or a container for `<for>` loops. |
| `Bind(container, mapper)` | A container of structs; `mapper` returns a `ManualStructVisitor` exposing named fields. |
| `BindCollection("name", &container[, mapper])` | Same as `Bind` for containers, under an explicit template name. |
| `Import("name", lambda)` | A lambda or free function as an event handler; a `std::function<void(std::string)>` becomes a parameterized handler. |
| `Import<ComponentType>()` | Another component type, usable as a tag in this template. |

### Behavior and structure

| Method | Description |
| :--- | :--- |
| `bool OnEvent(Event)` *(virtual)* | Intercept input before built-in handling. Return `true` to consume the event. Events propagate to children via the base implementation. |
| `bool Digest()` | Compares bound state against snapshots; re-renders on change. Called by the screen loop. |
| `Element* Root() const` | The root of this component's element tree. |
| `Ref<Element> Slot(std::string_view name)` | The slot element with the given name (`""` for the default slot). |
| `void EnableHotReload()` | Watches the component's source file and re-parses `view` on change. See [Hot Reload](/guide/hot-reload). |
| `CaptureMouse()` / `ReleaseMouse()` | Route all mouse events to this component (e.g. while dragging). |

## `rtxui::Element`

A node in the live element tree.

| Member | Description |
| :--- | :--- |
| `Element* Parent()` | Parent element, or `nullptr` at the root. |
| `size_t ChildCount() const`, `Element* ChildAt(size_t)` | Indexed child access. |
| `const std::vector<Ref<Element>>& children() const` | All children. |
| `Element* QuerySelector(std::string_view)` | First descendant matching a selector (`#id`, `.class`, or tag). |
| `const std::map<std::string, std::string>& Attributes() const` | Parsed attributes. |
| `id`, `classes` | The element's id string and class list. |
| `scroll_x()` / `scroll_y()` | Current scroll offsets in cells. |
| `set_scroll_x(int, bool smooth = false)` / `set_scroll_y(...)` | Set scroll offsets, optionally animated. |
| `scroll_width()` / `scroll_height()` | Total size of the scrollable content. |
| `focused()`, `hovered()`, `active()` | Interaction state, as used by CSS pseudo-classes. |

## `rtxui::Screen`

Connects a component tree to the terminal and owns the event loop.

| Method | Description |
| :--- | :--- |
| `Screen(Ref<ComponentBase>, std::shared_ptr<TerminalDevice> = nullptr)` | Mounts the component and draws the first frame. Pass a custom device for headless use (tests use `MockTerminalDevice`). |
| `void Loop()` | Runs the event loop until the user quits. |
| `void Step()` | Runs a single loop iteration: wait, dispatch, run posted tasks, digest, draw. |
| `void Dispatch(Event)` | Injects one event as if it came from the terminal. |
| `void Draw()` | Renders the current state unconditionally. |
| `void SetSmoothScrollEnabled(bool)` | Toggles scroll animation (disable for deterministic tests). |

## `rtxui::Event`

A variant over keyboard, mouse, and resize inputs.

| Member | Description |
| :--- | :--- |
| `event.is<Event::Keyboard>()`, `event.is<Event::Mouse>()` | Kind tests. |
| `event.get<T>()`, `event.get_if<T>()` | Typed access to the payload. |
| `Event::ArrowUp()`, `Event::Return()`, `Event::Escape()`, ... | Named constants for common keys: arrows, `PageUp`/`PageDown`, `Home`/`End`, `Tab`/`TabReverse`, `Return`, `Escape`, `CtrlC`, function keys, and letters. Compare with `==`. |

`Event::Keyboard` exposes `codepoint`, `special` (the key enum), `modifier`,
and `motion` (pressed/repeat/released). `Event::Mouse` exposes `button`,
`motion`, and `x`/`y` cell coordinates.

## `rtxui::Color`

| Member | Description |
| :--- | :--- |
| `Color::RGB(r, g, b)` | Opaque color. |
| `Color::RGBA(r, g, b, a)` | Color with alpha; alpha blends over the cell behind it. |

## `task::TaskRunner`

Thread-safe task posting onto the UI loop. This is the supported way to
update the interface from other threads: the loop wakes immediately, runs
the task, digests, and repaints.

| Method | Description |
| :--- | :--- |
| `static TaskRunner* Current()` | The runner for the current thread. On the main thread this is the screen's runner; capture it there and share it with workers. |
| `void PostTask(Task)` | Schedules a callback on the loop. Callable from any thread. |
| `void PostDelayedTask(Task, std::chrono::steady_clock::duration)` | Schedules a callback after a delay. |
