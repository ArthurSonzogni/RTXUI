// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Looping over a collection of structs.
//
// A mapper exposes each struct's fields to the template, which reads them with
// dot notation on the loop variable.
//
// Try it: toggle a task to see only that row re-render.
#include <rtxui/rtxui.hpp>
#include <string>
#include <vector>

using namespace rtxui;

struct Task {
  std::string name;
  bool completed;
  bool operator==(const Task& other) const = default;
};

class ComplexLoopApp : public Component<ComplexLoopApp> {
 public:
  std::vector<Task> tasks = {{"Setup Project", true},
                             {"Implement Loops", true},
                             {"Write Docs", false}};
  std::string new_task_name = "";

  void AddTask() {
    if (!new_task_name.empty()) {
      tasks.push_back({new_task_name, false});
      new_task_name = "";
    }
  }
  void RemoveTask(std::string index_str) {
    size_t index = std::stoull(index_str);
    if (index < tasks.size()) {
      tasks.erase(tasks.begin() + index);
    }
  }

  std::string_view view = R"html(
      <div class="container">
        <div class="input-row">
          <input value="{new_task_name}" placeholder="New task name..." />
          <button onclick="AddTask">Add Task</button>
        </div>
        <div class="list">
          <for each="{tasks}" as="t">
            <div class="item">
              <span class="status">{t.status}</span>
              <span class="name">{t.name}</span>
              <button class="remove-btn" onclick="RemoveTask({$index})">X</button>
            </div>
          </for>
        </div>
      </div>
      <style>
        self {
          --border: rgb(48, 54, 61);
          --danger: rgb(248, 81, 73);

          display: block;
          padding: 1;
          background-color: rgb(13, 17, 23);
          color: white;
        }
        .container { padding: 1; }
        .input-row { display: flex; gap: 1; margin-bottom: 1; }
        input { width: 25; padding: 0 1; }
        .list { display: flex; flex-direction: column; gap: 0; border: tall; border-color: var(--border); }
        .item { display: flex; gap: 2; padding: 0 1; align-items: center; }
        .status { width: 10; color: rgb(56, 189, 248); }
        .name { flex-grow: 1; color: white; }
        .remove-btn { color: var(--danger); border: none; padding: 0 1; background-color: transparent; }
        .remove-btn:hover { color: white; background-color: var(--danger); }
        button { background-color: rgb(22, 27, 34); color: white; border: tall; border-color: var(--border); padding: 0 1; }
        button:hover { background-color: rgb(88, 166, 255); border-color: rgb(121, 192, 255); }
      </style>
    )html";

  ComplexLoopApp() {
    Bind(tasks, [](const Task& t) {
      return std::make_shared<ManualStructVisitor>(
          std::map<std::string, std::string, std::less<>>{
              {"name", t.name},
              {"status", t.completed ? "✅ Done" : "⏳ Pending"}});
    });
    Bind(new_task_name);
    Bind(AddTask);
    Bind(RemoveTask);
  }
};

int main() {
  auto app = Ref<ComplexLoopApp>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
