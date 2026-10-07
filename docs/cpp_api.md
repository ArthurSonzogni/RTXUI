# C++ Engine API & Architecture Specification

This specification defines the public C++ API of RTXUI (`#include <rtxui/rtxui.hpp>`), documenting system contracts, thread-safety invariants, exception guarantees, and the frame execution lifecycle.

---

## 1. Core Architectural Invariants

### 1.1 Concurrency Model
- **UI Thread Affinity**: All component construction, template compilation, DOM mutations, layout resolution, and `Screen::Draw()` operations **must** execute on the designated main UI thread. They are non-reentrant and not thread-safe.
- **Cross-Thread Synchronization**: Background worker threads communicate with UI state exclusively by posting callbacks with `rtxui::PostTask()`. The runner queues them in a mutex-guarded queue and wakes the terminal event loop via a self-pipe (POSIX) or an event object (Windows).

### 1.2 Exception Safety Contract
- RTXUI is strictly non-throwing and compatible with `-fno-exceptions`. No C++ exceptions are thrown or caught across the public API surface.
- Operations subject to failure return `rtxui::expected<T, Error>` or `std::optional<T>`, or encode errors within deterministic state flags.

### 1.3 Memory & Object Ownership
- **Intrusive Smart Pointer (`rtxui::Ref<T>`)**: Components and DOM elements inherit from `rtxui::RefCounted`. `Ref<T>` manages intrusive reference counting, ensuring deterministic destruction on the main UI thread without secondary heap control blocks.
- **Scratch Layout Memory (`rtxui::LayoutArena`)**: The layout engine uses a double-buffered monotonic arena allocator. Box structures and physical fragments are allocated in bulk and recycled per frame, guaranteeing zero layout heap allocations in steady-state rendering.

### 1.4 Deterministic Frame Lifecycle
Every screen update follows an exact 6-stage sequential pipeline:

```
[Event Arrival / Task Wakeup]
             │
             ▼
1. Event Dispatch       : Capture -> Target -> Bubbling phase
             │
             ▼
2. State Mutation       : Handlers mutate bound variables / properties
             │
             ▼
3. Digest Reconciliation: Snapshot diffing detects mutations; dirty DOM subtrees patched
             │
             ▼
4. Layout Computation   : CSS cascade resolved; LayoutTreeBuilder populates LayoutArena;
                          RunLayout() computes geometric bounds and physical fragments
             │
             ▼
5. Rasterization        : Paint() rasterizes fragments into 2D Texture of Cell structs
             │
             ▼
6. Terminal Output      : Texture::RenderDiff() diffs against previous frame;
                          flushed to terminal wrapped in DEC Mode 2026 Synchronized Output
```

---

## 2. Reference Handle: `rtxui::Ref<T>`

Intrusive reference-counted pointer for `rtxui::ComponentBase` instances.

```cpp
template <typename T>
class Ref;
```

### Member Methods
| Signature | Invariants & Semantics |
| :--- | :--- |
| `template <typename... Args> static Ref<T> New(Args&&... args)` | Allocates `T` on the heap and returns an owning `Ref<T>` initialized with a reference count of 1. |
| `T* get() const noexcept` | Returns a raw pointer to the managed object without modifying the reference count. |
| `T* operator->() const noexcept` | Member access operator. |
| `T& operator*() const noexcept` | Dereferences the underlying instance. |
| `explicit operator bool() const noexcept` | Evaluates to `true` if managing a non-null pointer. |
| `void reset() noexcept` | Releases ownership; decrements reference count and destroys object if count reaches 0. |

---

## 3. Component Base: `rtxui::Component<Derived>`

CRTP base class for user-defined components.

```cpp
template <typename Derived>
class Component : public ComponentBase;
```

### 3.1 Template Declaration
Components provide HTML structure via exactly one of two public members, found at compile time:
1. **`view` Member** (Recommended): A `std::string_view` member variable. Supports runtime [Hot Reload](/guide/hot-reload).
2. **`Setup()` Method**: A non-virtual method returning `std::string_view` (no `override`).

Defining both is a compile error.

```cpp
class CounterCard : public rtxui::Component<CounterCard> {
 public:
  int count = 0;
  void Increment() { count++; }

  void InitReflection() override {
    Bind(count);
    Bind(Increment);
  }

  std::string_view view = R"html(
    <div class="card">
      <span>Count: {count}</span>
      <button onclick="Increment">+1</button>
    </div>
  )html";
};
```

### 3.2 Registration & Binding Interface
Bindings are registered during component initialization (`InitReflection()` or constructor):

| Method Signature | Semantics |
| :--- | :--- |
| `void Bind(T& member)` | Binds primitive member as reactive state. Auto-snapshots for dirty detection during `Digest()`. |
| `void Bind(const T& method)` | Registers a `const` member method as a computed property in templates. |
| `void Bind(T& container, Mapper mapper)` | Binds a collection of structs for repetition in `<for>` loops. |
| `void Import(std::string name, Callback cb)` | Registers an event handler (invoked by `onclick="name"`). |
| `template <typename C> void Import()` | Registers component type `C` as an XML/HTML tag available within this template. |

### 3.3 Lifecycle & Interaction Methods
| Method Signature | Invariants & Semantics |
| :--- | :--- |
| `virtual bool OnEvent(Event event)` | Intercepts keyboard/mouse input before DOM dispatch. Return `true` to consume the event. |
| `bool Digest()` | Compares bound members against snapshots; reconciles template DOM if differences are detected. Returns `true` if state changed. |
| `ElementHandle RootElement() const` | Handle to the root element of the expanded component template (null before mounting). |
| `ElementHandle QueryElement(std::string_view selector) const` | Handle to the first element matching `#id`, `.class`, or `tag`, or a null handle. |
| `ComponentBase* QueryComponent(std::string_view selector)` | The component rendered at `selector`, or `nullptr`. |
| `void CaptureMouse() noexcept` | Routes all subsequent mouse events exclusively to this component until released. |
| `void ReleaseMouse() noexcept` | Restores standard mouse hit-testing. |
| `void EnableHotReload()` | Watches the defining source file via `std::source_location` and dynamically reloads the template on disk modification. |

