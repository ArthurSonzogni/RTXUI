# RTXUI Tutorial: Building Reactive Terminal Apps

Welcome to RTXUI! This tutorial guides you through building a reactive terminal application from scratch.

---

## 1. Minimal Application Setup

Every RTXUI application begins with a main component and a `Screen` runner. Let's create a minimal component that prints a greeting:

```cpp
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class HelloWorldApp : public Component<HelloWorldApp> {
 public:
  std::string_view Setup() override {
    Import<rtxui::div>();
    return R"html(
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
  }
};

int main() {
  auto app = Ref<HelloWorldApp>::New();
  Screen screen(app);
  screen.Loop(); // Starts interactive rendering
  return 0;
}
```

<WasmTerminal src="/wasm/rtxui_example_helloworld.js" :cols="80" :rows="40" />

---

## 2. Adding Reactive State

RTXUI uses compile-time reflection to track class variables as reactive state. Simply define members on your component class, and reference them in curly braces `{}` inside your template:

```cpp
class CounterApp : public Component<CounterApp> {
 public:
  // State variables
  int count = 0;

  CounterApp() {
    Bind(count);
    BindComputed(double_count);
    Import("Increment", [this]() { count++; });
    Import("Decrement", [this]() { count--; });
  }

  // Computed state method
  int double_count() const { return count * 2; }

  std::string_view Setup() override {
    Import<rtxui::div>();
    Import<rtxui::span>();
    Import<rtxui::button>();
    return R"html(
      <div>
        <span>Count: {count}</span>
        <span>Double: {double_count}</span>
        <button onclick="Increment">Increment</button>
        <button onclick="Decrement">Decrement</button>
      </div>
    )html";
  }
};
```
- **Reactivity Model**: Whenever a button is clicked, an event handler modifies `count`. The screen runs a Digest cycle, detects that `count` has changed, and updates the DOM elements.

<WasmTerminal src="/wasm/rtxui_example_counter.js" :cols="80" :rows="40" />

---

## 3. Nesting and Scrolling Containers

When layouts contain lists or large blocks of content, they can overflow. RTXUI supports full horizontal and vertical scrolling with visual scrollbars.

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
        <div>Item 4</div>
        <div>Item 5</div>
        <div>Item 6</div>
        <div>Item 7</div>
        <div>Item 8</div>
        <div>Item 9</div>
        <div>Item 10</div>
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

By adding `overflow-y: scroll` and restricting `height`, RTXUI automatically crops overflowing elements and displays a modern, responsive scrollbar on the right. Scroll events bubble up nested containers when boundaries are reached.

<WasmTerminal src="/wasm/rtxui_example_nested_scroll.js" :cols="80" :rows="40" />

---

## 4. Unicode & CJK Full-Width Characters

RTXUI includes native support for Unicode grapheme cluster parsing and string layout width calculation. Wide characters (such as Chinese, Japanese, and Korean) take up exactly two terminal cell columns, aligning correctly in flexbox and grid layouts.

```cpp
class CJKApp : public Component<CJKApp> {
 public:
  std::string_view Setup() override {
    Import<rtxui::div>();
    return R"html(
      <div class="container">
        <div>Chinese: 中文 (Width = 4 cells)</div>
        <div>Japanese: 日本語 (Width = 6 cells)</div>
        <div>Korean: 한국어 (Width = 6 cells)</div>
      </div>
      <style>
        self {
          display: block;
          padding: 1;
          border: solid;
          border-color: green;
        }
      </style>
    )html";
  }
};
```

<WasmTerminal src="/wasm/rtxui_example_cjk.js" :cols="80" :rows="40" />

---

## 5. Interactive Text Input

The `<input>` element provides a fully interactive text box out-of-the-box. It features two-way data binding, full grapheme-aware navigation (including Ctrl to skip word boundaries), backspace/delete manipulation (including Ctrl to delete words), mouse click positioning, and horizontal scrolling on overflow.

