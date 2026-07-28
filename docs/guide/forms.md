# Form Elements

RTXUI provides a set of interactive form elements for building terminal UIs
with user input. Each element supports two-way data binding through the `{}`
syntax — when the user interacts with a form control, the bound C++ variable
updates automatically, and vice versa.

---

## Input (`<input>`)

A single-line text entry field. Bind a `std::string` member to the `value`
attribute for two-way updates. Supports arrow-key navigation (Ctrl to move by
word), deletion (Ctrl to delete words), mouse click positioning, and horizontal
scrolling on overflow.

Text selection works via Shift+arrow keys, double-click (selects a word) and
click-drag, or Ctrl+A (select all). With an active selection, Ctrl+C copies
and Ctrl+X cuts to the system clipboard (via the terminal's OSC 52 escape
sequence, so it works over SSH without a host-side clipboard tool). Pasting
uses the terminal's own paste action: RTXUI enables bracketed paste mode, so
pasted text is inserted as a single update rather than one keystroke per
character.

Set `disabled="true"` (or bind it, e.g. `disabled="{is_disabled}"`) to remove
the field from keyboard/mouse interaction and tab order entirely; it matches
the `:disabled` CSS pseudo-class, which the default style uses to dim it.
Set `readonly="true"` to keep the field focusable, selectable, and copyable
while blocking anything that would change its value (typing, paste,
backspace/delete, cut); it matches `:read-only`.

`placeholder` shows hint text (styled dim by default, via a `.placeholder`
class) whenever `value` is empty, and never touches `value` itself.

```html
<input value="{text}" />
```

```cpp
class MyApp : public Component<MyApp> {
  std::string text = "Hello, RTXUI!";

  std::string_view view = R"html(
    <input value="{text}" />
  )html";

  MyApp() { Bind(text); }
};
```

<ExampleTabs src="/wasm/rtxui_example_input.js">
<template #source>

<<< @/../example/input.cpp

</template>
</ExampleTabs>

---

## Textarea (`<textarea>`)

A multi-line scrollable text editor. Bind a `std::string` to `value`. Supports
arrow-key navigation, Enter for newlines, Home/End for line boundaries, and
Ctrl+Backspace/Delete for word deletion. Enter carries over the current
line's leading indentation onto the new line; Tab/Shift-Tab indent and
unindent the current line.

Selection, copy/cut, paste, `disabled`, `readonly`, and `placeholder` all
work the same way as `<input>` (see above; a multi-line placeholder string
renders across multiple lines, same as `value`). A newline pasted into a
single-line `<input>` is dropped; pasting into a `<textarea>` inserts the
newlines as-is without
triggering the auto-indent that a manually-typed Enter gets. Under
`readonly`, Enter and Tab/Shift-Tab (which would otherwise insert a newline
or indent) are no-ops too.

```html
<textarea value="{text}" />
```

```cpp
class MyApp : public Component<MyApp> {
  std::string text = "Line one\nLine two\n";

  std::string_view view = R"html(
    <textarea value="{text}" />
  )html";

  MyApp() { Bind(text); }
};
```

<ExampleTabs src="/wasm/rtxui_example_textarea.js">
<template #source>

<<< @/../example/textarea.cpp

</template>
</ExampleTabs>

---

## Checkbox (`<checkbox>`)

A boolean toggle. Bind a `bool` member to the `checked` attribute. Click or
press Space when focused to toggle. The element's text content becomes the
label.

```html
<checkbox checked="{enabled}">Enable Notifications</checkbox>
```

```cpp
class MyApp : public Component<MyApp> {
  bool enabled = false;

  std::string_view view = R"html(
    <checkbox checked="{enabled}">Enable Notifications</checkbox>
  )html";

  MyApp() { Bind(enabled); }
};
```

<ExampleTabs src="/wasm/rtxui_example_checkbox.js">
<template #source>

