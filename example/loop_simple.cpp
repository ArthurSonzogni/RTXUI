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
#include <cstddef>
#include <rtxui/rtxui.hpp>
#include <string>
#include <string_view>
#include <vector>

using namespace rtxui;

class SimpleLoopApp : public Component<SimpleLoopApp> {
 public:
  std::vector<std::string> items = {"Apple", "Banana", "Cherry"};
  std::string new_fruit;

  void AddItem() {
    if (!new_fruit.empty()) {
      items.push_back(new_fruit);
      new_fruit.clear();
    }
  }
  void RemoveItem(std::string index) {
    size_t i = std::stoull(index);
    if (i < items.size()) {
      items.erase(items.begin() + i);
    }
  }

  std::string_view view = R"html(
      <div class="card">
        <h1>Loops</h1>
        <p>&lt;for each&gt; repeats its content per element, and $index gives the row's position to a handler.</p>

        <div class="add">
          <input value="{new_fruit}" placeholder="A fruit..." />
          <button onclick="AddItem">Add</button>
        </div>

        <div class="list">
          <for each="{items}" as="fruit">
            <div class="row">
              <span class="grow">{fruit}</span>
              <button class="remove" onclick="RemoveItem({$index})">✕</button>
            </div>
          </for>
        </div>
      </div>

      <style>
        self {
          --border: rgb(48, 54, 61);
          --muted: rgb(139, 148, 158);
          --accent: rgb(88, 166, 255);

          display: flex;
          align-items: center;
          justify-content: center;
          width: 100%;
          color: rgb(230, 237, 243);
        }
        .card {
          border: tall;
          border-color: var(--border);
          background-color: rgb(22, 27, 34);
          padding: 1 3;
          width: 50;
        }
        h1 {
          color: var(--accent);
        }
        p {
          color: var(--muted);
        }
        .add {
          display: flex;
          gap: 1;
          margin-bottom: 1;
        }
        input {
          flex-grow: 1;
          border: tall;
          border-color: var(--border);
          background-color: rgb(13, 17, 23);
          padding: 0 1;
        }
        input:focus {
          border-color: var(--accent);
        }
        button {
          border: tall;
          border-color: var(--border);
          background-color: rgb(13, 17, 23);
          padding: 0 1;
        }
        button:hover, button:focus {
          border-color: var(--accent);
          color: var(--accent);
        }
        .list {
          border: round;
          border-color: var(--border);
        }
        .row {
          display: flex;
          gap: 2;
          padding: 0 1;
        }
        .row:hover {
          background-color: rgb(33, 38, 45);
        }
        .grow {
          flex-grow: 1;
        }
        .remove {
          border: none;
          background-color: transparent;
          color: rgb(248, 81, 73);
        }
        .remove:hover, .remove:focus {
          background-color: rgb(73, 32, 36);
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
