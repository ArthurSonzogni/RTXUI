// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/base/task.hpp"

#include <catch2/catch_test_macros.hpp>

#include "rtxui/base/task_runner.hpp"

#include <thread>
#include <condition_variable>
#include <mutex>

namespace {

TEST_CASE("Task basic", "[task]") {
  std::vector<int> values;

  auto task_1 = [&values] { values.push_back(1); };
  auto task_2 = [&values] { values.push_back(2); };
  auto task_3 = [&values] { values.push_back(3); };

  auto runner = task::TaskRunner();

  runner.PostTask(task_1);
  runner.PostTask(task_2);
  runner.PostTask(task_3);
  runner.Run();

  REQUIRE(values == std::vector<int>{1, 2, 3});
}

TEST_CASE("Task posted within task", "[task]") {
  std::vector<int> values;

  auto task_1 = [&values] {
    values.push_back(1);
    auto task_2 = [&values] { values.push_back(5); };
    task::TaskRunner::Current()->PostTask(std::move(task_2));
    values.push_back(2);
  };

  auto task_2 = [&values] {
    values.push_back(3);
    auto task_2 = [&values] { values.push_back(6); };
    task::TaskRunner::Current()->PostTask(std::move(task_2));
    values.push_back(4);
  };

  auto runner = task::TaskRunner();

  runner.PostTask(task_1);
  runner.PostTask(task_2);
  runner.Run();

  REQUIRE(values == std::vector<int>{1, 2, 3, 4, 5, 6});
}

TEST_CASE("Run delayed task", "[task]") {
  std::vector<int> values;

  auto task_1 = [&values] { values.push_back(1); };
  auto task_2 = [&values] { values.push_back(2); };
  auto task_3 = [&values] { values.push_back(3); };

  auto runner = task::TaskRunner();

  runner.PostDelayedTask(task_3, std::chrono::milliseconds(300));
  runner.PostDelayedTask(task_1, std::chrono::milliseconds(100));
  runner.PostDelayedTask(task_2, std::chrono::milliseconds(200));
  runner.Run();

  REQUIRE(values == std::vector<int>{1, 2, 3});
}

TEST_CASE("Task posting from background thread", "[task][multithread]") {
  std::vector<int> values;
  auto runner = task::TaskRunner();

  std::mutex test_mutex;
  std::condition_variable cv;
  bool started = false;

  auto* runner_ptr = &runner;

  std::thread bg_thread([runner_ptr, &values, &test_mutex, &cv, &started]() {
    {
      std::unique_lock<std::mutex> lock(test_mutex);
      cv.wait(lock, [&started]() { return started; });
    }

    runner_ptr->PostTask([&values]() {
      values.push_back(42);
    });
  });

  bool wakeup_called = false;
  runner.SetWakeupCallback([&wakeup_called]() {
    wakeup_called = true;
  });

  {
    std::lock_guard<std::mutex> lock(test_mutex);
    started = true;
  }
  cv.notify_one();

  bg_thread.join();
  runner.Run();

  REQUIRE(wakeup_called);
  REQUIRE(values == std::vector<int>{42});
}

}  // namespace
