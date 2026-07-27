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
        self {
          display: block;
          padding: 1;
          background-color: rgb(18, 18, 18);
          color: white;
        }
        .container {
          padding: 1;
          max-width: 80;
          margin: auto;
        }

        .input-row {
            display: flex;
            gap: 1;
            margin-bottom: 1;
          }
        input {
          width: 20;
          padding: 0 1;
        }

        .item-row {
          width: 100%;
          display: flex;
          gap: 2;
          align-items: center;
          margin-top: 1;
          background-color: rgb(30,46,84);
        }
        .fruit-name {
          flex-grow: 1;
          padding: 1;
        }
        .item-row button {
          border: tall;
          border-color: rgb(239, 68, 68);
          color: rgb(239, 68, 68);
          background-color: transparent;
          padding: 0 1;
          margin: 0;
        }
        .item-row button:hover { background-color: rgb(239, 68, 68);
          color: white;
          border-color: rgb(248, 113, 113);
        }

        button {
          background-color: rgb(30, 41, 59);
          color: white;
          padding: 0 1;
          transition: all 1s;
        }

        button:hover {
          background-color: rgb(59, 130, 246);
          border-color: rgb(96, 165, 250);
        }

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