---

## 4. DOM Access: `rtxui::ElementHandle`

A weak, nullable handle to an element of the rendered tree, declared in `<rtxui/element.hpp>`. The element itself is internal; the handle is one pointer with out-of-line methods, so the element's layout never reaches the ABI.

- **Null-safe**: a query that matched nothing gives a null handle. On a null handle, getters return empty values and setters do nothing.
- **Weak**: a handle does not keep its element alive. Once a re-render destroys the element, the handle becomes null instead of dangling.
- **Template-shaped**: `<slot>` wrappers are transparent and text nodes are not children, so the tree matches the template as written.

### 4.1 Hierarchy & Traversal
| Method Signature | Description |
| :--- | :--- |
| `explicit operator bool() const` | `false` for a null handle. |
| `ElementHandle QuerySelector(std::string_view selector) const` | First descendant matching `#id`, `.class`, or `tag`. |
| `ElementHandle Parent() const` | Parent element, or a null handle at the root. |
| `size_t ChildCount() const` | Number of child elements. |
| `ElementHandle ChildAt(size_t index) const` | Child element by 0-based index, or a null handle when out of range. |

### 4.2 Content
| Method Signature | Description |
| :--- | :--- |
| `std::string tag() const` | Tag name, e.g. `"div"`. |
| `std::optional<std::string> GetAttribute(std::string_view name) const` | Attribute value, or `std::nullopt` if absent. |

### 4.3 Scrolling
| Method Signature | Description |
| :--- | :--- |
| `int scroll_x() const` / `int scroll_y() const` | Current scroll offsets in cells. |
| `void SetScrollX(int offset, bool smooth = false)` / `void SetScrollY(int offset, bool smooth = false)` | Sets the scroll position, optionally animated. Negative offsets become 0; offsets past the end are clamped by the next layout. |

---

## 5. Terminal Runtime: `rtxui::Screen`

Coordinates terminal I/O, device capability negotiation, and the primary event loop.

```cpp
class Screen {
 public:
  Screen(Ref<ComponentBase> root, std::shared_ptr<TerminalDevice> device = nullptr);
  ~Screen();

  void Loop();
  void Step();
  void Draw();
  void Dispatch(Event event);
  void SetSmoothScrollEnabled(bool enabled) noexcept;
};
```

### 5.1 Loop Execution
- `Loop()`: Enters raw terminal mode, hides the cursor, routes signals, and executes the blocking event loop until `Exit()` is triggered.
- `Step()`: Non-blocking execution unit: processes pending terminal I/O, runs scheduled tasks, reconciles dirty state (`Digest()`), and flushes frames to output.
- `Draw()`: Unconditionally executes the layout, paint, and screen diff pass. Output is synchronized using DEC Mode 2026 (`\x1b[?2026h` / `\x1b[?2026l`) to guarantee tear-free updates.

---

## 6. Input Event Model: `rtxui::Event`

Encapsulates terminal input events as a tagged variant.

```cpp
class Event {
 public:
  template <typename T> bool is() const noexcept;
  template <typename T> const T& get() const noexcept;
  template <typename T> const T* get_if() const noexcept;
};
```

### 6.1 `Event::Keyboard`
| Member | Type | Description |
| :--- | :--- | :--- |
| `codepoint` | `char32_t` | Decoded UTF-32 character value. |
| `special` | `Key` | Enumeration for special keys: `ArrowUp`, `Return`, `Escape`, `Tab`, `F1`–`F12`, etc. |
| `modifier` | `Modifier` | Bitmask for active modifiers: `Shift`, `Alt`, `Ctrl`, `Meta`. |
| `motion` | `Motion` | Interaction state: `Pressed`, `Repeat`, `Released`. |

### 6.2 `Event::Mouse`
| Member | Type | Description |
| :--- | :--- | :--- |
| `x` | `int` | 1-based column terminal coordinate. |
| `y` | `int` | 1-based row terminal coordinate. |
| `button` | `Button` | Mouse button: `None`, `Left`, `Middle`, `Right`, `WheelUp`, `WheelDown`. |
| `motion` | `Motion` | Mouse motion state: `Pressed`, `Released`, `Moved`. |

---

## 7. Color Representation: `rtxui::Color`

Represents 32-bit RGBA color values for cell background and foreground channels.

```cpp
struct Color {
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  uint8_t a = 255;

  static Color RGB(uint8_t r, uint8_t g, uint8_t b) noexcept;
  static Color RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) noexcept;
};

Color Blend(Color over, Color under) noexcept;
```

---

## 8. Asynchronous Task Scheduling: `<rtxui/task.hpp>`

Provides thread-safe task dispatching onto the primary UI event loop.

```cpp
namespace rtxui {
void PostTask(std::function<void()> task);
void PostDelayedTask(std::function<void()> task,
                     std::chrono::milliseconds delay);
}
```

`PostDelayedTask()` is a timer: the task runs on the event loop once `delay`
has passed, and the loop wakes for it on its own, so a change it makes to
bound state reaches the screen without waiting for input.

### Thread Invariant
`PostTask()` may be called from any thread. On a thread running an event loop it schedules onto that loop; from any other thread (a worker) it schedules onto the application's loop and wakes it immediately. A task posted after the event loop is gone is dropped.
