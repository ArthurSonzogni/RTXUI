# Hello World

RTXUI applications consist of a main component and a `Screen` runner. This example creates a minimal component that prints a greeting.

### Minimal Application Setup

```cpp
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class HelloWorldApp : public Component<HelloWorldApp> {
 public:
  std::string_view view = R"html(
      <div class="card">
        Hello World from RTXUI!
      </div>
      <style>
        self {
          display: block;
          padding: 1;
          background-color: rgb(15, 23, 42);
        }
        .card {
          border: solid;
          border-color: rgb(59, 130, 246);
          padding: 1;
          color: rgb(241, 245, 249);
        }
      </style>
    )html";

  HelloWorldApp() {
    Import<rtxui::div>();
  }
};

int main() {
  auto app = Ref<HelloWorldApp>::New();
  Screen screen(app);
  screen.Loop(); // Starts interactive rendering
  return 0;
}
```

<ExampleTabs src="/wasm/rtxui_example_helloworld.js">
<template #source>

<<< @/../example/helloworld.cpp

</template>
</ExampleTabs>
