#include "task_runner.hpp"

#include <cassert>
#include <thread>

namespace task {

static thread_local TaskRunner* current_task_runner = nullptr;  // NOLINT

// static
auto TaskRunner::Current() -> TaskRunner* {
  assert(current_task_runner);
  return current_task_runner;
}

auto TaskRunner::PostTask(Task task) -> void {
  queue_.PostTask(PendingTask{std::move(task)});
}

auto TaskRunner::PostDelayedTask(Task task,
                                 std::chrono::steady_clock::duration duration)
    -> void {
  queue_.PostTask(PendingTask{std::move(task), duration});
}

/// Runs the tasks in the queue.
auto TaskRunner::Run() -> void {
  // Install the current task runner, as the "current" running one.
  assert(!previous_task_runner_);
  previous_task_runner_ = current_task_runner;
  current_task_runner = this;

  while (true) {
    auto maybe_task = queue_.Get();
    if (std::holds_alternative<Task>(maybe_task)) {
      std::get<Task>(maybe_task)();
      continue;
    }

    auto duration = std::get<std::chrono::steady_clock::duration>(maybe_task);
    if (duration == std::chrono::steady_clock::duration::max()) {
      break;
    }

    // Sleep for the duration of the next task.
    std::this_thread::sleep_for(duration);
  }

  current_task_runner = previous_task_runner_;
  previous_task_runner_ = nullptr;
}

}  // namespace task
