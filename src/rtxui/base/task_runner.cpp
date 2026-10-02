// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/base/task_runner.hpp"

#include <algorithm>
#include <cassert>
#include <mutex>
#include <thread>
#include <vector>

#include "rtxui/task.hpp"

namespace task {

static thread_local TaskRunner* current_task_runner = nullptr;  // NOLINT

namespace {

// Every live runner, newest last, for rtxui::PostTask from threads that run
// none. Guarded by a mutex held while posting, so a runner cannot be
// destroyed mid-post.
std::mutex& LiveRunnersMutex() {
  static std::mutex mutex;
  return mutex;
}
std::vector<TaskRunner*>& LiveRunners() {
  static std::vector<TaskRunner*> runners;
  return runners;
}

}  // namespace

// static
auto TaskRunner::Current() -> TaskRunner* {
  assert(current_task_runner);
  return current_task_runner;
}

TaskRunner::TaskRunner() {
  current_task_runner = this;
  std::lock_guard<std::mutex> lock(LiveRunnersMutex());
  LiveRunners().push_back(this);
}

TaskRunner::~TaskRunner() {
  {
    std::lock_guard<std::mutex> lock(LiveRunnersMutex());
    std::erase(LiveRunners(), this);
  }
  if (current_task_runner == this) {
    current_task_runner = nullptr;
  }
}

auto TaskRunner::PostTask(Task task) -> void {
  queue_.PostTask(PendingTask{std::move(task)});
  WakeupCallback wakeup;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    wakeup = wakeup_callback_;
  }
  if (wakeup) {
    wakeup();
  }
}

auto TaskRunner::PostDelayedTask(Task task,
                                 std::chrono::steady_clock::duration duration)
    -> void {
  queue_.PostTask(PendingTask{std::move(task), duration});
  WakeupCallback wakeup;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    wakeup = wakeup_callback_;
  }
  if (wakeup) {
    wakeup();
  }
}

auto TaskRunner::SetWakeupCallback(WakeupCallback callback) -> void {
  std::lock_guard<std::mutex> lock(mutex_);
  wakeup_callback_ = std::move(callback);
}

/// Runs the tasks in the queue.
auto TaskRunner::RunUntilNextDelayedTask(bool* executed_any)
    -> std::chrono::steady_clock::duration {
  // Install the current task runner, as the "current" running one.
  assert(!previous_task_runner_);
  previous_task_runner_ = current_task_runner;
  current_task_runner = this;

  while (true) {
    auto maybe_task = queue_.Get();
    if (std::holds_alternative<std::monostate>(maybe_task)) {
      // No more tasks to execute, exit the loop.
      current_task_runner = previous_task_runner_;
      previous_task_runner_ = nullptr;
      return std::chrono::steady_clock::duration::zero();
    }

    if (std::holds_alternative<Task>(maybe_task)) {
      if (executed_any) {
        *executed_any = true;
      }
      std::get<Task>(maybe_task)();
      continue;
    }

    if (std::holds_alternative<std::chrono::steady_clock::duration>(
            maybe_task)) {
      current_task_runner = previous_task_runner_;
      previous_task_runner_ = nullptr;
      return std::get<std::chrono::steady_clock::duration>(maybe_task);
    }
  }
}

auto TaskRunner::Run() -> void {
  while (true) {
    auto duration = RunUntilNextDelayedTask();
    if (duration == std::chrono::steady_clock::duration::zero()) {
      // No more tasks to execute, exit the loop.
      return;
    }

    // Sleep for the duration until the next task can be executed.
    std::this_thread::sleep_for(duration);
  }
}

}  // namespace task

namespace rtxui {

void PostTask(std::function<void()> task) {
  // On a thread with an event loop, post to that loop.
  if (task::current_task_runner) {
    task::current_task_runner->PostTask(std::move(task));
    return;
  }
  // From a worker thread, post to the application's loop. Once it is gone
  // there is nothing left to run the task, so it is dropped.
  std::lock_guard<std::mutex> lock(task::LiveRunnersMutex());
  if (!task::LiveRunners().empty()) {
    task::LiveRunners().back()->PostTask(std::move(task));
  }
}

}  // namespace rtxui
