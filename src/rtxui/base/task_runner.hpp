// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef TASK_RUNNER_HPP
#define TASK_RUNNER_HPP

#include <functional>
#include <mutex>

#include "rtxui/base/task.hpp"
#include "rtxui/base/task_queue.hpp"

namespace task {

class TaskRunner {
 public:
  TaskRunner();
  ~TaskRunner();

  // Returns the task runner for the current thread.
  static auto Current() -> TaskRunner*;

  /// Schedules a task to be executed immediately.
  auto PostTask(Task task) -> void;

  /// Schedules a task to be executed after a certain duration.
  auto PostDelayedTask(Task task, std::chrono::steady_clock::duration duration)
      -> void;

  /// Runs the tasks in the queue, return the delay until the next delayed task
  /// can be executed. When `executed_any` is provided, it is set to whether
  /// at least one task ran (callers use this to trigger a digest, since tasks
  /// commonly mutate component state).
  auto RunUntilNextDelayedTask(bool* executed_any = nullptr)
      -> std::chrono::steady_clock::duration;

  // Runs the tasks in the queue, blocking until all tasks are executed.
  auto Run() -> void;

  using WakeupCallback = std::function<void()>;
  auto SetWakeupCallback(WakeupCallback callback) -> void;

 private:
  TaskRunner* previous_task_runner_ = nullptr;
  TaskQueue queue_;
  mutable std::mutex mutex_;
  WakeupCallback wakeup_callback_;
};

}  // namespace task

#endif  // TASK_RUNNER_HPP
