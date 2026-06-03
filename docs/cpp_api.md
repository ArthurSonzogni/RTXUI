# C++ API Reference

This reference catalog lists the public C++ classes, structures, and methods provided by the RTXUI runtime.

---

## 1. Components

### `rtxui::Component<Derived>`
The base class for defining reactive UI elements. Implemented using the Curiously Recurring Template Pattern (CRTP).

| Method | Signature | Description |
| :--- | :--- | :--- |
| `Setup` | `virtual std::string_view Setup() = 0` | Returns the component's HTML/XML layout template. |
| `InitReflection` | `virtual void InitReflection()` | Registers member fields for data-binding. Call the base implementation inside overrides. |
| `Bind` | `void Bind(T& ref)` | Binds a variable (int, bool, string) for template value interpolation. |
| `BindCollection` | `void BindCollection(std::string name, const Container* ptr)` | Binds a standard container (vector, list) for `<for>` loops. |
| `BindCollection` | `void BindCollection(std::string name, const Container* ptr, MapperFn mapper)` | Binds a container with a custom object property mapping function. |

---

## 2. DOM Primitives

### `rtxui::Element`
Represents an active DOM node in the parsed XML tree.

| Method / Property | Signature | Description |
| :--- | :--- | :--- |
| `Parent` | `Element* Parent()` | Returns a pointer to the parent DOM element. |
| `ChildCount` | `size_t ChildCount() const` | Returns the count of immediate children. |
| `ChildAt` | `Element* ChildAt(size_t index)` | Returns child node at the given index. |
| `children` | `const std::vector<Ref<Element>>& children() const` | Returns references to all child nodes. |
| `Attributes` | `const std::map<std::string, std::string>& Attributes() const` | Returns parsed XML attributes. |
| `scroll_x` | `int scroll_x() const` | Gets current horizontal scroll offset. |
| `set_scroll_x` | `void set_scroll_x(int x)` | Sets horizontal scroll position. |
| `scroll_width` | `int scroll_width() const` | Gets total width of scrollable content. |
| `scroll_y` | `int scroll_y() const` | Gets current vertical scroll offset. |
| `set_scroll_y` | `void set_scroll_y(int y)` | Sets vertical scroll position. |
| `scroll_height` | `int scroll_height() const` | Gets total height of scrollable content. |
| `QuerySelector` | `Element* QuerySelector(std::string_view selector)` | Finds a node matching a selector (e.g. `.class` or `#id`). |

---

## 3. Screen Runtime

### `rtxui::Screen`
Controls layout calculations, event dispatching, and rendering loops.

| Method | Signature | Description |
| :--- | :--- | :--- |
| `Screen` | `Screen(Ref<ComponentBase> component, std::shared_ptr<TerminalDevice> dev = nullptr)` | Mounts root component onto terminal screen renderer. |
| `Loop` | `void Loop()` | Enters interactive blocking terminal input loop. |
| `Draw` | `void Draw()` | Manually triggers layout updates and repaints (runs digest cycle). |
| `HandleEvent` | `void HandleEvent(const Event& event)` | Feeds a keyboard/mouse event to active element tree. |

---

## 4. Events & Color Primitives

### `rtxui::Event`
Represents standard keyboard, mouse click, and mouse scroll inputs.

| Method | Signature | Description |
| :--- | :--- | :--- |
| `is_mouse` | `bool is_mouse() const` | Checks if event represents a mouse click/move/scroll. |
| `ArrowLeft` | `static const Event& ArrowLeft()` | Static left arrow key event. |
| `ArrowRight` | `static const Event& ArrowRight()` | Static right arrow key event. |
| `ArrowUp` | `static const Event& ArrowUp()` | Static up arrow key event. |
| `ArrowDown` | `static const Event& ArrowDown()` | Static down arrow key event. |
| `PageUp` | `static const Event& PageUp()` | Static page up key event. |
| `PageDown` | `static const Event& PageDown()` | Static page down key event. |
| `Escape` | `static const Event& Escape()` | Static Escape key event. |
| `CtrlC` | `static const Event& CtrlC()` | Static Ctrl-C key event. |

### `rtxui::Color`
Represents terminal color definitions.

| Method | Signature | Description |
| :--- | :--- | :--- |
| `RGB` | `static Color RGB(uint8_t r, uint8_t g, uint8_t b)` | Instantiates RGB color object. |
| `RGBA` | `static Color RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a)` | Instantiates RGBA color object with transparency. |
