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

`maxlength` caps how many graphemes `value` can grow to from user input
(typing, pasting, Enter, Tab-indent). A paste that would overflow it is
truncated to fit rather than rejected outright, matching a browser's
maxlength field. It only limits interactive edits — assigning a longer
string to the bound C++ variable directly is unaffected.

Ctrl+Z undoes; Ctrl+Y or Ctrl+Shift+Z redoes. Edits coalesce into a single
undo step while they're contiguous — typing "abc" without moving the
cursor undoes as one step, as do consecutive Backspace/Delete presses —
but moving the cursor, selecting text, pasting, cutting, Enter, or
Tab-indent each start a fresh step (so pasted text always undoes as a
whole, separately from typing before or after it). Undo/redo is blocked
while `readonly` or `disabled`.

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
unindent the current line. A vertical scrollbar (`scrollbar-width: auto` by
default, same as any other scrollable element) shows the scroll position;
set `scrollbar-width: none` on it to hide it, as `<input>` does for its
own horizontal scrollbar.

Selection, copy/cut, paste, `disabled`, `readonly`, `placeholder`,
`maxlength`, and undo/redo all work the same way as `<input>` (see above;
a multi-line placeholder string renders across multiple lines, same as
`value`; `maxlength` also blocks Enter/Tab-indent once there's no room
left; Enter and Tab-indent each undo as their own step, never merged with
surrounding typing). A newline pasted into a
single-line `<input>` is dropped; pasting into a `<textarea>` inserts the
newlines as-is without
triggering the auto-indent that a manually-typed Enter gets. Under
`readonly`, Enter and Tab/Shift-Tab (which would otherwise insert a newline
or indent) are no-ops too.

Set `linenumbers` to `true` (or `absolute`) to show a line-number gutter, or
to `relative` for vim-style relative numbers (the active line shows its
absolute number; every other line shows its distance from it). `line_start`
offsets the displayed numbers (e.g. `41` to show the field as an excerpt
starting at line 41 of a larger file); `line_end` blanks the gutter past a
given displayed number (unbounded by default). `line_wrap="subline"` gives
word-wrapped continuation rows their own blank gutter entry instead of
letting the logical-line numbering drift out of alignment with them; this
is a wrap-count estimate, not an exact one.

Set `highlight_current_line="true"` for a full-width background band behind
the line the cursor is on, independently of `linenumbers` (works with or
without a gutter).

Several internal elements expose [`::part()`](/guide/css/basics) names so an
app can theme them: `part="gutter"` on the gutter container, `part="line-number"`
on every row (also `active` on the active line's row, and `wrapped` on a
subline continuation row), `part="line"` on every `highlight_current_line`
row (also `current-line` on the active one), and — regardless of
`linenumbers`/`highlight_current_line` — `part="selection"` on the
selected-text span, `part="cursor"` (also `cursor-focused` while focused) on
the cursor cell, and `part="placeholder"` on the placeholder text.

```html
<textarea value="{text}" />
<textarea value="{text}" linenumbers="true" />
<textarea value="{text}" linenumbers="true" highlight_current_line="true" />
```

```css
textarea::part(gutter)       { color: rgb(100, 116, 139); }
textarea::part(active)       { color: rgb(129, 140, 248); }
textarea::part(current-line) { background-color: rgb(49, 55, 79); }
textarea::part(selection)    { background-color: rgb(67, 56, 202); }
textarea::part(placeholder)  { color: rgb(100, 116, 139); }
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
