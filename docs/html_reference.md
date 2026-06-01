# RTXUI Built-in Component Reference

This reference lists all the built-in HTML/XML elements supported by RTXUI out-of-the-box. In RTXUI, these elements are implemented as default C++ components, meaning you must import them in your component's reflection setup (e.g., `Import<rtxui::div>()`) to use them in templates.

---

## Component List

| Tag | Category | Default CSS Display | Description |
| :--- | :--- | :--- | :--- |
| `<div>` | Block | `block` | General block container. Stacks children vertically. |
| `<span>` | Inline | `inline` | General inline container. Flows children horizontally. |
| `<h1>` | Block | `block` | Page or section heading with underlined, bold text and bottom margin. |
| `<p>` | Block | `block` | Paragraph block with top and bottom margins. |
| `<strong>` | Inline | `inline` | Renders bold text. |
| `<ul>` | Block | `block` | Unordered list container with left indentation. |
| `<ol>` | Block | `block` | Ordered list container with left indentation. |
| `<li>` | Block | `block` | A single item inside a list. |
| `<button>` | Block | `inline-block` | Interactive clickable button with borders and padding. |
| `<input>` | Block | `inline flex` | Interactive single-line text input field. |
| `<textarea>` | Block | `block` | Interactive multi-line text editing area with vertical scrolling. |
| `<checkbox>` | Inline | `inline-block` | Interactive binary checkbox/toggle component. |
| `<slider>` | Inline | `inline-block` | Interactive horizontal slider/range control. |
| `<progress>` | Inline | `inline-block` | Horizontal block-level progress bar. |
| `<select>` | Block | `inline flex` | Dropdown selection list. |
| `<option>` | Block | `block` | A selectable option item inside a `<select>`. |
| `<hr>` | Block | `block` | A horizontal rule/separator line. |
| `<if>` | Logic | N/A | Conditional rendering block. |
| `<elif>` | Logic | N/A | Alternative conditional block (must follow `<if>` or `<elif>`). |
| `<else>` | Logic | N/A | Fallback conditional block (must follow `<if>` or `<elif>`). |

---

## Global Attributes

The following attributes and syntaxes are supported on all elements (including custom components):

### Data Binding
You can bind element attributes to reactive state or computed properties using two styles:
- **React-style**: `attribute="{expression}"`
- **Vue-style**: `:attribute="expression"` (Short for `attribute="{expression}"`)

### Event Listeners
Interactive elements support event listeners using two styles:
- **React-style**: `onclick="handler"`, `oncontextmenu="handler"`, `onchange="handler"`.
- **Vue-style**: `@click="handler"`, `@click.right="handler"`, `@change="handler"`.

### `if`
Specifies a condition for rendering the element.
- **Value**: An expression that evaluates to `true` or `1` for the element to be rendered.
- **Example**:
  ```html
  <div if="{is_visible}">Visible Content</div>
  <!-- or -->
  <div :if="is_visible">Visible Content</div>
  ```

### `tabindex`
Specifies the sequential keyboard focus navigation order of the element.
- **Values**:
  - `tabindex >= 0` (e.g. `0`, `1`, `2`): The element is sequentially focusable using the **Tab** and **Shift-Tab** keys. Positive values determine the order of focus traversal (lower values first), followed by elements with `tabindex="0"` in document order.
  - `tabindex="-1"`: The element is focusable (e.g., clickable or programmatically focusable), but is excluded from sequential tab focus traversal.
- **Example**:
  ```html
  <div tabindex="1">First in Tab Order</div>
  <div tabindex="0">Second in Tab Order</div>
  <div tabindex="-1">Excluded from Tab order, but still focusable</div>
  ```

### `focusable`
A boolean attribute specifying if an element is focusable. If set to `true`, it is equivalent to setting `tabindex="0"`.
- **Example**:
  ```html
  <div focusable="true">Interactive Div</div>
  ```

