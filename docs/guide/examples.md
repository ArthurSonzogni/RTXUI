# Examples Index

Every example in the repository's [example/](https://github.com/ArthurSonzogni/RTXUI/tree/main/example) directory, running live in WebAssembly and accompanied by its C++ source code.

## Applications

Complete applications, showing how individual features compose into full programs.

| Example | Demonstrates | Guide |
| :--- | :--- | :--- |
| [app_dashboard.cpp](/guide/examples/app_dashboard) | A complete application, rather than a demo of one property. | [Guide](/reactivity) |
| [app_filebrowser.cpp](/guide/examples/app_filebrowser) | A complete application driven by the keyboard. | [Guide](/guide/scrolling) |
| [demo.cpp](/guide/examples/demo) | A kitchen-sink dashboard combining most of the library in one program. | [Guide](/reactivity) |
| [playground.cpp](/guide/examples/playground) | A live HTML/CSS editor and preview, side by side. | [Guide](/guide/playground) |

## Core Concepts

Components, reactivity, interpolation, conditional rendering, and slots.

| Example | Demonstrates | Guide |
| :--- | :--- | :--- |
| [conditional.cpp](/guide/examples/conditional) | Conditional rendering. | [Guide](/guide/conditionals) |
| [counter.cpp](/guide/examples/counter) | Reactive state and `{...}` interpolation. | [Guide](/guide/interpolation) |
| [helloworld.cpp](/guide/examples/helloworld) | The smallest possible RTXUI application. | [Guide](/guide/hello-world) |
| [loop_complex.cpp](/guide/examples/loop_complex) | Looping over a collection of structs. | [Guide](/guide/loops) |
| [loop_simple.cpp](/guide/examples/loop_simple) | Looping over a `std::vector<std::string>` with `<for>`. | [Guide](/guide/loops) |
| [slots.cpp](/guide/examples/slots) | Content projection with `<slot>`. | [Guide](/guide/cpp/slots) |

## Form Elements

User input through keyboard and mouse, bound to C++ variables.

| Example | Demonstrates | Guide |
| :--- | :--- | :--- |
| [checkbox.cpp](/guide/examples/checkbox) | The `<checkbox>` component bound to a bool. | [Guide](/guide/forms) |
| [fieldset.cpp](/guide/examples/fieldset) | `<fieldset>` and `<legend>` grouping. | [Reference](/html_reference) |
| [input.cpp](/guide/examples/input) | The `<input>` component with two-way binding. | [Guide](/guide/forms) |
| [label.cpp](/guide/examples/label) | `<label>` delegating clicks and focus to the control it names. | [Guide](/guide/forms) |
| [progress.cpp](/guide/examples/progress) | The `<progress>` component driven by bound state. | [Guide](/guide/forms) |
| [radio.cpp](/guide/examples/radio) | `<radio>` buttons sharing a `name`, with the selection bound to a C++ string. | [Guide](/guide/forms) |
| [select.cpp](/guide/examples/select) | The `<select>` dropdown with `<option>` children. | [Guide](/guide/forms) |
| [slider.cpp](/guide/examples/slider) | The `<slider>` component bound to an int. | [Guide](/guide/forms) |
| [textarea.cpp](/guide/examples/textarea) | The `<textarea>` component: multi-line editing with a line-number gutter. | [Guide](/guide/forms) |

## Layout & Box Model

Positioning terminal cells with flexbox, grid, borders, and margins.

| Example | Demonstrates | Guide |
| :--- | :--- | :--- |
| [border_scroll_demo.cpp](/guide/examples/border_scroll_demo) | Borders and scrollbars sharing an edge. | [Guide](/guide/css/box-model) |
| [borders.cpp](/guide/examples/borders) | Every border style: solid, double, dashed, round, tall, vkey and more. | [Guide](/guide/css/box-model) |
| [grid.cpp](/guide/examples/grid) | CSS grid. | [Guide](/guide/css/grid) |
| [layout.cpp](/guide/examples/layout) | Flexbox basics. | [Guide](/guide/css/flexbox) |
| [layout_flex.cpp](/guide/examples/layout_flex) | An interactive flexbox playground. | [Guide](/guide/css/flexbox) |
| [positioning.cpp](/guide/examples/positioning) | position: relative, absolute and fixed, plus z-index stacking. | [Guide](/guide/css/positioning) |
| [sticky.cpp](/guide/examples/sticky) | position: sticky. | [Guide](/guide/css/positioning) |

## Scrolling & Overflow

