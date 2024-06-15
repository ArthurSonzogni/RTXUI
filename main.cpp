#include <fl/fl.h>

auto app = fl::App();

app.Register("SubComponent", ...);

// Define the <Component> component into the App.
app.Register("Component", [](fl::Component& component) {
  // Component attributes. They act as input parameters to the component.
  component.Attribute("input_1", true);
  component.Attribute("input_2", 42);

  // Component model. They act as
  component.State("state", true);

  // Function to handle the button click event.
  component.Function("toggle", [&](fl::State& state, fl::Event& event) {
    state["enabled"] = !state["enabled"];
  });

  component::Computed("computed",
                      [](fl::State&) { return 42 + state["input_1"]; });

  component.Template(R"(
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

    <!-- This demonstrates embedding a sub-component from the parent component -->
    <slot></slot>
  )");

  component.Style(R"(
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
  )")
})
