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

---

## Component Details

### `<div>`
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

### `<span>`
Used as an inline wrapper for text elements.
- **Default Styles**:
  ```css
  self { display: inline; }
  ```

### `<h1>`
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

### `<p>`
Used to format paragraph blocks.
- **Default Styles**:
  ```css
  self { 
    display: block; 
    margin-top: 1;
    margin-bottom: 1;
  }
  ```

### `<strong>`
Highlights inline text with bold styling.
- **Default Styles**:
  ```css
  self { 
    display: inline; 
    font-weight: bold;
  }
  ```

### `<ul>` & `<ol>` & `<li>`
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

### `<button>`
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

### `<input>`
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
