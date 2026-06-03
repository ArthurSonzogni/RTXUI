# HTML Element Reference

This reference documents all built-in XML/HTML tags supported by the RTXUI parser, including their default CSS styles, parameters, and attribute options.

---

## 1. Tag Reference Table

| Tag | Category | Default CSS Style | Key Attributes | Description |
| :--- | :--- | :--- | :--- | :--- |
| `<div>` | Container | `display: block;` | None | General block layout wrapper. Stacks children vertically. |
| `<span>` | Container | `display: inline;` | None | General inline layout wrapper. Flows children horizontally. |
| `<h1>` | Typography | `display: block; font-weight: bold; text-decoration: underlined; margin-bottom: 1;` | None | Major section heading with bottom spacing. |
| `<p>` | Typography | `display: block; margin-top: 1; margin-bottom: 1;` | None | Paragraph layout text block. |
| `<strong>` | Typography | `display: inline; font-weight: bold;` | None | Highlights inline text in bold. |
| `<ul>` | List | `display: block; padding-left: 2;` | None | Unordered list block. |
| `<ol>` | List | `display: block; padding-left: 2;` | None | Ordered list block. |
| `<li>` | List | `display: block;` | None | Individual list item. |
| `<button>` | Interactive | `display: inline-block; border: tall; padding-left: 1; padding-right: 1;` | `onclick` / `@click`, `oncontextmenu` / `@click.right` | Interactive clickable button widget. |
| `<input>` | Interactive | `display: inline flex; flex-direction: row; border: solid; border-color: #555; padding-left: 1; padding-right: 1; overflow-x: scroll; scrollbar-width: none; white-space: nowrap;` | `value` | Interactive single-line text entry field. |
| `<textarea>` | Interactive | `display: block; border: solid; border-color: #555; padding-left: 1; padding-right: 1; overflow-y: scroll;` | `value` | Interactive multi-line scrollable text field. |
| `<checkbox>` | Interactive | `display: inline-block; cursor: pointer;` | `checked`, `onchange` | Interactive boolean check toggle. |
| `<slider>` | Interactive | `display: inline-block; cursor: pointer;` | `value`, `min`, `max`, `step`, `width`, `onchange` | Interactive range slider control. |
| `<progress>` | Display | `display: inline-block;` | `value`, `max`, `width` | Non-interactive progress bar tracker. |
| `<select>` | Interactive | `display: inline flex; flex-direction: column;` | `value`, `onchange` | Dropdown picker list menu. |
| `<option>` | Interactive | `display: block;` | `value` | Pickable choice element inside `<select>`. |
| `<hr>` | Display | `display: block; margin-top: 1; margin-bottom: 1; overflow: hidden; white-space: nowrap;` | None | Horizontal rule/divider line. |
| `<if>` | Control Flow | N/A | `condition` | Dynamic conditional branch renderer. |
| `<elif>` | Control Flow | N/A | `condition` | Alternative branch. Must follow `<if>` or `<elif>`. |
| `<else>` | Control Flow | N/A | None | Fallback branch. Must follow `<if>` or `<elif>`. |

---

## 2. Global Attributes

These attributes are supported on all elements:

| Attribute | Value Type | Description |
| :--- | :--- | :--- |
| `id` | `string` | Unique identifier (used for `#id` selectors in CSS and queries). |
| `class` | `string` | Space-separated class list (used for `.class` styling selectors). |
| `if` / `:if` | `expression` | Renders the element only if the condition evaluates to `true`. |
| `for` | `expression` | Loops the element over a collection (e.g. `item in items`). |
| `tabindex` | `integer` | Focus traversal order (`>= 0` for sequential tab, `-1` to skip). |
| `focusable` | `boolean` | Shorthand (`focusable="true"` is equivalent to `tabindex="0"`). |

---

## 3. Data & Event Bindings Syntax

| Syntax Style | Target | Example | Description |
| :--- | :--- | :--- | :--- |
| **React-style** | Attribute Binding | `value="{score}"` | Binds attribute to variable interpolation. |
| **Vue-style** | Attribute Binding | `:value="score"` | Shorthand for value interpolation binding. |
| **React-style** | Event Binding | `onclick="MyCallback"` | Binds trigger event to C++ callback. |
| **Vue-style** | Event Binding | `@click="MyCallback"` | Shorthand for event callback binding. |
| **Vue Modifier** | Event Binding | `@click.right="Menu"` | Binds modifier (e.g., mouse right-click). |
