# All Examples & Demos

This page provides a comprehensive index of all 39 example files available in the RTXUI repository under the [example/](file:///home/arthursonzogni/programmation/real/RTXUI/example) directory. Each example is designed to showcase specific reactive terminal rendering features, interactive widgets, layout behaviors, or CSS styling capabilities.

---

## Getting Started & Core Concepts

These examples demonstrate the fundamental building blocks of RTXUI, including components, reactivity, interpolation, conditional rendering, and slots.

| Example File | Component Class | Description | Guide / Reference |
| :--- | :--- | :--- | :--- |
| [helloworld.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/helloworld.cpp) | `HelloWorldApp` | Minimal RTXUI application rendering a hello message. | [Hello World Guide](file:///home/arthursonzogni/programmation/real/RTXUI/docs/guide/hello-world.md) |
| [counter.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/counter.cpp) | `Counter` | Interactive counter showing basic state reactivity and interpolation. | [Value Interpolation](file:///home/arthursonzogni/programmation/real/RTXUI/docs/guide/interpolation.md) |
| [conditional.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/conditional.cpp) | `ConditionalApp` | Conditional element rendering based on reactive boolean flags. | [Conditional Rendering](file:///home/arthursonzogni/programmation/real/RTXUI/docs/guide/conditionals.md) |
| [loop_simple.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/loop_simple.cpp) | `SimpleLoopApp` | Rendering basic lists of strings reactively. | [Loops & Lists Guide](file:///home/arthursonzogni/programmation/real/RTXUI/docs/guide/loops.md) |
| [loop.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/loop.cpp) | `LoopApp` | Interactive additions and removals to lists with reactive UI updates. | [Loops & Lists Guide](file:///home/arthursonzogni/programmation/real/RTXUI/docs/guide/loops.md) |
| [loop_complex.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/loop_complex.cpp) | `ComplexLoopApp` | Rendering complex nested structures and tracking list items. | [Loops & Lists Guide](file:///home/arthursonzogni/programmation/real/RTXUI/docs/guide/loops.md) |
| [slots.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/slots.cpp) | `Card` | Creating reusable components with named slots and default composition. | [Component Slots](file:///home/arthursonzogni/programmation/real/RTXUI/docs/guide/cpp/slots.md) |
| [demo.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/demo.cpp) | `Header` | The flagship dashboard showcase showing many widgets and interactive UI tabs. | [Reactive Model Guide](file:///home/arthursonzogni/programmation/real/RTXUI/docs/reactivity.md) |

---

## Form Elements & Interactive Inputs

These examples show how to gather user inputs using keyboard and mouse events bound to reactive C++ variables.

| Example File | Component Class | Description | Guide / Reference |
| :--- | :--- | :--- | :--- |
| [input.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/input.cpp) | `InputDemo` | Single-line interactive text input with cursor management. | [Forms Guide](file:///home/arthursonzogni/programmation/real/RTXUI/docs/guide/forms.md) / [Event Handlers](file:///home/arthursonzogni/programmation/real/RTXUI/docs/guide/bindings.md) |
| [textarea.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/textarea.cpp) | `TextareaDemo` | Multi-line text editor supporting keyboard editing and navigation. | [Form Elements Guide](file:///home/arthursonzogni/programmation/real/RTXUI/docs/guide/forms.md) |
| [checkbox.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/checkbox.cpp) | `CheckboxDemo` | Interactive checkboxes bound to boolean variables. | [Form Elements Guide](file:///home/arthursonzogni/programmation/real/RTXUI/docs/guide/forms.md) |
| [slider.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/slider.cpp) | `SliderDemo` | Horizontal slider control for choosing numeric range values. | [Form Elements Guide](file:///home/arthursonzogni/programmation/real/RTXUI/docs/guide/forms.md) |
| [progress.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/progress.cpp) | `ProgressDemo` | Visual progress bar element adjusting to state changes. | [Form Elements Guide](file:///home/arthursonzogni/programmation/real/RTXUI/docs/guide/forms.md) |
| [select.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/select.cpp) | `SelectDemo` | Dropdown element with optional menu select items. | [Form Elements Guide](file:///home/arthursonzogni/programmation/real/RTXUI/docs/guide/forms.md) |

---

## Layout & Box Model

These examples demonstrate how terminal character cells are positioned using flexbox layouts, borders, and margins.

| Example File | Component Class | Description | Guide / Reference |
| :--- | :--- | :--- | :--- |
| [layout.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/layout.cpp) | `Box` | CSS Flexbox spacing, justification, alignment, and flex properties. | [Flexbox Layouts Guide](file:///home/arthursonzogni/programmation/real/RTXUI/docs/guide/css/flexbox.md) |
| [borders.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/borders.cpp) | `BorderBox` | Showcases various CSS border styles (`solid`, `double`, `dashed`, etc.). | [Box Model Guide](file:///home/arthursonzogni/programmation/real/RTXUI/docs/guide/css/box-model.md) |
| [border_scroll_demo.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/border_scroll_demo.cpp) | `BorderScrollDemo` | Visual, interactive borders demo showing mouse scrollbars alignment. | [Box Model Guide](file:///home/arthursonzogni/programmation/real/RTXUI/docs/guide/css/box-model.md) |
| [positioning.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/positioning.cpp) | `PositioningApp` | Absolute and relative layout coordinates and layered stacking. | [Positioning Guide](file:///home/arthursonzogni/programmation/real/RTXUI/docs/guide/css/positioning.md) |
| [sticky.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/sticky.cpp) | `StickyDemo` | Sticky components that adhere to viewport borders while scrolling. | [Positioning Guide](file:///home/arthursonzogni/programmation/real/RTXUI/docs/guide/css/positioning.md) |

---

## Scrolling & Overflows

RTXUI supports advanced horizontal/vertical scrolling, nested scrolling, and customizable mouse/focus behaviors.

| Example File | Component Class | Description | Guide / Reference |
| :--- | :--- | :--- | :--- |
| [focus_scroll.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/focus_scroll.cpp) | `FocusScrollDemo` | Automatically centers focused list elements within the scroll area. | [Scrolling Containers](file:///home/arthursonzogni/programmation/real/RTXUI/docs/guide/scrolling.md) |
| [horizontal_scroll.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/horizontal_scroll.cpp) | `HorizontalScrollDemo` | Horizontal scrollbars, custom content wrapping, and mouse horizontal scrolling. | [Scrolling Containers](file:///home/arthursonzogni/programmation/real/RTXUI/docs/guide/scrolling.md) |
| [nested_scroll.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/nested_scroll.cpp) | `NestedScrollDemo` | Propagating scroll events upwards through nested scroll boundaries. | [Scrolling Containers](file:///home/arthursonzogni/programmation/real/RTXUI/docs/guide/scrolling.md) |
| [scroll_behavior.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/scroll_behavior.cpp) | `ScrollBehaviorDemo` | Demonstrates smooth vs. auto scrolling behaviors. | [Scrolling Containers](file:///home/arthursonzogni/programmation/real/RTXUI/docs/guide/scrolling.md) |
| [anchor.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/anchor.cpp) | `AnchorDemo` | Hash anchor link navigation triggering scroll-into-view on target elements. | [Scrolling Containers](file:///home/arthursonzogni/programmation/real/RTXUI/docs/guide/scrolling.md) |

---

## Typography & CSS Styling

These examples cover colors, opacity blending, text decoration, alignment, and hover pseudo-classes.

| Example File | Component Class | Description | Guide / Reference |
| :--- | :--- | :--- | :--- |
| [colors.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/colors.cpp) | `ColorBox` | Foreground/background text coloring, hex and RGB parsed values. | [Typography Guide](file:///home/arthursonzogni/programmation/real/RTXUI/docs/guide/typography.md) |
| [opacity.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/opacity.cpp) | `OpacityDemo` | Alpha blending, color mixing, and nested opacity inheritance rules. | [Typography Guide](file:///home/arthursonzogni/programmation/real/RTXUI/docs/guide/typography.md) |
| [text_align.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/text_align.cpp) | `TextAlignDemo` | Text block layout with left, right, and center alignments. | [Typography Guide](file:///home/arthursonzogni/programmation/real/RTXUI/docs/guide/typography.md) |
| [text_decoration.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/text_decoration.cpp) | `TextDecorationDemo` | Styles such as `underline`, `strikethrough`, `italic`, `bold`, and `dim`. | [Typography Guide](file:///home/arthursonzogni/programmation/real/RTXUI/docs/guide/typography.md) |
| [pseudo_classes.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/pseudo_classes.cpp) | `PseudoClassesDemo` | Interactive styling rules triggered on `:hover`, `:active`, and `:focus`. | [Transitions & Animations](file:///home/arthursonzogni/programmation/real/RTXUI/docs/guide/css/animations.md) |
| [transitions.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/transitions.cpp) | `TransitionsDemo` | Smooth animated transitions for color, position, and dimensions. | [Transitions & Animations](file:///home/arthursonzogni/programmation/real/RTXUI/docs/guide/css/animations.md) |
| [animation.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/animation.cpp) | `AnimationDemo` | Keyframe-based cyclic css animations running in the terminal. | [Transitions & Animations](file:///home/arthursonzogni/programmation/real/RTXUI/docs/guide/css/animations.md) |

---

## Special & Advanced Features

Advanced topics, layout components, CJK rendering, and media responsive styling.

| Example File | Component Class | Description | Guide / Reference |
| :--- | :--- | :--- | :--- |
| [lists.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/lists.cpp) | `ListsDemo` | Simple and ordered list widgets rendering formatting helper markers. | [HTML Elements Reference](file:///home/arthursonzogni/programmation/real/RTXUI/docs/html_reference.md) |
| [hr.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/hr.cpp) | `HrDemo` | Separators and horizontal rule custom styled components. | [HTML Elements Reference](file:///home/arthursonzogni/programmation/real/RTXUI/docs/html_reference.md) |
| [table.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/table.cpp) | `TableDemo` | Layout rendering using standard HTML table elements. | [HTML Elements Reference](file:///home/arthursonzogni/programmation/real/RTXUI/docs/html_reference.md) |
| [cjk.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/cjk.cpp) | `CJKDemo` | Rendering multi-byte Unicode and double-width CJK ideographs correctly. | [Unicode & CJK Guide](file:///home/arthursonzogni/programmation/real/RTXUI/docs/guide/unicode.md) |
| [markdown.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/markdown.cpp) | `MarkdownDemo` | Reading external markdown files and rendering them dynamically. | [Markdown Guide](file:///home/arthursonzogni/programmation/real/RTXUI/docs/guide/markdown.md) |
| [media.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/media.cpp) | `MediaQueriesDemo` | Modifying layouts dynamically on terminal resize events via `@media`. | [Media Queries Guide](file:///home/arthursonzogni/programmation/real/RTXUI/docs/guide/css/media-queries.md) |
| [spatial_navigation.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/spatial_navigation.cpp) | `SpatialNavDemo` | 2D navigation (Up/Down/Left/Right arrow keys) for grid element focus. | [Focus & Tab Navigation](file:///home/arthursonzogni/programmation/real/RTXUI/docs/guide/html/focus.md) |
| [tabindex.cpp](file:///home/arthursonzogni/programmation/real/RTXUI/example/tabindex.cpp) | `TabIndexDemo` | Custom keyboard focus ordering using standard `tabindex` properties. | [Focus & Tab Navigation](file:///home/arthursonzogni/programmation/real/RTXUI/docs/guide/html/focus.md) |
