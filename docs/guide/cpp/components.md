# Creating Components

In RTXUI, components are the primary building blocks of the user interface. Every component encapsulates its markup template, reactive state, and scoped CSS styles.

---

## Component Definition (`Component<Derived>`)

Every custom component inherits from `rtxui::Component<Derived>` via the Curiously Recurring Template Pattern (CRTP). Declare the template using the `view` member string:

```cpp
#include <rtxui/rtxui.hpp>

class MyCard : public rtxui::Component<MyCard> {
 public:
  std::string_view view = R"html(
    <div class="card">
      <h2>Card Title</h2>
      <span>Card content goes here.</span>
    </div>

    <style>
      .card {
        border: solid;
        padding: 1;
      }
      h2 {
        color: rgb(88, 166, 255);
        margin-bottom: 1;
      }
    </style>
  )html";
};
```

Components can also compute or load their template dynamically by overriding `std::string_view Setup() override`. If `view` is defined on the class, it is used by default.

---

## Importing Child Components (`Import<T>`)

To instantiate a custom C++ component inside another component's template, import it in the parent constructor using `Import<T>()`:

```cpp
class Header : public rtxui::Component<Header> {
 public:
  std::string_view view = R"html(
    <header>
      <h1>Application Dashboard</h1>
    </header>
  )html";
};

class Dashboard : public rtxui::Component<Dashboard> {
 public:
  Dashboard() {
    Import<Header>();
    Import<MyCard>();
  }

  std::string_view view = R"html(
    <div>
      <Header />
      <MyCard />
    </div>
  )html";
};
```

By default, the tag name matches the C++ class name (e.g. `<Header />`).

### Custom Tag Aliases

To use a different XML tag name in the template, pass an alias string to `Import<T>`:

```cpp
Dashboard() {
  Import<MyCard>("card");
}
```

The template can now use `<card />`:

```html
<div>
  <card />
</div>
```

---

## State & Data Binding in Components

Child components can declare internal reactive state. Bind members with `Bind(member)` to enable template interpolation and two-way synchronization:

```cpp
class Counter : public rtxui::Component<Counter> {
 public:
  int count = 0;

  void Increment() {
    count++;
  }

  Counter() {
    Bind(count);
    Bind("Increment", [this] { Increment(); });
  }

  std::string_view view = R"html(
    <div>
      <span>Count: {count}</span>
      <button @click="Increment">+1</button>
    </div>
  )html";
};
```

When compiled with C++26 static reflection (`RTXUI_HAS_REFLECTION`), struct members and method callbacks are bound automatically without explicit `Bind()` calls.

---

## Content Projection with Slots

Custom components can accept projected child markup from caller templates using `<slot>` containers:

```cpp
class Panel : public rtxui::Component<Panel> {
 public:
  std::string_view view = R"html(
    <div class="panel">
      <div class="header">
        <slot.title select="h3">Default Title</slot.title>
      </div>
      <div class="body">
        <slot>Default body content.</slot>
      </div>
    </div>
  )html";
};
```

Callers project content into the default or named slots directly:

```html
<Panel>
  <h3>System Status</h3>
  <span>All services operational.</span>
</Panel>
```

For complete details on slot matching and named slots, see the [Slots Guide](/guide/cpp/slots).

---

## Next Steps

- **[State Bindings & Reflection](/guide/cpp/bindings)**: Two-way data binding, member types, and reflection.
- **[Component Lifecycle](/guide/cpp/lifecycle)**: Mounting, rendering passes, and event loops.
- **[Navigating the DOM](/guide/cpp/dom)**: Inspecting elements and querying components dynamically.

