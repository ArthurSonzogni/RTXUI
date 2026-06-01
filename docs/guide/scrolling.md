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
