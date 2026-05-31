# Scrolling Containers

When layouts contain lists or large blocks of content, they can overflow. RTXUI supports full horizontal and vertical scrolling with visual scrollbars.

### Enabling Scroll

By adding `overflow-y: scroll` (or `overflow-x: scroll`) and restricting the dimensions (`height` or `width`), RTXUI automatically crops overflowing elements and displays a modern, responsive scrollbar.

```cpp
class ScrollBox : public Component<ScrollBox> {
 public:
  std::string_view Setup() override {
    Import<rtxui::div>();
    return R"html(
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