Handling scrollable regions, focus tracking, and overflow behavior.

| Example | Demonstrates | Guide |
| :--- | :--- | :--- |
| [anchor.cpp](/guide/examples/anchor) | Anchor navigation. | [Guide](/guide/scrolling) |
| [focus_scroll.cpp](/guide/examples/focus_scroll) | Scroll-into-view on keyboard focus. | [Guide](/guide/scrolling) |
| [horizontal_scroll.cpp](/guide/examples/horizontal_scroll) | Horizontal overflow. | [Guide](/guide/scrolling) |
| [nested_scroll.cpp](/guide/examples/nested_scroll) | Nested scroll containers and scroll chaining. | [Guide](/guide/scrolling) |
| [scroll_behavior.cpp](/guide/examples/scroll_behavior) | scroll-behavior: smooth versus auto. | [Guide](/guide/scrolling) |

## Typography & Styling

Colors, text decorations, pseudo-classes, transitions, and keyframe animations.

| Example | Demonstrates | Guide |
| :--- | :--- | :--- |
| [animation.cpp](/guide/examples/animation) | Keyframe animations. | [Guide](/guide/css/animations) |
| [colors.cpp](/guide/examples/colors) | Foreground and background colors. | [Guide](/guide/typography) |
| [opacity.cpp](/guide/examples/opacity) | Opacity and alpha blending, including how nested opacity compounds. | [Guide](/guide/typography) |
| [pseudo_classes.cpp](/guide/examples/pseudo_classes) | The interactive pseudo-classes: :hover, :focus and :active. | [Guide](/guide/css/basics) |
| [text_align.cpp](/guide/examples/text_align) | text-align: left, center and right. | [Guide](/guide/typography) |
| [text_decoration.cpp](/guide/examples/text_decoration) | Text decoration: bold, dim, italic, underline and strikethrough. | [Guide](/guide/typography) |
| [transitions.cpp](/guide/examples/transitions) | CSS transitions. | [Guide](/guide/css/animations) |

## Components & Advanced Features

Built-in HTML elements, CJK text, Markdown rendering, spatial nav, and cookbook recipes.

| Example | Demonstrates | Guide |
| :--- | :--- | :--- |
| [cjk.cpp](/guide/examples/cjk) | Double-width text. | [Guide](/guide/unicode) |
| [cookbook_async.cpp](/guide/examples/cookbook_async) | Recipe: updating the UI from a worker thread. | [Guide](/guide/cookbook) |
| [cookbook_dialog.cpp](/guide/examples/cookbook_dialog) | Recipe: a confirmation dialog. | [Guide](/guide/cookbook) |
| [cookbook_tabs.cpp](/guide/examples/cookbook_tabs) | Recipe: tabs built by hand. | [Guide](/guide/cookbook) |
| [details.cpp](/guide/examples/details) | The `<details>`/`<summary>` disclosure widget. | [Reference](/html_reference) |
| [dialog.cpp](/guide/examples/dialog) | The built-in `<dialog>` element. | [Reference](/html_reference) |
| [hr.cpp](/guide/examples/hr) | The `<hr>` separator, and how border styles apply to it. | [Reference](/html_reference) |
| [lists.cpp](/guide/examples/lists) | `<ul>`, `<ol>` and `<li>`, including nesting, `start`/`reversed`, per-item `value`, and the list-style-type property. | [Reference](/html_reference) |
| [markdown.cpp](/guide/examples/markdown) | The `<markdown>` component: a live editor and rendered preview, side by side. | [Guide](/guide/markdown) |
| [media.cpp](/guide/examples/media) | `@media` queries reacting to terminal size. | [Guide](/guide/css/media-queries) |
| [spatial_navigation.cpp](/guide/examples/spatial_navigation) | Arrow-key spatial navigation. | [Guide](/guide/html/focus) |
| [tabindex.cpp](/guide/examples/tabindex) | Controlling focus order with tabindex. | [Guide](/guide/html/focus) |
| [table.cpp](/guide/examples/table) | Table elements: `<table>`, `<thead>`, `<tr>`, `<th>`, `<td>`, with colspan, rowspan and a sticky header row. | [Reference](/html_reference) |
| [tabs.cpp](/guide/examples/tabs) | The built-in `<tabs>`/`<tab-pane>` components. | [Reference](/html_reference) |
| [tooltip.cpp](/guide/examples/tooltip) | The `<tooltip>` component, shown on hover in each of four directions. | [Reference](/html_reference) |
