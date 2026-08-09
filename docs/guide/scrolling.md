# Scrolling Containers

When layouts contain lists or large blocks of content, they can overflow. RTXUI supports full horizontal and vertical scrolling with visual scrollbars.

### Enabling Scroll

By adding `overflow-y: scroll` (or `overflow-x: scroll`) and restricting the dimensions (`height` or `width`), RTXUI automatically crops overflowing elements and displays a modern, responsive scrollbar.

```cpp
class ScrollBox : public Component<ScrollBox> {
 public:
  std::string_view view = R"html(
      <div class="scroll-container">
        <div>Item 1</div>
        <div>Item 2</div>
        <div>Item 3</div>
        <!-- ... -->
      </div>
      <style>
        .scroll-container {
          display: block;
          height: 5;
          overflow-y: scroll;
          scrollbar-width: auto;
          border: wide;
          border-color: rgb(29, 78, 216);
        }
      </style>
    )html";

  ScrollBox() {
    Import<rtxui::div>();
  }
};
```

### Event Bubbling

Scroll events automatically bubble up to nested containers when boundaries are reached, allowing for natural feeling nested scroll areas.

<ExampleTabs src="/wasm/rtxui_example_nested_scroll.js">
<template #source>

<<< @/../example/nested_scroll.cpp

</template>
</ExampleTabs>

### Scroll-Into-View on Keyboard Focus

RTXUI automatically scrolls containers to keep focused elements visible when users navigate using the keyboard (`Tab` and `Shift+Tab`).

<ExampleTabs src="/wasm/rtxui_example_focus_scroll.js">
<template #source>

<<< @/../example/focus_scroll.cpp

</template>
</ExampleTabs>

### Horizontal Scrolling

<ExampleTabs src="/wasm/rtxui_example_horizontal_scroll.js">
<template #source>

<<< @/../example/horizontal_scroll.cpp

</template>
</ExampleTabs>

### Scroll Behavior

<ExampleTabs src="/wasm/rtxui_example_scroll_behavior.js">
<template #source>

<<< @/../example/scroll_behavior.cpp

</template>
</ExampleTabs>

### Anchor Navigation

RTXUI supports anchor navigation using `<a>` tags. When an `<a>` tag with a target hash (e.g. `href="#sec-intro"`) is clicked, RTXUI automatically scrolls the element with the corresponding `id` (e.g. `id="sec-intro"`) into view. Combined with `position: sticky` and `scroll-behavior: smooth`, this allows building robust sidebar navigation layouts.

<ExampleTabs src="/wasm/rtxui_example_anchor.js">
<template #source>

<<< @/../example/anchor.cpp

</template>
</ExampleTabs>


## An application using all of it

A file browser: the listing scrolls, its rows are focusable, and moving the
focus walks them while keeping the focused row on screen.

<ExampleTabs src="/wasm/rtxui_example_app_filebrowser.js" :cols="100" :rows="28">
<template #source>

<<< @/../example/app_filebrowser.cpp

</template>
</ExampleTabs>