```cpp
class InputApp : public Component<InputApp> {
 public:
  std::string my_text = "Hello World";

  InputApp() {
    Bind(my_text);
  }

  std::string_view Setup() override {
    Import<rtxui::div>();
    Import<rtxui::input>();
    return R"html(
      <div class="card">
        <input class="styled-input" value="{my_text}" />
        <div>Value: {my_text}</div>
      </div>
      <style>
        .styled-input {
          display: inline flex;
          width: 30;
          border: solid;
          border-color: rgb(59, 130, 246);
          padding-left: 1;
          padding-right: 1;
          margin-bottom: 1;
        }
      </style>
    )html";
  }
};
```

<WasmTerminal src="/wasm/rtxui_example_input.js" :cols="80" :rows="40" />

---

## 6. Loops with `<for>`

RTXUI supports iterating over collections using the `<for>` tag. You can bind `std::vector` or any range-compatible container.

### Simple Collection

For simple types like `std::string` or `int`, RTXUI automatically converts the item to text. You can use the built-in `$index` variable to get the current iteration index.

```cpp
class SimpleLoopApp : public Component<SimpleLoopApp> {
 public:
  std::vector<std::string> items = {"Apple", "Banana", "Cherry"};
  std::string new_fruit = "";

  SimpleLoopApp() {
    BindCollection("items", &items);
    Bind(new_fruit);
    
    Import("AddItem", [this]() {
      if (!new_fruit.empty()) {
        items.push_back(new_fruit);
        new_fruit = "";
      }
    });

    Import("RemoveItem", [this](std::string index_str) {
      size_t index = std::stoull(index_str);
      if (index < items.size()) {
        items.erase(items.begin() + index);
      }
    });
  }

  std::string_view Setup() override {
    return R"html(
      <div class="container">
        <div class="input-row">
          <input value="{new_fruit}" placeholder="Enter fruit name..." />
          <button onclick="AddItem">Add Fruit</button>
        </div>
        <ul>
          <for each="{items}" as="fruit">
            <li>
              <span>{fruit}</span>
              <button onclick="RemoveItem({$index})">Remove</button>
            </li>
          </for>
        </ul>
      </div>
    )html";
  }
};
```

<WasmTerminal src="/wasm/rtxui_example_loop_simple.js" :cols="80" :rows="40" />

### Complex Collection with Field Mapping

For complex objects, you can provide a mapper function. This example demonstrates using `$index` to remove specific items from the list.

```cpp
struct Task {
  std::string name;
  bool completed;
};

class ComplexLoopApp : public Component<ComplexLoopApp> {
 public:
  std::vector<Task> tasks = {{"Build", true}, {"Test", false}};
  std::string new_task_name = "";

  ComplexLoopApp() {
    BindCollection("tasks", &tasks, [](const Task& t) {
      return std::make_shared<ManualStructVisitor>(std::unordered_map<std::string, std::string>{
        {"name", t.name},
        {"status", t.completed ? "✅ Done" : "⏳ Pending"}
      });
    });
    Bind(new_task_name);

    Import("AddTask", [this]() {
      if (!new_task_name.empty()) {
        tasks.push_back({new_task_name, false});
        new_task_name = "";
      }
    });

    Import("RemoveTask", [this](std::string index_str) {
      size_t index = std::stoull(index_str);
      if (index < tasks.size()) {
        tasks.erase(tasks.begin() + index);
      }
    });
  }

  std::string_view Setup() override {
    return R"html(
      <div>
        <input value="{new_task_name}" placeholder="New task..." />
        <button onclick="AddTask">Add Task</button>
        <for each="{tasks}" as="t">
          <div>
            <span>{t.status} - {t.name}</span>
            <button onclick="RemoveTask({$index})">X</button>
          </div>
        </for>
      </div>
    )html";
  }
};
```

<WasmTerminal src="/wasm/rtxui_example_loop_complex.js" :cols="80" :rows="40" />

