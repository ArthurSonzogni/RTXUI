# Event Handlers & Input Bindings

RTXUI maps terminal interactions (such as mouse clicks, keyboard presses, or input changes) to registered C++ callback functions.

---

## 1. Registering Callbacks in C++

To bind a member function to template events, register it in your component constructor using the `Bind()` method. To register lambda expressions or standalone functions, import them using the `Import()` method:

```cpp
struct ClickApp : public rtxui::Component<ClickApp> {
  int clicks = 0;

  void Increment() { clicks++; }

  ClickApp() {
    Bind(clicks);
    
    // Bind member function callback
    Bind(Increment);
    
    // Import a lambda with parameters
    Import("Reset", [this]() { clicks = 0; });
  }

  std::string_view Setup();
};
```

---

## 2. Event Attributes

Once registered, a callback is wired to an element with an `on<event>`
attribute, as in HTML. The attribute value is the name of a bound callback:
```html
<button onclick="Increment">Increment</button>
```

---

## 3. Right Click

`onclick` takes the left click. `oncontextmenu` takes the right click:

```html
<button oncontextmenu="ShowContextMenu">Options</button>
```

---

## 4. Keyboard Shortcuts

`onkey:<key>` runs a handler when that key is pressed:

```cpp
struct ListView : public rtxui::Component<ListView> {
  int cursor = 0;

  void Down() { cursor++; }
  void Up() { cursor--; }
  void Jump(int to) { cursor = to; }

  ListView() {
    Bind(cursor);
    Bind(Down);
    Bind(Up);
    Bind(Jump);
  }

  std::string_view view = R"html(
    <div onkey:j="Down" onkey:ArrowDown="Down"
         onkey:k="Up" onkey:ArrowUp="Up"
         onkey:g="Jump(0)" onkey:ctrl+d="Jump(10)">
      <span>Row {cursor}</span>
    </div>
  )html";
};
```

A key is named as one of these:

- a character: `j`, `G`, `?`, `/`. An uppercase letter is the shifted key;
  `shift+g` means the same as `G`.
- `Space`, or a special key: `Return` (or `Enter`), `Escape`, `Tab`,
  `Backspace`, `Delete`, `Insert`, `ArrowUp`, `ArrowDown`, `ArrowLeft`,
  `ArrowRight`, `Home`, `End`, `PageUp`, `PageDown`, `F1` to `F12`.
- either of those after modifiers: `ctrl+d`, `alt+x`, `ctrl+alt+x`,
  `shift+Tab`, `ctrl+ArrowDown`.

A plain `onkey="Handler"` runs for every key. Its handler can take the key as
an `rtxui::Event::Keyboard` argument:

<!-- snippet: fragment -->
```cpp
bool Typed(rtxui::Event::Keyboard key) {
  log += key.Print();
  return false;  // Let the key go on.
}
```

**Which handlers run.** The key goes from the focused element up through its
parents to the application's root. With nothing focused, the handlers on the
application template's top-level elements run. On each element, the handler
for the key runs before the one for every key. The first handler that runs
takes the key, unless it returns `false`: then the key goes on, to the next
handler and, past the last one, to what the key does by default (<kbd>Tab</kbd>
moves the focus, <kbd>Enter</kbd> clicks the focused button, arrows move it).

**What runs first.** A focused `<input>`, `<textarea>` or `<select>` keeps the
keys it uses: typing `j` into a search box types a `j`. An `OnEvent()`
override also sees a key before the `onkey` handlers do. Pasted text is
never taken as a shortcut.

A key name that is no key, such as `onkey:Arow`, is reported as a
[diagnostic](/guide/diagnostics).

---

## 5. Handler Arguments

A handler can take arguments, written in parentheses after its name. Each one
is read from the template's text into the parameter's type:

```cpp
struct ListApp : public rtxui::Component<ListApp> {
  std::vector<std::string> items = {"a", "b", "c"};
  int width = 0;
  int height = 0;

  void Remove(int index) { items.erase(items.begin() + index); }
  void Resize(int w, int h) {
    width = w;
    height = h;
  }

  ListApp() {
    Bind(items);
    Bind(Remove);
    Bind(Resize);
  }

  std::string_view view = R"html(
    <div>
      <button onclick="Resize(80, 24)">80x24</button>
      <for each="{items}" as="item">
        <div onclick="Remove({$index})">{item}</div>
      </for>
    </div>
  )html";
};
```

Inside a [`<for>` loop](/guide/loops), `{$index}` is the current item's index,
and `{item.field}` any field of the item.

| Parameter type | Template text |
| :--- | :--- |
| `int`, `size_t`, any integer | `Remove(3)` |
| `float`, `double` | `Zoom(1.5)` |
| `bool` | `Show(true)`, `Show(0)` |
| `std::string`, `std::string_view` | `Open(notes.txt)`: the text as written, without quotes |
| an enum | `SetSort(kSize)`: the enumerator's name, with C++26 reflection |
| your own type, or an enum without reflection | what your `bool Parse(std::string_view, T&)` function, declared next to the type, accepts |

Several arguments are separated by commas: `Resize(80, 24)` calls
`void Resize(int w, int h)`. A handler taking a single string receives
everything between the parentheses, commas included.

Text that does not convert, such as `Remove(abc)` for an `int`, or the wrong
number of arguments, is reported as a [diagnostic](/guide/diagnostics), and
the handler does not run.

A lambda imported with `Import("Name", [this](std::string arg) { ... })`
receives the text as is.

---

## 6. Two-Way Data Binding

For interactive input widgets, RTXUI supports automatic two-way data bindings. When a user types text or checks a box, the bound C++ member variable is updated instantly, and programmatically changing the C++ value repaints the input field:

*   **`<input>` / `<textarea>`**: Bind the `value` attribute to a reactive string.
*   **`<checkbox>`**: Bind the `checked` attribute to a reactive boolean.
*   **`<select>`**: Bind the `value` attribute to a reactive string matching the selected option.

```html
<input value="{search_query}" />
<checkbox checked="{is_enabled}">Toggle Features</checkbox>
```

---

## Interactive Demo

Below is the interactive tab view for two-way input field bindings:

<ExampleTabs src="/wasm/rtxui_example_input.js">
<template #source>

<<< @/../example/input.cpp

</template>
</ExampleTabs>