---

## `&lt;if&gt;` &amp; `&lt;elif&gt;` &amp; `&lt;else&gt;`
Used for block-level conditional rendering.
- **Attributes**:
  - `condition` (on `<if>` and `<elif>`): Expression to evaluate.
- **Behavior**: Consecutive blocks form a chain; only the first block whose condition is met (or the `<else>` block if no conditions match) will be rendered. Whitespace and comments between blocks are ignored.
- **Example**:
  ```html
  <if condition="{status == 'loading'}">
    <progress value="50" />
  </if>
  <elif condition="{status == 'error'}">
    <span>Error occurred!</span>
  </elif>
  <else>
    <span>Loaded successfully.</span>
  </else>
  ```

---

## `&lt;div&gt;`
Used as a block-level wrapper to group elements vertically.
- **Default Styles**:
  ```css
  self { display: block; }
  ```
- **Example**:
  ```html
  <div>
    <span>First Line</span>
    <span>Second Line</span>
  </div>
  ```

## `&lt;span&gt;`
Used as an inline wrapper for text elements.
- **Default Styles**:
  ```css
  self { display: inline; }
  ```

## `&lt;h1&gt;`
Renders a major section heading.
- **Default Styles**:
  ```css
  self { 
    display: block; 
    font-weight: bold;
    text-decoration: underlined;
    margin-bottom: 1;
  }
  ```

## `&lt;p&gt;`
Used to format paragraph blocks.
- **Default Styles**:
  ```css
  self { 
    display: block; 
    margin-top: 1;
    margin-bottom: 1;
  }
  ```

## `&lt;strong&gt;`
Highlights inline text with bold styling.
- **Default Styles**:
  ```css
  self { 
    display: inline; 
    font-weight: bold;
  }
  ```

## `&lt;ul&gt;` &amp; `&lt;ol&gt;` &amp; `&lt;li&gt;`
Used to create structured lists.
- **Default Styles**:
  ```css
  ul, ol { 
    display: block; 
    padding-left: 2;
  }
  li { 
    display: block; 
  }
  ```

## `&lt;button&gt;`
An interactive button that responds to left-click and right-click actions.
- **Default Styles**:
  ```css
  self { 
    display: inline-block; 
    border: tall;
    padding-left: 1;
    padding-right: 1;
  }
  ```
- **Attributes**:
  - `onclick` or `@click.left` / `@click`: Specifies the name of the callback function imported into the component's bindings.
  - `oncontextmenu` or `@click.right`: Callback triggered on a mouse right-click event.
- **Example**:
  ```html
  <button onclick="MyCallback">Click Me</button>
  ```

## `&lt;input&gt;`
An interactive single-line text entry field.
- **Default Styles**:
  ```css
  self {
    display: inline flex;
    flex-direction: row;
    border: solid;
    border-color: #555;
    padding-left: 1;
    padding-right: 1;
    overflow-x: scroll;
    scrollbar-width: none;
    white-space: nowrap;
  }
  ```
- **Attributes**:
  - `value`: Two-way reactive string binding. Modifying the text updates the state variable, and programmatically changing the state updates the text field.
- **Built-in Keyboard & Mouse Bindings**:
  - `ArrowLeft` / `ArrowRight`: Moves the cursor character-by-character (correctly navigating multi-byte combining marks and double-width CJK character cells).
  - `Ctrl + ArrowLeft` / `Ctrl + ArrowRight`: Moves the cursor past space-demarcated word boundaries.
  - `Backspace` / `Delete`: Deletes the character before/after the cursor.
  - `Ctrl + Backspace` / `Ctrl + Delete`: Deletes the word segment to the left/right.
  - Left Mouse Click: Focuses the input element and positions the cursor at the closest grapheme boundary closest to the click coordinates.
  - Auto-Scrolling: If the input text length exceeds the input element's bounds, it automatically scrolls horizontally (`scroll_x`) to keep the cursor visible.
