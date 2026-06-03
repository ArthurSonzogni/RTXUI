# State Bindings & Reflection

To make C++ member variables accessible inside your HTML templates, you must register them in the reflection system.

## The `InitReflection` Method

Override the `InitReflection()` method in your component. Always call the parent class implementation `ComponentBase::InitReflection()` at the beginning of the override.

### Binding Primitive Values
Use `Bind()` to link scalar types (such as `std::string`, `int`, `double`, `bool`) to the template environment:

```cpp
struct Profile : public rtxui::Component<Profile> {
  std::string username = "Alice";
  int level = 5;

  void InitReflection() override {
    ComponentBase::InitReflection();
    Bind(username);
    Bind(level);
  }

  std::string_view Setup() override {
    return R"html(
      <div>
        <span>User: {username}</span>
        <span>Level: {level}</span>
      </div>
    )html";
  }
};
```

### Binding Standard Collections
For rendering lists dynamically (using the template `<for>` or `for` attribute), use `BindCollection()`:

```cpp
struct TodoList : public rtxui::Component<TodoList> {
  std::vector<std::string> items = {"Task 1", "Task 2"};

  void InitReflection() override {
    ComponentBase::InitReflection();
    BindCollection("items", &items);
  }

  std::string_view Setup() override {
    return R"html(
      <ul>
        <li for="{todo in items}">{todo}</li>
      </ul>
    )html";
  }
};
```
Notice that the first parameter of `BindCollection` is the string name referenced in the template iterator expression.
