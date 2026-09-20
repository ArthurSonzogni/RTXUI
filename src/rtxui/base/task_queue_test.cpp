// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/base/task_queue.hpp"

#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <thread>
#include <vector>

namespace {

using namespace std::chrono_literals;

// Runs the queue until it reports no work left, collecting nothing itself --
// the tasks record their own order into whatever the caller captured.
void DrainReady(task::TaskQueue& queue) {
  while (true) {
    task::TaskQueue::MaybeTask next = queue.Get();
    if (auto* ready = std::get_if<task::Task>(&next)) {
      (*ready)();
      continue;
    }
    return;
  }
}

TEST_CASE("TaskQueue runs immediate tasks in the order posted", "[task]") {
  task::TaskQueue queue;
  std::vector<int> order;

  for (int i = 0; i < 5; ++i) {
    queue.PostTask(task::Task([&order, i] { order.push_back(i); }));
  }
  DrainReady(queue);

  CHECK(order == std::vector<int>{0, 1, 2, 3, 4});
}

TEST_CASE("TaskQueue reports the delay until the next delayed task", "[task]") {
  task::TaskQueue queue;
  bool ran = false;
  queue.PostTask(task::PendingTask([&ran] { ran = true; }, 1h));

  // Nothing is due, so Get() hands back how long to wait rather than a task.
  task::TaskQueue::MaybeTask next = queue.Get();
  auto* delay = std::get_if<std::chrono::steady_clock::duration>(&next);
  REQUIRE(delay != nullptr);
  CHECK(*delay > 0s);
  CHECK(*delay <= 1h);
  CHECK_FALSE(ran);
}

TEST_CASE("TaskQueue reports monostate when it is empty", "[task]") {
  task::TaskQueue queue;
  CHECK(std::holds_alternative<std::monostate>(queue.Get()));

  // Still empty once drained, rather than reporting a zero delay forever.
  queue.PostTask(task::Task([] {}));
  DrainReady(queue);
  CHECK(std::holds_alternative<std::monostate>(queue.Get()));
}

TEST_CASE("TaskQueue treats a task dated in the past as immediate", "[task]") {
  task::TaskQueue queue;
  bool ran = false;
  queue.PostTask(task::PendingTask([&ran] { ran = true; }, -1h));

  auto next = queue.Get();
  REQUIRE(std::holds_alternative<task::Task>(next));
  std::get<task::Task>(next)();
  CHECK(ran);
}

TEST_CASE("TaskQueue promotes delayed tasks once they come due", "[task]") {
  task::TaskQueue queue;
  std::vector<std::string> order;

  queue.PostTask(
      task::PendingTask([&order] { order.push_back("delayed"); }, 20ms));
  queue.PostTask(task::Task([&order] { order.push_back("immediate"); }));

  // The immediate task is available straight away; the delayed one is not.
  DrainReady(queue);
  CHECK(order == std::vector<std::string>{"immediate"});

  std::this_thread::sleep_for(40ms);
  DrainReady(queue);
  CHECK(order == std::vector<std::string>{"immediate", "delayed"});
}

TEST_CASE("TaskQueue orders due delayed tasks by their deadline", "[task]") {
  task::TaskQueue queue;
  std::vector<int> order;

  // Posted latest-first, so anything that preserved post order rather than
  // deadline order would come out reversed.
  queue.PostTask(task::PendingTask([&order] { order.push_back(3); }, 30ms));
  queue.PostTask(task::PendingTask([&order] { order.push_back(1); }, 10ms));
  queue.PostTask(task::PendingTask([&order] { order.push_back(2); }, 20ms));

  std::this_thread::sleep_for(60ms);
  DrainReady(queue);

  CHECK(order == std::vector<int>{1, 2, 3});
}

TEST_CASE("TaskQueue accepts tasks posted from other threads", "[task]") {
  task::TaskQueue queue;
  std::atomic<int> ran = 0;

  constexpr int kThreads = 4;
  constexpr int kPerThread = 50;

  std::vector<std::thread> threads;
  threads.reserve(kThreads);
  for (int t = 0; t < kThreads; ++t) {
    threads.emplace_back([&queue, &ran] {
      for (int i = 0; i < kPerThread; ++i) {
        queue.PostTask(task::Task(
            [&ran] { ran.fetch_add(1, std::memory_order_relaxed); }));
      }
    });
  }
  for (std::thread& thread : threads) {
    thread.join();
  }

  DrainReady(queue);
  CHECK(ran.load() == kThreads * kPerThread);
}

}  // namespace