- **Example**:
  ```html
  <input value="{search_query}" />
  ```

- **Interactive Demo**:
  
<ExampleTabs src="/wasm/rtxui_example_input.js">
<template #source>

<<< @/../example/input.cpp

</template>
</ExampleTabs>


---

## `&lt;textarea&gt;`
An interactive multi-line text editing area. Shares the same event-handling base as `<input>` but supports newlines, vertical cursor navigation, and vertical scrolling.
- **Default Styles**:
  ```css
  self {
    display: block;
    border: solid;
    border-color: #555;
    padding-left: 1;
    padding-right: 1;
    overflow-y: scroll;
  }
  ```
- **Attributes**:
  - `value`: Two-way reactive string binding. The string may contain `\n` newline characters. Programmatic changes update the editor, and user edits update the bound variable.
- **Built-in Keyboard & Mouse Bindings**:
  - `ArrowLeft` / `ArrowRight`: Moves the cursor character-by-character.
  - `ArrowUp` / `ArrowDown`: Moves the cursor to the same visual column on the previous/next line, preserving the *ideal column* across empty lines.
  - `Home` / `End`: Moves the cursor to the start or end of the **current line** (not the whole value).
  - `Ctrl + ArrowLeft` / `Ctrl + ArrowRight`: Moves the cursor past space-delimited word boundaries.
  - `Enter` (`Return`): Inserts a `\n` newline at the cursor position.
  - `Backspace` / `Delete`: Deletes the character/grapheme before/after the cursor. Backspace at the start of a line joins it with the previous line.
  - `Ctrl + Backspace` / `Ctrl + Delete`: Deletes the word segment to the left/right.
  - Left Mouse Click: Focuses the textarea and positions the cursor at the closest grapheme boundary to the click coordinates (both row and column).
  - Auto-Scrolling: Automatically scrolls vertically (`scroll_y`) to keep the cursor line visible within the element's bounds.
- **Example**:
  ```html
  <textarea value="{notes}" />
  ```

- **Interactive Demo**:
  
<ExampleTabs src="/wasm/rtxui_example_textarea.js">
<template #source>

<<< @/../example/textarea.cpp

</template>
</ExampleTabs>


---

## `&lt;checkbox&gt;`
An interactive toggle control for boolean values.
- **Default Styles**:
  ```css
  self {
    display: inline-block;
    cursor: pointer;
  }
  .focused {
    background-color: #333;
    color: #fff;
  }
  .checkmark {
    font-weight: bold;
    color: #38bdf8;
  }
  ```
- **Attributes**:
  - `checked`: Reactive boolean binding. Toggling state updates the variable.
  - `onchange`: Callback triggered when the checked state changes.
- **Built-in Keyboard & Mouse Bindings**:
  - `Space`: Toggles the checked state when focused.
  - Left Mouse Click: Focuses and toggles the checked state.
- **Example**:
  ```html
  <checkbox checked="{is_enabled}" onchange="ToggleEnabled">Enable Notifications</checkbox>
  ```

- **Interactive Demo**:

<ExampleTabs src="/wasm/rtxui_example_checkbox.js">
<template #source>

<<< @/../example/checkbox.cpp

</template>
</ExampleTabs>


---

## `&lt;slider&gt;`
An interactive horizontal range slider component.
- **Default Styles**:
  ```css
  self {
    display: inline-block;
    cursor: pointer;
  }
  .focused {
    background-color: #333;
    color: #fff;
  }
  .track-left {
    color: #38bdf8;
  }
  .track-right {
    color: #555;
  }
  .thumb {
    font-weight: bold;
    color: #38bdf8;
  }
  ```
- **Attributes**:
  - `value`: Reactive integer value binding.
  - `min`: Minimum bound (default `0`).
  - `max`: Maximum bound (default `100`).
  - `step`: Step value increment (default `1`).
  - `width`: Total layout track width in characters (default `20`).
  - `onchange`: Callback triggered when the value changes.
