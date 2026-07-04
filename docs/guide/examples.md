# All Examples & Demos

This page provides a comprehensive index of all 39 example files available in the RTXUI repository under the [example/](https://github.com/ArthurSonzogni/RTXUI/blob/main/example) directory. Each example is designed to showcase specific reactive terminal rendering features, interactive widgets, layout behaviors, or CSS styling capabilities.

---

## Getting Started & Core Concepts

These examples demonstrate the fundamental building blocks of RTXUI, including components, reactivity, interpolation, conditional rendering, and slots.

| Example File | Component Class | Description | Guide / Reference |
| :--- | :--- | :--- | :--- |
| [helloworld.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/helloworld.cpp) | `HelloWorldApp` | Minimal RTXUI application rendering a hello message. | [Hello World Guide](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/guide/hello-world.md) |
| [counter.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/counter.cpp) | `Counter` | Interactive counter showing basic state reactivity and interpolation. | [Value Interpolation](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/guide/interpolation.md) |
| [conditional.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/conditional.cpp) | `ConditionalApp` | Conditional element rendering based on reactive boolean flags. | [Conditional Rendering](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/guide/conditionals.md) |
| [loop_simple.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/loop_simple.cpp) | `SimpleLoopApp` | Rendering basic lists of strings reactively. | [Loops & Lists Guide](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/guide/loops.md) |
| [loop.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/loop.cpp) | `LoopApp` | Interactive additions and removals to lists with reactive UI updates. | [Loops & Lists Guide](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/guide/loops.md) |
| [loop_complex.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/loop_complex.cpp) | `ComplexLoopApp` | Rendering complex nested structures and tracking list items. | [Loops & Lists Guide](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/guide/loops.md) |
| [slots.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/slots.cpp) | `Card` | Creating reusable components with named slots and default composition. | [Component Slots](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/guide/cpp/slots.md) |
| [demo.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/demo.cpp) | `Header` | The flagship dashboard showcase showing many widgets and interactive UI tabs. | [Reactive Model Guide](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/reactivity.md) |

---

## Form Elements & Interactive Inputs

These examples show how to gather user inputs using keyboard and mouse events bound to reactive C++ variables.

| Example File | Component Class | Description | Guide / Reference |
| :--- | :--- | :--- | :--- |
| [input.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/input.cpp) | `InputDemo` | Single-line interactive text input with cursor management. | [Forms Guide](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/guide/forms.md) / [Event Handlers](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/guide/bindings.md) |
| [textarea.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/textarea.cpp) | `TextareaDemo` | Multi-line text editor supporting keyboard editing and navigation. | [Form Elements Guide](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/guide/forms.md) |
| [checkbox.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/checkbox.cpp) | `CheckboxDemo` | Interactive checkboxes bound to boolean variables. | [Form Elements Guide](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/guide/forms.md) |
| [label.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/label.cpp) | `LabelDemo` | Clicking on a label delegates focus and actions to its associated input/checkbox. | [HTML Elements Reference](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/html_reference.md) |
| [slider.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/slider.cpp) | `SliderDemo` | Horizontal slider control for choosing numeric range values. | [Form Elements Guide](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/guide/forms.md) |
| [progress.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/progress.cpp) | `ProgressDemo` | Visual progress bar element adjusting to state changes. | [Form Elements Guide](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/guide/forms.md) |
| [select.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/select.cpp) | `SelectDemo` | Dropdown element with optional menu select items. | [Form Elements Guide](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/guide/forms.md) |

---

## Layout & Box Model

These examples demonstrate how terminal character cells are positioned using flexbox layouts, borders, and margins.

| Example File | Component Class | Description | Guide / Reference |
| :--- | :--- | :--- | :--- |
| [layout.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/layout.cpp) | `Box` | CSS Flexbox spacing, justification, alignment, and flex properties. | [Flexbox Layouts Guide](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/guide/css/flexbox.md) |
| [borders.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/borders.cpp) | `BorderBox` | Showcases various CSS border styles (`solid`, `double`, `dashed`, etc.). | [Box Model Guide](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/guide/css/box-model.md) |
| [border_scroll_demo.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/border_scroll_demo.cpp) | `BorderScrollDemo` | Visual, interactive borders demo showing mouse scrollbars alignment. | [Box Model Guide](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/guide/css/box-model.md) |
| [positioning.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/positioning.cpp) | `PositioningApp` | Absolute and relative layout coordinates and layered stacking. | [Positioning Guide](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/guide/css/positioning.md) |
| [sticky.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/sticky.cpp) | `StickyDemo` | Sticky components that adhere to viewport borders while scrolling. | [Positioning Guide](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/guide/css/positioning.md) |

---

## Scrolling & Overflows

RTXUI supports advanced horizontal/vertical scrolling, nested scrolling, and customizable mouse/focus behaviors.

| Example File | Component Class | Description | Guide / Reference |
| :--- | :--- | :--- | :--- |
| [focus_scroll.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/focus_scroll.cpp) | `FocusScrollDemo` | Automatically centers focused list elements within the scroll area. | [Scrolling Containers](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/guide/scrolling.md) |
| [horizontal_scroll.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/horizontal_scroll.cpp) | `HorizontalScrollDemo` | Horizontal scrollbars, custom content wrapping, and mouse horizontal scrolling. | [Scrolling Containers](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/guide/scrolling.md) |
| [nested_scroll.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/nested_scroll.cpp) | `NestedScrollDemo` | Propagating scroll events upwards through nested scroll boundaries. | [Scrolling Containers](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/guide/scrolling.md) |
| [scroll_behavior.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/scroll_behavior.cpp) | `ScrollBehaviorDemo` | Demonstrates smooth vs. auto scrolling behaviors. | [Scrolling Containers](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/guide/scrolling.md) |
| [anchor.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/anchor.cpp) | `AnchorDemo` | Hash anchor link navigation triggering scroll-into-view on target elements. | [Scrolling Containers](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/guide/scrolling.md) |

---

## Typography & CSS Styling

These examples cover colors, opacity blending, text decoration, alignment, and hover pseudo-classes.

| Example File | Component Class | Description | Guide / Reference |
| :--- | :--- | :--- | :--- |
| [colors.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/colors.cpp) | `ColorBox` | Foreground/background text coloring, hex and RGB parsed values. | [Typography Guide](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/guide/typography.md) |
| [opacity.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/opacity.cpp) | `OpacityDemo` | Alpha blending, color mixing, and nested opacity inheritance rules. | [Typography Guide](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/guide/typography.md) |
| [text_align.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/text_align.cpp) | `TextAlignDemo` | Text block layout with left, right, and center alignments. | [Typography Guide](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/guide/typography.md) |
| [text_decoration.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/text_decoration.cpp) | `TextDecorationDemo` | Styles such as `underline`, `strikethrough`, `italic`, `bold`, and `dim`. | [Typography Guide](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/guide/typography.md) |
| [pseudo_classes.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/pseudo_classes.cpp) | `PseudoClassesDemo` | Interactive styling rules triggered on `:hover`, `:active`, and `:focus`. | [Transitions & Animations](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/guide/css/animations.md) |
| [transitions.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/transitions.cpp) | `TransitionsDemo` | Smooth animated transitions for color, position, and dimensions. | [Transitions & Animations](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/guide/css/animations.md) |
| [animation.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/animation.cpp) | `AnimationDemo` | Keyframe-based cyclic css animations running in the terminal. | [Transitions & Animations](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/guide/css/animations.md) |

---

## Special & Advanced Features

Advanced topics, layout components, CJK rendering, and media responsive styling.

| Example File | Component Class | Description | Guide / Reference |
| :--- | :--- | :--- | :--- |
| [lists.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/lists.cpp) | `ListsDemo` | Simple and ordered list widgets rendering formatting helper markers. | [HTML Elements Reference](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/html_reference.md) |
| [tooltip.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/tooltip.cpp) | `TooltipDemo` | Hovering over buttons displays dynamic tooltips in different directions. | [HTML Element Reference](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/html_reference.md) |
| [hr.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/hr.cpp) | `HrDemo` | Separators and horizontal rule custom styled components. | [HTML Elements Reference](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/html_reference.md) |
| [table.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/table.cpp) | `TableDemo` | Layout rendering using standard HTML table elements. | [HTML Elements Reference](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/html_reference.md) |
| [cjk.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/cjk.cpp) | `CJKDemo` | Rendering multi-byte Unicode and double-width CJK ideographs correctly. | [Unicode & CJK Guide](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/guide/unicode.md) |
| [markdown.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/markdown.cpp) | `MarkdownDemo` | Reading external markdown files and rendering them dynamically. | [Markdown Guide](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/guide/markdown.md) |
| [media.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/media.cpp) | `MediaQueriesDemo` | Modifying layouts dynamically on terminal resize events via `@media`. | [Media Queries Guide](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/guide/css/media-queries.md) |
| [spatial_navigation.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/spatial_navigation.cpp) | `SpatialNavDemo` | 2D navigation (Up/Down/Left/Right arrow keys) for grid element focus. | [Focus & Tab Navigation](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/guide/html/focus.md) |
| [tabindex.cpp](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/tabindex.cpp) | `TabIndexDemo` | Custom keyboard focus ordering using standard `tabindex` properties. | [Focus & Tab Navigation](https://github.com/ArthurSonzogni/RTXUI/blob/main/docs/guide/html/focus.md) |
