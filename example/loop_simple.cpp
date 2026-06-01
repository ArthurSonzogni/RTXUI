#include <rtxui/rtxui.hpp>
#include <string>
#include <vector>

using namespace rtxui;

class SimpleLoopApp : public Component<SimpleLoopApp> {
 public:
  std::vector<std::string> items = {"Apple", "Banana", "Cherry"};
  std::string new_fruit = "";

  void AddItem() {
    if (!new_fruit.empty()) {
      items.push_back(new_fruit);
      new_fruit = "";
    }
  }
  void RemoveItem(std::string index_str) {
    size_t index = std::stoull(index_str);
    if (index < items.size()) {
      items.erase(items.begin() + index);
    }
  }

  std::string_view view = R"html(
      <div class="container">
        <div class="input-row">
          <input value="{new_fruit}" placeholder="Enter fruit name..." />
          <button onclick="AddItem">Add Fruit</button>
        </div>
        <ul>
          <for each="{items}" as="fruit">
            <li class="item-row">
              <span class="fruit-name">{fruit}</span>
              <button onclick="RemoveItem({$index})">Remove</button>
            </li>
          </for>
        </ul>
      </div>
      <style>
        .container { padding: 1; }
        .input-row { display: flex; gap: 1; margin-bottom: 1; }
        input { border: solid; width: 20; padding: 0 1; }
        .item-row { display: flex; gap: 2; align-items: center; }
        .fruit-name { min-width: 15; }
        .item-row button { border: solid; border-color: red; color: red; padding: 0 1; }
      </style>
    )html";

  SimpleLoopApp() {
    Bind(items);
    Bind(new_fruit);
    Bind(AddItem);
    Bind(RemoveItem);
  }
};

int main() {
  auto app = Ref<SimpleLoopApp>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
