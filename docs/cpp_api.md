# C++ Engine API & Architecture Specification

This specification defines the public C++ API of RTXUI (`#include <rtxui/rtxui.hpp>`), documenting system contracts, thread-safety invariants, exception guarantees, and the frame execution lifecycle.

---

## 1. Core Architectural Invariants

### 1.1 Concurrency Model
- **UI Thread Affinity**: All component construction, template compilation, DOM mutations, layout resolution, and `Screen::Draw()` operations **must** execute on the designated main UI thread. They are non-reentrant and not thread-safe.
- **Cross-Thread Synchronization**: Background worker threads communicate with UI state exclusively by dispatching callbacks via `rtxui::task::TaskRunner::Current()->PostTask(...)`. The runner queues tasks in a thread-safe atomic lock-free queue and wakes the terminal event loop via a POSIX signal pipe (`self-pipe`) or Windows event object.

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

Intrusive reference-counted pointer for `rtxui::ComponentBase` and `rtxui::Element` instances.

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
Components provide HTML structure via one of two mechanisms:
1. **`view` Member** (Recommended): A `std::string_view` member variable. Supports runtime [Hot Reload](/guide/hot-reload).
2. **`Setup()` Override**: A virtual method returning `std::string_view`.

```cpp
class CounterCard : public rtxui::Component<CounterCard> {
 public:
  int count = 0;

  void InitReflection() override {
    Bind(count);
    Bind("Increment", [this]() { count++; });
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
| `Element* Root() const noexcept` | Returns the root `Element` of the expanded component template. |
| `Ref<Element> Slot(std::string_view name)` | Accesses the slot anchor with the given identifier (`""` for default slot). |
| `void CaptureMouse() noexcept` | Routes all subsequent mouse events exclusively to this component until released. |
| `void ReleaseMouse() noexcept` | Restores standard mouse hit-testing. |
| `void EnableHotReload()` | Watches the defining source file via `std::source_location` and dynamically reloads the template on disk modification. |

---

## 4. DOM Node: `rtxui::Element`

Represents a node in the rendered element tree.

### 4.1 Hierarchy & Traversal
| Method Signature | Description |
| :--- | :--- |
| `Element* Parent() const noexcept` | Pointer to parent element, or `nullptr` at root. |
| `size_t ChildCount() const noexcept` | Count of direct child nodes. |
| `Element* ChildAt(size_t index) const` | Accesses child node by 0-based index. |
| `const std::vector<Ref<Element>>& children() const` | Read-only view of child element vector. |
| `Element* QuerySelector(std::string_view selector)` | First descendant matching `#id`, `.class`, or `tag`. |
| `void Visit(const std::function<void(Element&)>& visitor)` | Pre-order traversal over element subtree. |

### 4.2 Attributes & State
| Method Signature | Description |
| :--- | :--- |
| `const std::string* GetAttribute(const std::string& name) const` | Retrieves parsed attribute value, or `nullptr` if absent. |
| `bool focused() const noexcept` | `true` if element holds active keyboard focus. |
| `bool hovered() const noexcept` | `true` if mouse pointer coordinates intersect element bounds. |
| `bool active() const noexcept` | `true` if element is being pressed by mouse button or spacebar. |
| `bool checked() const noexcept` | `true` if toggle element (`checkbox`, `radio`) is checked. |
| `bool disabled() const noexcept` | `true` if element interaction is disabled. |

### 4.3 Geometry & Scrolling
| Method Signature | Description |
| :--- | :--- |
| `int absolute_x() const noexcept` | Physical screen X coordinate (columns) of top-left boundary. |
| `int absolute_y() const noexcept` | Physical screen Y coordinate (rows) of top-left boundary. |
| `int layout_width() const noexcept` | Computed border-box width in character cells. |
| `int layout_height() const noexcept` | Computed border-box height in character cells. |
| `int scroll_x() const noexcept` / `int scroll_y() const noexcept` | Current horizontal and vertical scroll offsets in cells. |
| `void set_scroll_x(int offset, bool smooth = false)` | Sets horizontal scroll position, optionally animated. |
| `void set_scroll_y(int offset, bool smooth = false)` | Sets vertical scroll position, optionally animated. |
| `int scroll_width() const noexcept` / `int scroll_height() const noexcept` | Extent of scrollable inner content. |

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

## 8. Asynchronous Task Scheduler: `rtxui::task::TaskRunner`

Provides thread-safe task dispatching onto the primary UI event loop.

```cpp
namespace task {
class TaskRunner {
 public:
  static TaskRunner* Current() noexcept;
  void PostTask(Task task);
  void PostDelayedTask(Task task, std::chrono::steady_clock::duration delay);
};
}
```

### Thread Invariant
`TaskRunner::Current()` returns the runner associated with the calling thread. The main UI runner must be captured on the UI thread and distributed to worker threads. Calling `PostTask()` from any background thread thread-safely enqueues the callable and unblocks the screen event loop immediately.
