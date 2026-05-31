# RTXUI C++ API Reference

This reference documents the core public C++ API classes and primitives of RTXUI.

---

## 1. Components

### `rtxui::Component<Derived>`
The base class for defining reactive components. Every component should inherit from this class using the Curiously Recurring Template Pattern (CRTP).

```cpp
template <typename Derived>
class Component : public ComponentBase {
 public:
  virtual ~Component() = default;

  // The view template string defining the XML structure.
  // Must return valid XML/HTML containing bindings.
  virtual std::string_view Setup() = 0;

  // Optional: override to register child props or reactive hooks
  virtual void InitReflection() {
    ComponentBase::InitReflection();
  }

  // Registers a member variable for two-way binding.
  void Bind(T& ref);

  // Registers a range/collection for use with the <for> tag.
  void BindCollection(std::string name, const Container* ptr);

  // Registers a range with a custom visitor for complex objects.
  void BindCollection(std::string name, const Container* ptr, MapperFn mapper);
};
```

---

## 2. DOM Primitives

### `rtxui::Element`
Represents a DOM node parsed from the XML view template. Elements store attributes, reactive state, and inline ComputedStyle definitions.

```cpp
class Element : public RefCounted {
 public:
  // DOM Navigation
  Element* Parent();
  size_t ChildCount() const;
  Element* ChildAt(size_t index);
  const std::vector<Ref<Element>>& children() const;

  // Attributes
  const std::map<std::string, std::string>& Attributes() const;

  // Scroll Position accessors (Horizontal & Vertical)
  int scroll_x() const;
  void set_scroll_x(int x);
  int scroll_width() const;

  int scroll_y() const;
  void set_scroll_y(int y);
  int scroll_height() const;

  // Query selector for locating elements (e.g. "#id" or ".class")
  Element* QuerySelector(std::string_view selector);
};
```

---

## 3. Terminal & Lifecycle

### `rtxui::Screen`
The main screen runtime that mounts the root component, receives events from the terminal input, and runs the digest cycle.

```cpp
class Screen {
 public:
  // Creates a screen rendering target.
  Screen(Ref<ComponentBase> component, std::shared_ptr<TerminalDevice> device = nullptr);

  // Starts the main interactive event loop.
  void Loop();

  // Forces a layout rebuild and repaint iteration (the Digest cycle).
  void Draw();

  // Manually feeds a keyboard or mouse event to the active component tree.
  void HandleEvent(const Event& event);
};
```

---

## 4. Input & Color Formatting

### `rtxui::Event`
Wraps terminal input events (keyboard key strokes, mouse clicks, mouse scrolling wheel).

```cpp
class Event {
 public:
  bool is_mouse() const;
  
  // Pre-configured static key events:
  static const Event& ArrowLeft();
  static const Event& ArrowRight();
  static const Event& ArrowUp();
  static const Event& ArrowDown();
  static const Event& PageUp();
  static const Event& PageDown();
  static const Event& Escape();
  static const Event& CtrlC();
};
```

### `rtxui::Color`
Representation of colors (supporting standard terminal RGB/RGBA).

```cpp
struct Color {
  static Color RGB(uint8_t r, uint8_t g, uint8_t b);
  static Color RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
};
```
