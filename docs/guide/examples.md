# Examples Index

Every example in the repository's
[example/](https://github.com/ArthurSonzogni/RTXUI/tree/main/example)
directory, grouped by topic. Each builds as `build/rtxui_example_<name>`.

## Core Concepts

Components, reactivity, interpolation, conditional rendering, and slots.

| Example | Demonstrates | Guide |
| :--- | :--- | :--- |
| [helloworld.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/helloworld.cpp) | Minimal application rendering a hello message. | [Hello World](/guide/hello-world) |
| [counter.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/counter.cpp) | Basic state reactivity and interpolation. | [Interpolation](/guide/interpolation) |
| [conditional.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/conditional.cpp) | Conditional rendering from boolean state. | [Conditionals](/guide/conditionals) |
| [loop_simple.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/loop_simple.cpp) | Rendering a list of strings. | [Loops & Lists](/guide/loops) |
| [loop.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/loop.cpp) | Adding and removing list items interactively. | [Loops & Lists](/guide/loops) |
| [loop_complex.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/loop_complex.cpp) | Collections of structs with per-field bindings. | [Loops & Lists](/guide/loops) |
| [slots.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/slots.cpp) | Reusable components with named slots. | [Slots](/guide/cpp/slots) |
| [demo.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/demo.cpp) | A dashboard combining many widgets and tabs. | [Reactivity](/reactivity) |

## Form Elements

User input through keyboard and mouse, bound to C++ variables.

| Example | Demonstrates | Guide |
| :--- | :--- | :--- |
| [input.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/input.cpp) | Single-line text input. | [Forms](/guide/forms) |
| [textarea.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/textarea.cpp) | Multi-line text editing. | [Forms](/guide/forms) |
| [checkbox.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/checkbox.cpp) | Checkboxes bound to booleans. | [Forms](/guide/forms) |
| [radio.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/radio.cpp) | Radio button groups. | [Forms](/guide/forms) |
| [label.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/label.cpp) | Labels delegating focus to their control. | [Forms](/guide/forms) |
| [slider.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/slider.cpp) | Numeric range slider. | [Forms](/guide/forms) |
| [progress.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/progress.cpp) | Progress bar tracking state. | [Forms](/guide/forms) |
| [select.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/select.cpp) | Dropdown with `<option>` children. | [Forms](/guide/forms) |
| [fieldset.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/fieldset.cpp) | Grouping controls with `<fieldset>`/`<legend>`. | [HTML Reference](/html_reference) |

## Layout & Box Model

Positioning terminal cells with flexbox, grid, borders, and margins.

| Example | Demonstrates | Guide |
| :--- | :--- | :--- |
| [layout.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/layout.cpp) | Flexbox spacing, justification, and alignment. | [Flexbox](/guide/css/flexbox) |
| [layout_flex.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/layout_flex.cpp) | Flex grow/shrink/basis distribution. | [Flexbox](/guide/css/flexbox) |
| [grid.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/grid.cpp) | Grid tracks, spans, and cell alignment. | [Grid](/guide/css/grid) |
| [borders.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/borders.cpp) | The border styles (`solid`, `double`, `dashed`, …). | [Box Model](/guide/css/box-model) |
| [border_scroll_demo.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/border_scroll_demo.cpp) | Borders combined with scrollbars. | [Box Model](/guide/css/box-model) |
| [positioning.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/positioning.cpp) | Absolute and relative positioning, stacking. | [Positioning](/guide/css/positioning) |
| [sticky.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/sticky.cpp) | Elements pinned to viewport edges while scrolling. | [Positioning](/guide/css/positioning) |

## Scrolling & Overflow

| Example | Demonstrates | Guide |
| :--- | :--- | :--- |
| [focus_scroll.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/focus_scroll.cpp) | Keeping the focused element visible in a scroll area. | [Scrolling](/guide/scrolling) |
| [horizontal_scroll.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/horizontal_scroll.cpp) | Horizontal scrollbars and mouse scrolling. | [Scrolling](/guide/scrolling) |
| [nested_scroll.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/nested_scroll.cpp) | Scroll events propagating through nested containers. | [Scrolling](/guide/scrolling) |
| [scroll_behavior.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/scroll_behavior.cpp) | `scroll-behavior: smooth` versus `auto`. | [Scrolling](/guide/scrolling) |
| [anchor.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/anchor.cpp) | Hash-anchor links scrolling their target into view. | [Scrolling](/guide/scrolling) |

## Typography & Styling

| Example | Demonstrates | Guide |
| :--- | :--- | :--- |
| [colors.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/colors.cpp) | Foreground/background colors, hex and RGB values. | [Typography](/guide/typography) |
| [opacity.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/opacity.cpp) | Alpha blending and nested opacity. | [Typography](/guide/typography) |
| [text_align.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/text_align.cpp) | Left, right, and center text alignment. | [Typography](/guide/typography) |
| [text_decoration.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/text_decoration.cpp) | Underline, strikethrough, italic, bold, dim. | [Typography](/guide/typography) |
| [pseudo_classes.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/pseudo_classes.cpp) | `:hover`, `:active`, and `:focus` rules. | [Transitions](/guide/css/animations) |
| [transitions.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/transitions.cpp) | Animated transitions of colors and dimensions. | [Transitions](/guide/css/animations) |
| [animation.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/animation.cpp) | Cyclic keyframe animations. | [Transitions](/guide/css/animations) |

## Components & Advanced Features

| Example | Demonstrates | Guide |
| :--- | :--- | :--- |
| [tabs.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/tabs.cpp) | Tabbed panes with `<tabs>`. | [HTML Reference](/html_reference) |
| [dialog.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/dialog.cpp) | Modal `<dialog>` opening and closing. | [HTML Reference](/html_reference) |
| [details.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/details.cpp) | Collapsible `<details>`/`<summary>` sections. | [HTML Reference](/html_reference) |
| [lists.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/lists.cpp) | Ordered and unordered lists with markers. | [HTML Reference](/html_reference) |
| [tooltip.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/tooltip.cpp) | Tooltips shown on hover, in four directions. | [HTML Reference](/html_reference) |
| [hr.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/hr.cpp) | Horizontal rules and separators. | [HTML Reference](/html_reference) |
| [table.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/table.cpp) | HTML table elements. | [HTML Reference](/html_reference) |
| [cjk.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/cjk.cpp) | Double-width CJK ideographs and mixed-width text. | [Unicode & CJK](/guide/unicode) |
| [markdown.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/markdown.cpp) | Rendering Markdown files with live styling. | [Markdown](/guide/markdown) |
| [media.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/media.cpp) | `@media` rules reacting to terminal resizes. | [Media Queries](/guide/css/media-queries) |
| [spatial_navigation.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/spatial_navigation.cpp) | Arrow-key focus movement across a 2D grid. | [Focus](/guide/html/focus) |
| [tabindex.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/tabindex.cpp) | Custom focus order with `tabindex`. | [Focus](/guide/html/focus) |
| [cookbook_tabs.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/cookbook_tabs.cpp) | Hand-built tabbed navigation. | [Cookbook](/guide/cookbook) |
| [cookbook_async.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/cookbook_async.cpp) | Worker thread posting results to the UI loop. | [Cookbook](/guide/cookbook) |
| [cookbook_dialog.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/cookbook_dialog.cpp) | Confirmation flow with `<dialog>`. | [Cookbook](/guide/cookbook) |
