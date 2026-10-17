// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Looping over a collection of structs.
//
// A mapper exposes each struct's fields to the template, which reads them with
// dot notation on the loop variable.
//
// Try it: click a status to toggle it, then add or remove a task.
#include <cstddef>
#include <rtxui/rtxui.hpp>
#include <string>
#include <string_view>
#include <vector>

using namespace rtxui;

struct Task {
  std::string name;
  bool completed;
  bool operator==(const Task& other) const = default;
};

// What a template reads of a Task: {task.name} and {task.status}.
void Fields(const Task& task, FieldSink& out) {
  out("name", task.name);
  out("status", task.completed ? "done" : "todo");
}

class ComplexLoopApp : public Component<ComplexLoopApp> {
 public:
  std::vector<Task> tasks = {
      {"Set up the project", true},
      {"Write the loops", true},
      {"Write the docs", false},
  };
  std::string new_task_name;

  void AddTask() {
    if (!new_task_name.empty()) {
      tasks.push_back({new_task_name, false});
      new_task_name.clear();
    }
  }
  void ToggleTask(std::string index) {
    size_t i = std::stoull(index);
    if (i < tasks.size()) {
      tasks[i].completed = !tasks[i].completed;
    }
  }
  void RemoveTask(std::string index) {
    size_t i = std::stoull(index);
    if (i < tasks.size()) {
      tasks.erase(tasks.begin() + i);
    }
  }

  std::string_view view = R"html(
      <div class="card">
        <h1>Looping over structs</h1>
        <p>A Fields() function names what the template reads of each Task. Click a status to toggle it.</p>

        <div class="add">
          <input value="{new_task_name}" placeholder="A task..." />
          <button onclick="AddTask">Add</button>
        </div>

        <div class="list">
          <for each="{tasks}" as="task">
            <div class="row">
              <button class="status {task.status}" onclick="ToggleTask({$index})">{task.status}</button>
              <span class="grow">{task.name}</span>
              <button class="remove" onclick="RemoveTask({$index})">✕</button>
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
          height: 100%;
          background-color: rgb(13, 17, 23);
          color: rgb(230, 237, 243);
        }
        .card {
          border: tall;
          border-color: var(--border);
          background-color: rgb(22, 27, 34);
          padding: 1 3;
          width: 56;
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
        .status {
          border: none;
          width: 6;
        }
        .done {
          color: rgb(63, 185, 80);
        }
        .todo {
          color: rgb(210, 153, 34);
        }
      </style>
    )html";

  ComplexLoopApp() {
    Bind(tasks);
    Bind(new_task_name);
    Bind(AddTask);
    Bind(ToggleTask);
    Bind(RemoveTask);
  }
};

int main() {
  auto app = Ref<ComplexLoopApp>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
