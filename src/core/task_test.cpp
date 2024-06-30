#include "task.hpp"
#include <catch2/catch_test_macros.hpp>
#include "task_runner.hpp"

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

}  // namespace