- **Built-in Keyboard & Mouse Bindings**:
  - `ArrowLeft` / `ArrowDown`: Decreases the value by `step`.
  - `ArrowRight` / `ArrowUp`: Increases the value by `step`.
  - Left Mouse Click: Focuses the slider and sets the value proportional to the clicked column on the track.
- **Example**:
  ```html
  <slider value="{volume}" min="0" max="100" step="5" width="20" />
  ```

- **Interactive Demo**:

<ExampleTabs src="/wasm/rtxui_example_slider.js">
<template #source>

<<< @/../example/slider.cpp

</template>
</ExampleTabs>


---

## `&lt;progress&gt;`
A read-only horizontal progress bar indicator.
- **Default Styles**:
  ```css
  self {
    display: inline-block;
  }
  .filled {
    color: #38bdf8;
  }
  .empty {
    color: #444;
  }
  ```
- **Attributes**:
  - `value`: Current progress value.
  - `max`: Maximum range value (default `100`).
  - `width`: Bar width in characters (default `20`).
- **Example**:
  ```html
  <progress value="{percentage}" max="100" width="30" />
  ```

- **Interactive Demo**:

<ExampleTabs src="/wasm/rtxui_example_progress.js">
<template #source>

<<< @/../example/progress.cpp

</template>
</ExampleTabs>


---

## `&lt;select&gt;` &amp; `&lt;option&gt;`
An interactive dropdown menu for picking from a list of options.
- **Default Styles**:
  ```css
  self {
    display: inline flex;
    flex-direction: column;
  }
  .select-btn {
    display: flex;
    flex-direction: row;
    justify-content: space-between;
    border: solid;
    border-color: #555;
    background-color: #1e293b;
    color: white;
    padding-left: 1;
    padding-right: 1;
    cursor: pointer;
  }
  .focused {
    border-color: #38bdf8;
  }
  .dropdown-list {
    display: flex;
    flex-direction: column;
    border: solid;
    border-top: none;
    border-color: #555;
    background-color: #0f172a;
  }
  .closed {
    display: none;
  }
  ```
- **Attributes**:
  - `value`: Reactive string value binding matching the selected `<option>`'s `value`.
  - `onchange`: Callback triggered when the selection changes.
- **Built-in Keyboard & Mouse Bindings**:
  - Left Mouse Click (on button): Toggles the dropdown list.
  - Left Mouse Click (on option): Selects the option and closes the dropdown.
  - `ArrowDown` / `ArrowUp` (when dropdown is open): Moves the selection highlight.
  - `ArrowDown` / `ArrowUp` (when dropdown is closed): Cycles selection value directly.
  - `Enter` / `Space` (when dropdown is open): Selects the highlighted option.
  - `Escape` (when dropdown is open): Closes the dropdown without selection changes.
- **Example**:
  ```html
  <select value="{selected_theme}">
    <option value="dark">Dark Theme</option>
    <option value="light">Light Theme</option>
  </select>
  ```

- **Interactive Demo**:

<ExampleTabs src="/wasm/rtxui_example_select.js">
<template #source>

<<< @/../example/select.cpp

</template>
</ExampleTabs>


---

## `&lt;hr&gt;`
A block-level horizontal separator rule. Renders a continuous horizontal line `─` across the layout width.
- **Default Styles**:
  ```css
  self {
    display: block;
    margin-top: 1;
    margin-bottom: 1;
    overflow: hidden;
    white-space: nowrap;
  }
  .hr-span {
    color: #555;
  }
  ```
- **Example**:
  ```html
  <hr />
  ```

- **Interactive Demo**:

<ExampleTabs src="/wasm/rtxui_example_hr.js">
<template #source>

<<< @/../example/hr.cpp

</template>
</ExampleTabs>


