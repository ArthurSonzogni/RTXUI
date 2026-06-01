#include <rtxui/rtxui.hpp>
#include <string>
#include <vector>

using namespace rtxui;

struct Item {
  std::string name;
  std::string color;
  bool operator==(const Item& other) const = default;
};

class LoopApp : public Component<LoopApp> {
 public:
  std::vector<std::string> simple_items = {"Apple", "Banana", "Cherry"};
  std::vector<Item> complex_items = {{"Red", "rgb(255, 0, 0)"},
                                     {"Green", "rgb(0, 255, 0)"},
                                     {"Blue", "rgb(0, 0, 255)"}};

  std::string_view view = R"html(
      <div class="container">
        <h1>Simple Loop</h1>
        <button onclick="AddFruit">Add Fruit</button>
        <ul>
          <for each="{simple_items}" as="fruit">
            <li>{fruit}</li>
          </for>
        </ul>

        <h1>Complex Loop</h1>
        <button onclick="AddColor">Add Color</button>
        <div class="palette">
          <for each="{complex_items}" as="color_item">
            <div class="color-box" style="background-color: {color_item.color}">
              {color_item.name}
            </div>
          </for>
        </div>
      </div>

      <style>
        self {
          display: block;
          padding: 1;
        }
        .container {
          display: flex;
          flex-direction: column;
          gap: 1;
        }
        .palette {
          display: flex;
          gap: 1;
        }
        .color-box {
          padding: 1;
          border: tall;
          min-width: 10;
          text-align: center;
        }
      </style>
    )html";

  LoopApp() {
    BindCollection("simple_items", &simple_items);
    BindCollection("complex_items", &complex_items, [](const Item& item) {
      return std::make_shared<ManualStructVisitor>(
          std::unordered_map<std::string, std::string>{{"name", item.name},
                                                       {"color", item.color}});
    });

    Import("AddFruit", [this]() {
      simple_items.push_back("New Fruit " +
                             std::to_string(simple_items.size() + 1));
    });

    Import("AddColor", [this]() {
      complex_items.push_back(
          {"New Color " + std::to_string(complex_items.size() + 1),
           "rgb(128, 128, 128)"});
    });
  }
};

int main() {
  auto app = Ref<LoopApp>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
