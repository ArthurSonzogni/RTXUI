# Examples Index

Every example in the repository's [example/](https://github.com/ArthurSonzogni/RTXUI/tree/main/example) directory, running live in WebAssembly and accompanied by its C++ source code.

## Applications

Complete applications, showing how individual features compose into full programs.

| Example | Demonstrates | Guide |
| :--- | :--- | :--- |
| [app_dashboard.cpp](/guide/examples/app_dashboard) | A complete application, rather than a demo of one property. Everything here has appeared on its own elsewhere in example/ -- reactive state, computed values, &amp;lt;for&amp;gt; over a collection of structs, conditional rendering, flexbox, grid, transitions and custom properties. This file is about how they compose into something you would actually ship. | [Guide](/reactivity) |
| [app_filebrowser.cpp](/guide/examples/app_filebrowser) | A complete application driven by the keyboard. Where app_dashboard.cpp is about composing layout and state, this one is about the two things a terminal UI lives or dies by: scrolling a list that is longer than the viewport, and moving a selection through it without a mouse. Rows carry `tabindex`, so RTXUI's built-in focus navigation walks them and scrolls the focused one into view; the preview pane re-renders from computed values as the selection moves. | [Guide](/guide/scrolling) |
| [demo.cpp](/guide/examples/demo) | A kitchen-sink dashboard combining most of the library in one program. For a smaller, more readable application see app_dashboard.cpp. | [Guide](/reactivity) |
| [playground.cpp](/guide/examples/playground) | A live HTML/CSS editor and preview, side by side. The preview re-parses on every keystroke using the same template engine that powers every RTXUI app, which is also why this is the docs homepage demo. | [Guide](/guide/playground) |

## Core Concepts

Components, reactivity, interpolation, conditional rendering, and slots.

| Example | Demonstrates | Guide |
| :--- | :--- | :--- |
| [conditional.cpp](/guide/examples/conditional) | Conditional rendering. &amp;lt;if condition="{computed}"&amp;gt; includes its children only when the bound value is true; the `if` attribute does the same inline on any element. | [Guide](/guide/conditionals) |
| [counter.cpp](/guide/examples/counter) | Reactive state and `{...}` interpolation. Bind(count) registers a member as reactive state: the DOM is patched whenever it changes. Bind() on a const method registers a computed value that is re-evaluated from that state, and Bind() on a plain method registers an `onclick` handler. | [Guide](/guide/interpolation) |
| [helloworld.cpp](/guide/examples/helloworld) | The smallest possible RTXUI application. A component is a class deriving from Component&amp;lt;Derived&amp;gt; whose `view` member holds an HTML template plus a &amp;lt;style&amp;gt; block. `self` selects the component's own root element, and custom properties declared there (--bg, --accent, ...) inherit into every descendant, so a single palette styles the whole tree. | [Guide](/guide/hello-world) |
| [loop_complex.cpp](/guide/examples/loop_complex) | Looping over a collection of structs. A mapper exposes each struct's fields to the template, which reads them with dot notation on the loop variable. | [Guide](/guide/loops) |
| [loop_simple.cpp](/guide/examples/loop_simple) | Looping over a std::vector&amp;lt;std::string&amp;gt; with &amp;lt;for&amp;gt;. {$index} gives the current position, which is how a row passes its identity to a parameterized callback. | [Guide](/guide/loops) |
| [slots.cpp](/guide/examples/slots) | Content projection with &amp;lt;slot&amp;gt;. A reusable Card component places its caller's markup into named slots, which is how every built-in tag is implemented too. | [Guide](/guide/cpp/slots) |

## Form Elements

User input through keyboard and mouse, bound to C++ variables.

