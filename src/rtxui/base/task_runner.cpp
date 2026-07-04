// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/base/task_runner.hpp"

#include <cassert>
#include <thread>

namespace task {

static thread_local TaskRunner* current_task_runner = nullptr;  // NOLINT

// static
auto TaskRunner::Current() -> TaskRunner* {
  assert(current_task_runner);
  return current_task_runner;
}

TaskRunner::TaskRunner() {
  current_task_runner = this;
}

TaskRunner::~TaskRunner() {
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
auto TaskRunner::RunUntilNextDelayedTask()
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
