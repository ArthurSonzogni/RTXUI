// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <rtxui/rtxui.hpp>

#include <chrono>
#include <thread>

#include "rtxui/base/task_runner.hpp"

using namespace rtxui;

// Cookbook recipe: background work without freezing the UI.
//
// Callbacks run on the main thread, so blocking inside one freezes rendering.
// Run the work on a std::thread and post the result back to the main loop
// with task::TaskRunner, which may be called from other threads.
class AsyncApp : public Component<AsyncApp> {
 public:
  std::string status = "Idle";
  bool is_loading = false;

  void StartBackgroundTask() {
    if (is_loading) {
      return;
    }
    status = "Fetching data...";
    is_loading = true;

    auto* main_runner = task::TaskRunner::Current();
    std::thread([this, main_runner]() {
      // Stand-in for real work: a network call, a database query, ...
      std::this_thread::sleep_for(std::chrono::seconds(2));

      // Only the main thread may touch component state. PostTask schedules
      // this lambda on the UI loop and wakes it.
      main_runner->PostTask([this]() {
        status = "Loaded 42 items.";
        is_loading = false;
      });
    }).detach();
  }

  AsyncApp() {
    Bind(status);
    Bind(is_loading);
    Bind(StartBackgroundTask);
  }

  std::string_view view = R"html(
    <div class="panel">
      <div>Status: <strong>{status}</strong></div>
      <if condition="{is_loading}">
        <div class="loader">Processing...</div>
      </if>
      <else>
        <button onclick="StartBackgroundTask">Trigger load</button>
      </else>
    </div>

    <style>
      .panel { padding: 1; border: solid; width: 40; }
      .loader { color: #f59e0b; }
    </style>
  )html";
};

int main() {
  auto app = Ref<AsyncApp>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
