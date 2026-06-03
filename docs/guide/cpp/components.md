# Creating Components

In RTXUI, components are the primary building blocks of the user interface. 

## Curiously Recurring Template Pattern (CRTP)

Every user-defined component inherits from `rtxui::Component<Derived>`, where `Derived` is the name of your class. This structure enables compile-time polymorphism for component templates and reflection.

### Basic Structure
To define a component, create a struct/class inheriting from `rtxui::Component` and override the `Setup()` method to return your XML/HTML template:

```cpp
#include <rtxui/component/component.hpp>

struct MyCard : public rtxui::Component<MyCard> {
  std::string_view Setup() override {
    return R"html(
      <div class="card">
        <h2>Card Title</h2>
        <span>Card content goes here.</span>
      </div>
      
      <style>
        .card {
          border: solid;
          padding: 1;
        }
      </style>
    )html";
  }
};
```