| Example | Demonstrates | Guide |
| :--- | :--- | :--- |
| [checkbox.cpp](/guide/examples/checkbox) | The &amp;lt;checkbox&amp;gt; component bound to a bool. Try it: click the box, or focus it and press Space. | [Guide](/guide/forms) |
| [fieldset.cpp](/guide/examples/fieldset) | &amp;lt;fieldset&amp;gt; and &amp;lt;legend&amp;gt; grouping. The legend is nested into the top border of the group it captions. | [Guide](/html_reference) |
| [input.cpp](/guide/examples/input) | The &amp;lt;input&amp;gt; component with two-way binding. Editing the field writes straight back into the bound std::string, and the interpolated value below updates on the same frame. | [Guide](/guide/forms) |
| [label.cpp](/guide/examples/label) | &amp;lt;label&amp;gt; delegating clicks and focus to the control it names | [Guide](/guide/forms) |
| [progress.cpp](/guide/examples/progress) | The &amp;lt;progress&amp;gt; component driven by bound state | [Guide](/guide/forms) |
| [radio.cpp](/guide/examples/radio) | &amp;lt;radio&amp;gt; buttons sharing a `name`, with the selection bound to a C++ string | [Guide](/guide/forms) |
| [select.cpp](/guide/examples/select) | The &amp;lt;select&amp;gt; dropdown with &amp;lt;option&amp;gt; children. Try it: focus it and press Enter or Space to open, then arrows to choose. | [Guide](/guide/forms) |
| [slider.cpp](/guide/examples/slider) | The &amp;lt;slider&amp;gt; component bound to an int. Try it: drag the thumb, or focus it and use the arrow keys. | [Guide](/guide/forms) |
| [textarea.cpp](/guide/examples/textarea) | The &amp;lt;textarea&amp;gt; component: multi-line editing with a line-number gutter | [Guide](/guide/forms) |

## Layout & Box Model

Positioning terminal cells with flexbox, grid, borders, and margins.

| Example | Demonstrates | Guide |
| :--- | :--- | :--- |
| [border_scroll_demo.cpp](/guide/examples/border_scroll_demo) | Borders and scrollbars sharing an edge. A scrollbar is laid out inside the border box, so it has to coexist with the border. Switch border styles to see each combination. | [Guide](/guide/css/box-model) |
| [borders.cpp](/guide/examples/borders) | Every border style: solid, double, dashed, round, tall, vkey and more. Each tile names the style it draws, so this doubles as a lookup table. | [Guide](/guide/css/box-model) |
| [grid.cpp](/guide/examples/grid) | CSS grid. grid-template sets the track sizes; items span tracks with grid-column and grid-row. | [Guide](/guide/css/grid) |
| [layout.cpp](/guide/examples/layout) | Flexbox basics. A flex row distributing three child components, each its own component with bound props. | [Guide](/guide/css/flexbox) |
| [layout_flex.cpp](/guide/examples/layout_flex) | An interactive flexbox playground. Every flex property is driven from the UI: change flex-direction, justify-content, align-items, and each item's grow/shrink/basis, and watch the boxes redistribute. | [Guide](/guide/css/flexbox) |
| [positioning.cpp](/guide/examples/positioning) | position: relative, absolute and fixed, plus z-index stacking | [Guide](/guide/css/positioning) |
| [sticky.cpp](/guide/examples/sticky) | position: sticky. Section headers pin to the top of the scroll container while their section is on screen. | [Guide](/guide/css/positioning) |

## Scrolling & Overflow

Handling scrollable regions, focus tracking, and overflow behavior.

| Example | Demonstrates | Guide |
| :--- | :--- | :--- |
| [anchor.cpp](/guide/examples/anchor) | Anchor navigation. An &amp;lt;a href="#id"&amp;gt; scrolls its target into view inside the nearest scroll container. | [Guide](/guide/scrolling) |
| [focus_scroll.cpp](/guide/examples/focus_scroll) | Scroll-into-view on keyboard focus. Tabbing to an element that is outside its scroll container scrolls it into view automatically. | [Guide](/guide/scrolling) |
| [horizontal_scroll.cpp](/guide/examples/horizontal_scroll) | Horizontal overflow. overflow-x on a container that is narrower than its content produces a horizontal scrollbar. | [Guide](/guide/scrolling) |
| [nested_scroll.cpp](/guide/examples/nested_scroll) | Nested scroll containers and scroll chaining. An inner container consumes wheel events until it reaches its end, then the event propagates to its parent. | [Guide](/guide/scrolling) |
| [scroll_behavior.cpp](/guide/examples/scroll_behavior) | scroll-behavior: smooth versus auto. Try it: click the jump buttons and compare how each column travels. | [Guide](/guide/scrolling) |

## Typography & Styling

Colors, text decorations, pseudo-classes, transitions, and keyframe animations.

