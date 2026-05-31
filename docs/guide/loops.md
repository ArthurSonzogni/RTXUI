# Loops

RTXUI supports iterating over collections using the `<for>` tag. You can bind `std::vector` or any range-compatible container.

## Simple Collection

For simple types like `std::string` or `int`, RTXUI automatically converts the item to text. You can use the built-in `$index` variable to get the current iteration index.

```cpp
class SimpleLoopApp : public Component<SimpleLoopApp> {
 public:
  std::vector<std::string> items = {"Apple", "Banana", "Cherry"};

  SimpleLoopApp() {
    BindCollection("items", &items);
  }

  std::string_view Setup() override {
    return R"html(
      <ul>
        <for each="{items}" as="fruit">
          <li>{fruit} (Index: {$index})</li>
        </for>
      </ul>
    )html";
  }
};
```

## Complex Collection with Field Mapping

For complex objects, you can provide a mapper function to expose fields to the template.

```cpp
struct Task {
  std::string name;
  bool completed;
};

class ComplexLoopApp : public Component<ComplexLoopApp> {
 public:
  std::vector<Task> tasks = {{"Build", true}, {"Test", false}};

  ComplexLoopApp() {
    BindCollection("tasks", &tasks, [](const Task& t) {
      return std::make_shared<ManualStructVisitor>(std::unordered_map<std::string, std::string>{
        {"name", t.name},
        {"status", t.completed ? "✅ Done" : "⏳ Pending"}
      });
    });
  }

  std::string_view Setup() override {
    return R"html(
      <for each="{tasks}" as="t">
        <div>{t.status} - {t.name}</div>
      </for>
    )html";
  }
};
```

<ExampleTabs src="/wasm/rtxui_example_loop_complex.js">
<template #source>

<<< @/../example/loop_complex.cpp

</template>
</ExampleTabs>