<<< @/../example/checkbox.cpp

</template>
</ExampleTabs>

---

## Slider (`<slider>`)

A range slider control. Bind an `int` to `value` and configure the range with
`min`, `max`, `step`, and `width` attributes. Drag the thumb with the mouse
or use Arrow Keys when focused.

```html
<slider value="{volume}" min="0" max="100" step="5" width="30" />
```

```cpp
class MyApp : public Component<MyApp> {
  int volume = 50;

  std::string_view view = R"html(
    <slider value="{volume}" min="0" max="100" step="5" width="30" />
    <span>{volume}%</span>
  )html";

  MyApp() { Bind(volume); }
};
```

| Attribute | Type  | Description                          |
|-----------|-------|--------------------------------------|
| `value`   | `int` | Current value (two-way bound)        |
| `min`     | `int` | Minimum value (default `0`)          |
| `max`     | `int` | Maximum value (default `100`)        |
| `step`    | `int` | Increment per step (default `1`)     |
| `width`   | `int` | Display width in terminal columns    |

<ExampleTabs src="/wasm/rtxui_example_slider.js">
<template #source>

<<< @/../example/slider.cpp

</template>
</ExampleTabs>

---

## Progress Bar (`<progress>`)

A non-interactive progress indicator. Bind an `int` to `value` and set the
`max` attribute to define the range. Use `width` to control the bar length.

```html
<progress value="{progress_val}" max="100" width="30" />
```

```cpp
class MyApp : public Component<MyApp> {
  int progress_val = 45;

  std::string_view view = R"html(
    <progress value="{progress_val}" max="100" width="30" />
    <span>{progress_val}%</span>
  )html";

  MyApp() { Bind(progress_val); }
};
```

| Attribute | Type  | Description                          |
|-----------|-------|--------------------------------------|
| `value`   | `int` | Current progress value (bound)       |
| `max`     | `int` | Maximum value (default `100`)        |
| `width`   | `int` | Display width in terminal columns    |

<ExampleTabs src="/wasm/rtxui_example_progress.js">
<template #source>

<<< @/../example/progress.cpp

</template>
</ExampleTabs>

---

## Select / Dropdown (`<select>`)

A dropdown picker menu. Bind a `std::string` to `value`. Children are
`<option>` elements whose `value` attributes determine the bound string.
Click to open, or focus with Tab and use Enter/Space. Navigate options with
ArrowUp/ArrowDown and confirm with Enter.

```html
<select value="{my_theme}">
  <option value="dark">Dark Theme</option>
  <option value="light">Light Theme</option>
  <option value="solarized">Solarized</option>
</select>
```

```cpp
class MyApp : public Component<MyApp> {
  std::string my_theme = "light";

  std::string_view view = R"html(
    <select value="{my_theme}">
      <option value="dark">Dark Theme</option>
      <option value="light">Light Theme</option>
      <option value="solarized">Solarized</option>
    </select>
  )html";

  MyApp() { Bind(my_theme); }
};
```

<ExampleTabs src="/wasm/rtxui_example_select.js">
<template #source>

<<< @/../example/select.cpp

</template>
</ExampleTabs>

---

## Label (`<label>`)

A helper element that delegates mouse click events and focus to an associated interactive element (like a checkbox or text input).
It can associate either explicitly via the `for` attribute (matching the target element's `id`), or implicitly by nesting the target element inside the `<label>`.

### Explicit Association (via `for` attribute)
```html
<label for="my-checkbox">Notification Settings</label>
<checkbox id="my-checkbox" checked="{enabled}">Enable Alerts</checkbox>
```

### Implicit Association (via Nesting)
```html
<label>
  Accept Terms
  <checkbox checked="{accepted}">Accept</checkbox>
</label>
```

<ExampleTabs src="/wasm/rtxui_example_label.js">
<template #source>

<<< @/../example/label.cpp

</template>
</ExampleTabs>