| Example | Demonstrates | Guide |
| :--- | :--- | :--- |
| [animation.cpp](/guide/examples/animation) | Keyframe animations. `@keyframes` plus the `animation` property drive a property through a cycle without any C++ state changing between frames. | [Guide](/guide/css/animations) |
| [colors.cpp](/guide/examples/colors) | Foreground and background colors. Colors accept rgb(), #rrggbb and named forms, and are composed into a swatch grid by a small reusable child component. | [Guide](/guide/typography) |
| [opacity.cpp](/guide/examples/opacity) | Opacity and alpha blending, including how nested opacity compounds | [Guide](/guide/typography) |
| [pseudo_classes.cpp](/guide/examples/pseudo_classes) | The interactive pseudo-classes: :hover, :focus and :active. Rules can be nested inside their parent with `&`, exactly as in modern CSS, so an element's interactive states live next to its base declarations. | [Guide](/guide/css/animations) |
| [text_align.cpp](/guide/examples/text_align) | text-align: left, center and right | [Guide](/guide/typography) |
| [text_decoration.cpp](/guide/examples/text_decoration) | Text decoration: bold, dim, italic, underline and strikethrough | [Guide](/guide/typography) |
| [transitions.cpp](/guide/examples/transitions) | CSS transitions. `transition` interpolates a property between its old and new computed value whenever a rule stops or starts matching -- here, when :hover applies. Each property can carry its own duration and easing function. | [Guide](/guide/css/animations) |

## Components & Advanced Features

Built-in HTML elements, CJK text, Markdown rendering, spatial nav, and cookbook recipes.

| Example | Demonstrates | Guide |
| :--- | :--- | :--- |
| [cjk.cpp](/guide/examples/cjk) | Double-width text. CJK ideographs and emoji occupy two terminal cells. Layout measures text in cells, not code points, so alignment holds for mixed-width content. | [Guide](/guide/unicode) |
| [cookbook_async.cpp](/guide/examples/cookbook_async) | Recipe: updating the UI from a worker thread. The UI is single-threaded. A background thread must hand results back through the task runner, which applies them between frames. | [Guide](/guide/cookbook) |
| [cookbook_dialog.cpp](/guide/examples/cookbook_dialog) | Recipe: a confirmation dialog. The built-in &amp;lt;dialog&amp;gt; renders centered above the interface with a dimmed backdrop and closes on Escape. Bind its `open` attribute to a bool. | [Guide](/guide/cookbook) |
| [cookbook_tabs.cpp](/guide/examples/cookbook_tabs) | Recipe: tabs built by hand. The same result as the built-in &amp;lt;tabs&amp;gt;, assembled from plain elements and a bound string -- useful when you want full control of the markup. | [Guide](/guide/cookbook) |
| [details.cpp](/guide/examples/details) | The &amp;lt;details&amp;gt;/&amp;lt;summary&amp;gt; disclosure widget. Try it: click a summary row, or focus it and press Enter. | [Guide](/html_reference) |
| [dialog.cpp](/guide/examples/dialog) | The built-in &amp;lt;dialog&amp;gt; element. `open` is bound to a bool; the dialog renders centered over the rest of the UI with a dimmed backdrop and closes on Escape. | [Guide](/html_reference) |
| [hr.cpp](/guide/examples/hr) | The &amp;lt;hr&amp;gt; separator, and how border styles apply to it | [Guide](/html_reference) |
| [lists.cpp](/guide/examples/lists) | &amp;lt;ul&amp;gt;, &amp;lt;ol&amp;gt; and &amp;lt;li&amp;gt;, including nesting, `start`/`reversed`, per-item `value`,. and the list-style-type property. | [Guide](/html_reference) |
| [markdown.cpp](/guide/examples/markdown) | The &amp;lt;markdown&amp;gt; component: a live editor and rendered preview, side by side | [Guide](/guide/markdown) |
| [media.cpp](/guide/examples/media) | `@media` queries reacting to terminal size. Try it: resize the terminal and watch the layout change breakpoint. | [Guide](/guide/css/media-queries) |
| [spatial_navigation.cpp](/guide/examples/spatial_navigation) | Arrow-key spatial navigation. Focus moves to the nearest focusable element in the direction pressed, computed from the laid-out geometry. | [Guide](/guide/html/focus) |
| [tabindex.cpp](/guide/examples/tabindex) | Controlling focus order with tabindex | [Guide](/guide/html/focus) |
| [table.cpp](/guide/examples/table) | Table elements: &amp;lt;table&amp;gt;, &amp;lt;thead&amp;gt;, &amp;lt;tr&amp;gt;, &amp;lt;th&amp;gt;, &amp;lt;td&amp;gt;, with colspan, rowspan and a. sticky header row. | [Guide](/html_reference) |
| [tabs.cpp](/guide/examples/tabs) | The built-in &amp;lt;tabs&amp;gt;/&amp;lt;tab-pane&amp;gt; components. The active pane is selected by the `value` attribute bound to a C++ string. | [Guide](/html_reference) |
| [tooltip.cpp](/guide/examples/tooltip) | The &amp;lt;tooltip&amp;gt; component, shown on hover in each of four directions | [Guide](/html_reference) |
