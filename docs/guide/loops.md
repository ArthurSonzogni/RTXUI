# Loops

RTXUI supports iterating over collections using the `<for>` tag. You can bind `std::vector` or any range-compatible container using the `Bind()` macro.

## Simple Collection

For simple types like `std::string` or `int`, RTXUI automatically converts the item to text. You can use the built-in `$index` variable to get the current iteration index.

```cpp
class SimpleLoopApp : public Component<SimpleLoopApp> {
 public:
  std::vector<std::string> items = {"Apple", "Banana", "Cherry"};

  std::string_view view = R"html(
      <ul>
        <for each="{items}" as="fruit">
          <li>{fruit} (Index: {$index})</li>
        </for>
      </ul>
    )html";

  SimpleLoopApp() {
    Bind(items);
  }
};
```

## Complex Collection with Field Mapping

For complex objects, you can provide a mapper function to expose fields to the template.


<ExampleTabs src="/wasm/rtxui_example_loop_complex.js">
<template #source>

<<< @/../example/loop_complex.cpp

</template>
</ExampleTabs>
