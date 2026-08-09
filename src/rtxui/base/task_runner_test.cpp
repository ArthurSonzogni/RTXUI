// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/base/task_runner.hpp"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

namespace {

using namespace std::chrono_literals;

TEST_CASE("TaskRunner runs posted tasks in order", "[task][task_runner]") {
  task::TaskRunner runner;
  std::vector<int> order;

  for (int i = 0; i < 4; ++i) {
    runner.PostTask([&order, i] { order.push_back(i); });
  }
  runner.RunUntilNextDelayedTask();

  CHECK(order == std::vector<int>{0, 1, 2, 3});
}

TEST_CASE("TaskRunner reports whether it ran anything",
          "[task][task_runner]") {
  task::TaskRunner runner;

  // The flag is what Screen uses to decide a digest is needed, so "nothing
  // ran" has to stay distinguishable from "something ran".
  bool executed = false;
  runner.RunUntilNextDelayedTask(&executed);
  CHECK_FALSE(executed);

  runner.PostTask([] {});
  executed = false;
  runner.RunUntilNextDelayedTask(&executed);
  CHECK(executed);
}

TEST_CASE("TaskRunner returns zero when the queue drains", "[task][task_runner]") {
  task::TaskRunner runner;
  runner.PostTask([] {});
  CHECK(runner.RunUntilNextDelayedTask() ==
        std::chrono::steady_clock::duration::zero());
}

TEST_CASE("TaskRunner returns the wait until a pending delayed task",
          "[task][task_runner]") {
  task::TaskRunner runner;
  bool ran = false;
  runner.PostDelayedTask([&ran] { ran = true; }, 1h);

  auto wait = runner.RunUntilNextDelayedTask();
  CHECK(wait > std::chrono::steady_clock::duration::zero());
  CHECK(wait <= 1h);
  CHECK_FALSE(ran);
}

TEST_CASE("TaskRunner runs a delayed task once it comes due",
          "[task][task_runner]") {
  task::TaskRunner runner;
  bool ran = false;
  runner.PostDelayedTask([&ran] { ran = true; }, 20ms);

  CHECK(runner.RunUntilNextDelayedTask() >
        std::chrono::steady_clock::duration::zero());
  CHECK_FALSE(ran);

  std::this_thread::sleep_for(40ms);
  CHECK(runner.RunUntilNextDelayedTask() ==
        std::chrono::steady_clock::duration::zero());
  CHECK(ran);
}

TEST_CASE("TaskRunner runs tasks posted from inside a task",
          "[task][task_runner]") {
  task::TaskRunner runner;
  std::vector<int> order;

  runner.PostTask([&] {
    order.push_back(1);
    // Re-entrant post: the runner must pick this up in the same drain rather
    // than leaving it stranded until the next one.
    task::TaskRunner::Current()->PostTask([&order] { order.push_back(2); });
  });
  runner.RunUntilNextDelayedTask();

  CHECK(order == std::vector<int>{1, 2});
}

TEST_CASE("TaskRunner::Current is the runner that is draining",
          "[task][task_runner]") {
  task::TaskRunner runner;
  task::TaskRunner* seen = nullptr;
  runner.PostTask([&seen] { seen = task::TaskRunner::Current(); });
  runner.RunUntilNextDelayedTask();
  CHECK(seen == &runner);
}

TEST_CASE("TaskRunner fires the wakeup callback on post",
          "[task][task_runner]") {
  task::TaskRunner runner;
  std::atomic<int> wakeups = 0;
  runner.SetWakeupCallback([&wakeups] { wakeups.fetch_add(1); });

  runner.PostTask([] {});
  CHECK(wakeups.load() == 1);

  runner.PostDelayedTask([] {}, 1h);
  CHECK(wakeups.load() == 2);
}

TEST_CASE("TaskRunner accepts posts from another thread",
          "[task][task_runner]") {
  task::TaskRunner runner;
  std::atomic<int> wakeups = 0;
  runner.SetWakeupCallback([&wakeups] { wakeups.fetch_add(1); });

  std::atomic<int> ran = 0;
  constexpr int kThreads = 4;
  constexpr int kPerThread = 25;

  std::vector<std::thread> threads;
  threads.reserve(kThreads);
  for (int t = 0; t < kThreads; ++t) {
    threads.emplace_back([&runner, &ran] {
      for (int i = 0; i < kPerThread; ++i) {
        runner.PostTask([&ran] { ran.fetch_add(1, std::memory_order_relaxed); });
      }
    });
  }
  for (std::thread& thread : threads) {
    thread.join();
  }

  runner.RunUntilNextDelayedTask();
  CHECK(ran.load() == kThreads * kPerThread);
  CHECK(wakeups.load() == kThreads * kPerThread);
}

TEST_CASE("TaskRunner::Run drains delayed tasks and returns",
          "[task][task_runner]") {
  task::TaskRunner runner;
  std::vector<int> order;

  runner.PostTask([&order] { order.push_back(1); });
  runner.PostDelayedTask([&order] { order.push_back(2); }, 10ms);
  runner.PostDelayedTask([&order] { order.push_back(3); }, 20ms);

  runner.Run();  // blocks until everything, including the delayed work, ran

  CHECK(order == std::vector<int>{1, 2, 3});
}

}  // namespace
