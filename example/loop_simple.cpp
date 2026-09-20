// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Looping over a std::vector<std::string> with <for>.
//
// {$index} gives the current position, which is how a row passes its identity
// to a parameterized callback.
//
// Try it: add a fruit, then remove one.
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
          --danger: rgb(248, 81, 73);

          display: block;
          padding: 1;
          background-color: rgb(13, 17, 23);
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
          border: tall;
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
          border-color: var(--danger);
          color: var(--danger);
          background-color: transparent;
          padding: 0 1;
          margin: 0;
        }
        .item-row button:hover { background-color: var(--danger);
          color: white;
          border-color: var(--danger);
        }

        button {
          background-color: rgb(22, 27, 34);
          color: white;
          transition: all 1s;
        }

        button:hover {
          background-color: rgb(88, 166, 255);
          border-color: rgb(121, 192, 255);
        }

      </style>
    )html";

  SimpleLoopApp() {
    Bind(items);
    Bind(new_fruit);
    Bind(AddItem);
    Bind(RemoveItem);
    EnableHotReload();
  }
};

int main() {
  auto app = Ref<SimpleLoopApp>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
