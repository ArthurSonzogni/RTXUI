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
| `<ol>` | List | `display: block; padding-left: 3;` | None | Ordered list block. |
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
| `<table>` | Container | `display: block;` | None | Table container element. Organizes child row elements in a grid. |
| `<tr>` | Container | `display: block;` | None | Table row element. Groups cell elements. |
| `<td>` | Container | `display: block;` | None | Table data cell element. Fits slot content. |
| `<th>` | Container | `display: block;` | None | Table header cell element. Fits slot content. |
| `<details>` | Interactive | `display: block;` | `open` | Collapsible section widget. |
| `<summary>` | Interactive | `display: inline;` | None | Clickable summary header for `<details>`. |
| `<fieldset>` | Container | `display: block;` | None | Grouping wrapper with border and optional legend. |
| `<legend>` | Container | `display: inline;` | None | Group label nested inside `<fieldset>` top border. |
| `<radio>` | Interactive | `display: inline-block;` | `checked`, `name`, `onchange` | Multi-choice radio button toggle. |
| `<tabs>` | Container | `display: flex;` | `value`, `onchange` | Tabbed section switcher. |
| `<tab-pane>` | Container | `display: block;` | `label`, `name` | Individual tabbed panel item. |
| `<dialog>` | Container | `display: block;` | `open`, `title` | Floating modal overlay dialog window. |
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

---

## 4. Lists

RTXUI supports unordered lists (`<ul>`), ordered lists (`<ol>`), and list items (`<li>`).

### Unordered Lists (`<ul>`)
Unordered lists render markers to the left of each item. By default, they have a left padding of `2`.
If no custom style is specified, the marker style alternates automatically based on nesting depth:
* Nesting Level 1: Disc (`• `)
* Nesting Level 2: Circle (`○ `)
* Nesting Level 3+: Square (`■ `)

### Ordered Lists (`<ol>`)
Ordered lists render sequential numbers (`1. `, `2. `, etc.) prefixing each list item. They have a default left padding of `3` to accommodate double-digit list numbering.

### List Items (`<li>`)
List items render the marker (bullet or number) and flow their slot content.
The prefix marker is styled using the `list-style-type` (or shorthand `list-style`) CSS property, which can be configured inside `<style>` blocks to affect list item descendants.

Example:
```xml
<ul>
  <li>First item</li>
  <li>Second item</li>
</ul>

<style>
  ul {
    list-style-type: square;
  }
</style>
```

### Live Demo

Below is the interactive live demo showcasing nested unordered lists, ordered lists, and custom list style styling:

<ExampleTabs src="/wasm/rtxui_example_lists.js">
<template #source>

<<< @/../example/lists.cpp

</template>
</ExampleTabs>

---

## 5. Horizontal Rule (`<hr>`)

The `<hr>` element renders a horizontal divider line spanning the full width of its container.

### Live Demo

<ExampleTabs src="/wasm/rtxui_example_hr.js">
<template #source>

<<< @/../example/hr.cpp

</template>
</ExampleTabs>

---

## 6. Tables

RTXUI supports structured table layouts through `<table>`, `<tr>`, `<td>`, and `<th>` elements.

### Elements
* `<table>`: The top-level table container.
* `<tr>`: Defines a row of cells inside a table.
* `<td>`: Defines a data cell.
* `<th>`: Defines a header cell.

### Layout Behavior
* **Column Sizing**: Column widths are calculated automatically. Cell widths conform to the maximum preferred width of cells in their respective column. If the table is styled with a fixed width, remaining space is distributed proportionally among columns.
* **Row Sizing**: Row height is automatically set to the height of the tallest cell in that row. Shorter cells in the same row are stretched vertically to align backgrounds and borders.

### Styling
You can style tables, rows, and cells using CSS. Borders are fully supported on tables, rows, and cells (e.g. `border: solid;`).

Example:
```xml
<table>
  <tr>
    <th>Header 1</th>
    <th>Header 2</th>
  </tr>
  <tr>
    <td>Data A</td>
    <td>Data B</td>
  </tr>
</table>

<style>
  table {
    border: solid;
    border-color: #3b82f6;
  }
  th {
    font-weight: bold;
    border-bottom: solid;
    border-color: #334155;
  }
  td, th {
    padding-left: 1;
    padding-right: 1;
  }
</style>
```

### Live Demo

Below is the interactive live demo showcasing table borders, alternating row backgrounds, dynamic auto-column sizing, and scrollable container overflow behavior:

<ExampleTabs src="/wasm/rtxui_example_table.js">
<template #source>

<<< @/../example/table.cpp

</template>
</ExampleTabs>

---

## 7. Collapsible Details (`<details>`)

The `<details>` element represents a disclosure widget in which information is visible only when the widget is toggled into an "open" state. A `<summary>` element is used to provide the visible label or header for the widget.

### Elements
* `<details>`: The collapsible wrapper container.
* `<summary>`: The label or header that toggles details when clicked or activated via keyboard.

### Attributes
* `open`: A boolean attribute (`"true"` or `"false"`) indicating whether the details content is expanded.

### Behavior
* **Toggle Interaction**: Clicking the summary element or focusing it and pressing `Enter`/`Space` toggles the details visibility.
* **Default Summary**: If no `<summary>` element is provided inside `<details>`, it automatically renders a default summary header labeled `"Details"`.

Example:
```xml
<details open="{is_expanded}">
  <summary>Click to see advanced options</summary>
  <div class="options">
    <checkbox checked="{enable_logs}">Enable Logs</checkbox>
    <checkbox checked="{verbose}">Verbose Output</checkbox>
  </div>
</details>
```

---

## 8. Grouping & Layout (`<fieldset>`, `<legend>`)

The `<fieldset>` element is used to group several controls and labels within a web form. The `<legend>` element defines a caption for the `<fieldset>` and is nested inside its top border.

Example:
```xml
<fieldset>
  <legend>User Credentials</legend>
  <input value="{username}" placeholder="Username" />
  <input value="{password}" placeholder="Password" />
</fieldset>
```

---

## 9. Radio Buttons (`<radio>`)

The `<radio>` element represents a radio button, allowing a single selection among multiple options sharing the same `name` attribute value.

Example:
```xml
<radio name="gender" checked="{is_male}">Male</radio>
<radio name="gender" checked="{is_female}">Female</radio>
```

---

## 10. Tabbed Interfaces (`<tabs>`, `<tab-pane>`)

The `<tabs>` and `<tab-pane>` elements build tabbed panels that allow switching between different views.

Example:
```xml
<tabs value="{active_section}">
  <tab-pane label="Profile" name="profile">
    <p>User profile information...</p>
  </tab-pane>
  <tab-pane label="Billing" name="billing">
    <p>User billing plans...</p>
  </tab-pane>
</tabs>
```

---

## 11. Overlays & Dialogs (`<dialog>`)

The `<dialog>` element represents a dialog box or other interactive component, such as a dismissible alert or subwindow overlay. When `open="true"`, it renders centered on top of all other elements using a translucent dark backdrop.

Example:
```xml
<dialog open="{is_dialog_open}" title="Exit Application">
  <p>Are you sure you want to exit?</p>
  <button onclick="ConfirmExit">Exit</button>
</dialog>
```



