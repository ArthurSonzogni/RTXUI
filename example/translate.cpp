// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Moving boxes with translate.
//
// `translate` moves a box as it is drawn and clicked, after layout, so nothing
// around it moves, and its percentages are of the box's own size. The cards
// lift a cell when hovered or focused, through a transition. Each task slides
// out of its list once done, an animation whose end, `onanimationend`, removes
// it.
//
// Try it: hover or Tab through the cards and click one, then click the tasks,
// or focus one and press Enter.
#include <algorithm>
#include <map>
#include <memory>
#include <rtxui/rtxui.hpp>
#include <string>
#include <vector>

using namespace rtxui;

struct Task {
  std::string id;
  std::string name;
  bool leaving = false;
  // Bound state is compared each frame to find what changed.
  bool operator==(const Task&) const = default;
};

std::vector<Task> AllTasks() {
  return {
      {"1", "Write the release notes"},
      {"2", "Answer the open issues"},
      {"3", "Update the screenshots"},
      {"4", "Tag the release"},
  };
}

class TranslateDemo : public Component<TranslateDemo> {
 public:
  std::string picked = "nothing yet";
  std::vector<Task> tasks = AllTasks();

  void Pick(std::string card) { picked = std::move(card); }

  // Starts the task sliding out; Remove() runs once it is out of sight.
  void Done(std::string id) {
    for (Task& task : tasks) {
      if (task.id == id) {
        task.leaving = true;
      }
    }
  }
  void Remove(std::string id) {
    std::erase_if(tasks, [&](const Task& task) { return task.id == id; });
  }
  void Reset() { tasks = AllTasks(); }
  bool AllDone() const { return tasks.empty(); }

  TranslateDemo() {
    Bind(picked);
    Bind(tasks, [](const Task& task) {
      return std::make_shared<ManualStructVisitor>(
          std::map<std::string, std::string, std::less<>>{
              {"id", task.id},
              {"name", task.name},
              {"state", task.leaving ? "leaving" : ""}});
    });
    Bind(Pick);
    Bind(Done);
    Bind(Remove);
    Bind(Reset);
    Bind(AllDone);
  }

  std::string_view view = R"html(
    <div class="page">
      <h1>translate</h1>

      <p class="label">Cards lift when hovered or focused:</p>
      <div class="cards">
        <div class="card" tabindex="0" onclick="Pick(Files)">Files</div>
        <div class="card" tabindex="0" onclick="Pick(Search)">Search</div>
        <div class="card" tabindex="0" onclick="Pick(Settings)">Settings</div>
      </div>
      <p class="label">Picked: {picked}</p>

      <p class="label">Tasks slide out once done:</p>
      <div class="tasks">
        <for each="{tasks}" as="task" key="{task.id}">
          <div class="task {task.state}" tabindex="0"
               onclick="Done({task.id})" onanimationend="Remove({task.id})">
            ☐ {task.name}
          </div>
        </for>
        <div class="all-done" if="{AllDone}">
          All done. <button onclick="Reset">Reset</button>
        </div>
      </div>
    </div>

    <style>
      self {
        display: block;
        width: 100%;
        height: 100%;
        background-color: rgb(13, 17, 23);
        color: rgb(230, 237, 243);
      }
      .page {
        padding: 0 2;
      }
      h1 {
        color: rgb(88, 166, 255);
        font-weight: bold;
      }
      .label {
        color: rgb(139, 148, 158);
      }

      /* Room above the cards for them to lift into. */
      .cards {
        display: flex;
        gap: 2;
        margin-top: 1;
      }
      .card {
        width: 14;
        padding: 0 1;
        border: tall;
        border-color: rgb(48, 54, 61);
        background-color: rgb(22, 27, 34);
        transition: translate 150ms ease-out, border-color 150ms;
      }
      .card:hover, .card:focus {
        translate: 0 -1;
        border-color: rgb(88, 166, 255);
      }

      /* Clipped, so a task leaving goes out of sight at the list's edge. */
      .tasks {
        width: 40;
        overflow: hidden;
      }
      .task {
        padding: 0 1;
        background-color: rgb(22, 27, 34);
      }
      .task:focus {
        background-color: rgb(31, 111, 235);
      }
      @keyframes leave {
        to { translate: calc(100% + 2); }
      }
      .task.leaving {
        animation: leave 300ms ease-in forwards;
      }
      .all-done {
        color: rgb(63, 185, 80);
      }
    </style>
  )html";
};

int main() {
  auto app = Ref<TranslateDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
