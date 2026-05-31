#include <rtxui/rtxui.hpp>
#include <vector>
#include <string>

using namespace rtxui;

struct Task {
  std::string name;
  bool completed;
  bool operator==(const Task& other) const = default;
};

class ComplexLoopApp : public Component<ComplexLoopApp> {
 public:
  std::vector<Task> tasks = {
    {"Setup Project", true},
    {"Implement Loops", true},
    {"Write Docs", false}
  };
  std::string new_task_name = "";

  ComplexLoopApp() {
    BindCollection("tasks", &tasks, [](const Task& t) {
      return std::make_shared<ManualStructVisitor>(std::unordered_map<std::string, std::string>{
        {"name", t.name},
        {"status", t.completed ? "✅ Done" : "⏳ Pending"}
      });
    });
    Bind(new_task_name);

    Import("AddTask", [this]() {
      if (!new_task_name.empty()) {
        tasks.push_back({new_task_name, false});
        new_task_name = "";
      }
    });

    Import("RemoveTask", [this](std::string index_str) {
      size_t index = std::stoull(index_str);
      if (index < tasks.size()) {
        tasks.erase(tasks.begin() + index);
      }
    });
  }

  std::string_view Setup() override {
    return R"html(
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
        .container { padding: 1; }
        .input-row { display: flex; gap: 1; margin-bottom: 1; }
        input { border: solid; width: 25; padding: 0 1; }
        .list { display: flex; flex-direction: column; gap: 0; border: tall; }
        .item { display: flex; gap: 2; padding: 0 1; align-items: center; }
        .status { width: 10; color: yellow; }
        .name { flex-grow: 1; color: white; }
        .remove-btn { color: red; border: none; padding: 0 1; }
        button { background-color: blue; color: white; }
      </style>
    )html";
  }
};

int main() {
  auto app = Ref<ComplexLoopApp>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
