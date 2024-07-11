#include "src/app.hpp"

App app;

// app.Register("SubComponent", ...);

// Define the <e> e into the App.
app.RegisterComponent("MyComponent", [](Element& e) {
  // Input parameters to the component.
  // The parent binds a reactive value, the child receives a deep copy.
  // Example: <MyComponent input_1="42" input_2="test"/>
  auto x1 = e.Attribute("input_1");
  e.Attribute("input_2");

  // Output parameters to the component. They are bound to functions.
  // Example: <MyComponent click="handleClick"/>
  e.Event("click");

  // Input/Output parameters to the component.
  // They acts as input/output parameters. This is a short hand for declaring
  // both a
  // e model. They acts as input/output parameters. They are short hand
  // for defining an attribute and an event assigning the value to the
  // attribute.
  //
  e.Model("model");

  // e internal state. It can be modified by the e itself,
  // passed as attribute, and displayed inside the DOM.
  e["state"] = true;

  // Function to handle the button click event.
  e.Function("toggle", [&](reactive::Reactive& that) {
    // Assigning a new value to a reactive state will re-render the template.
    state["enabled"] = !state["enabled"];
  });

  e.Computed("computed", [](reactive::Reactive& that) {
    // This function will be called whenever the `input_1` attribute or `state`
    // internal state changes. The returned value will be assigned to
    // `computed`.
    if (that["state"]) {
      return 42 - that["input_1"]
    }
    return 42 + that["input_1"]
  });

  e.Template(R"(
    <!-- This demonstrates the usage of interpolation -->
    <paragraph>
      Hello, World!
      input = {{input}}
      state = {{state}}
    </paragraph>

    <!-- This demonstrates event handling -->
    <button click.left="onClick">
      Click me!
    </button>

    <output if="state">
      This is a conditional text!
    </output>

    <SubComponent></SubComponent>

    <SubComponent prop="value"></SubComponent>

    <SubComponent>
      <template name="namedslot_1">Hello from slot 1</template>
      <template name="namedslot_2">Hello from slot 2</template>
    </SubComponent>

    <!-- This demonstrates embedding a sub-e from the parent e -->
    <slot></slot>
  )");

  // Style is scoped to the current component. It is not global or inherited.
  e.Style(R"(
    paragraph {
      display-outside: block;
      display-inside: flow;
    }

    .title {
      foreground-color: white;
      background-color: blue;
    }

    button {
      foreground-color: white;
      background-color: blue;
      transition: foreground-color 0.5s ease-in-out;
    }

    button:hover {
      foreground-color: black;
      background-color: white;
    }
  )");

  e.OnMounted([](reactive::Reactive state) {
    //
  });
});

class Component {
 public:
  virtual std::string Setup() {}
};

#define RTXUI(name)                         \
  struct RTXUI_##name : public Component {  \
    std::string Setup() final;              \
  };                                        \
  namespace {                               \
  rtxui::Register<RTXUI_##name> reg(#name); \
  }                                         \
  std::string RTXUI_##name::Setup()

#define Expose(x) Expose(#x, x)

RTXUI(MyComponent) {
  // This state is reflecting the `<MyComponent input="world"/>` attribute by
  // the parent.
  auto input = State("world");

  // This demonstrates the usage of computed values. `hello_input` will be
  // updated whenever `input` is updated.
  auto hello_input =
      Computed<std::string>([] { return "Hello, " + input.Value(); });

  auto counter_1 = State(0);
  auto counter_2 = State(0);

  // Computed values can depend on multiple reactive values.
  auto counter_3 =
      Computed<int>([&] { return counter_1.Value() + counter_2.Value(); });

  // This demonstrates the usage of event handling.
  auto onClick = [=] {
    // Test
    counter_1.value(counter_1.Value() + 1);
  };

  // Make values available to the template.
  BIND(input);
  BIND(hello_input);
  BIND(counter_1);
  BIND(onClick);

  return R"(
    <MyComponent input onclick>
      <style scoped>
        p {
          color: red;
        }
      </style>

      <p>
        {hello_input}
      </p>

      <button click={onClick}>
        Click me!
      </button>

      <p if={counter_1 > 0}>
        You clicked {counter_1} times.
      </p>

    </MyComponent> 
  )";
};
