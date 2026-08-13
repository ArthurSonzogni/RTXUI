# State Bindings & Reflection

To make C++ member variables accessible inside your HTML templates, you must register them in the reflection system.

## Component Constructor & Binding

Register members inside your component class constructor using `Bind()`.

### Binding Primitive Values
Use `Bind()` to link scalar types (such as `std::string`, `int`, `double`, `bool`) to the template environment:

```cpp
struct Profile : public rtxui::Component<Profile> {
  std::string username = "Alice";
  int level = 5;

  Profile() {
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
For rendering lists dynamically (using the template `<for>` or `for` attribute), use `BindCollection()` or `Bind()`:

```cpp
struct TodoList : public rtxui::Component<TodoList> {
  std::vector<std::string> items = {"Task 1", "Task 2"};

  TodoList() {
    BindCollection("items", &items);
  }

  std::string_view Setup() override {
    return R"html(
      <ul>
        <for each="{items}" as="todo">
          <li>{todo}</li>
        </for>
      </ul>
    )html";
  }
};
```
The first parameter of `BindCollection` is the name the template's
`each="{...}"` refers to. `Bind(items)` is equivalent when the template name
should match the member name. Collections of structs additionally take a
mapper — see [Loops & Lists](/guide/loops#collections-of-structs).
