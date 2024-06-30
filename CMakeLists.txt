cmake_minimum_required(VERSION 3.12)

project(cpp_react_tui
  LANGUAGES CXX
  VERSION 5.0.0
  DESCRIPTION "C++ reactive terminal renderering"
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(cpp_react_tui
  "src/core/expected.hpp"
  "src/core/likely.hpp"
  "src/core/string.cpp"
  "src/core/string.hpp"
  "src/core/task.cpp"
  "src/core/task.hpp"
  "src/core/task_queue.cpp"
  "src/core/task_queue.hpp"
  "src/core/task_runner.cpp"
  "src/core/task_runner.hpp"
  "src/dom/style.hpp"
  "src/paint/cell.cpp"
  "src/paint/cell.hpp"
  "src/paint/color.hpp"
  "src/paint/texture.cpp"
  "src/paint/texture.hpp"
  "src/reactive/reactive.cpp"
  "src/reactive/reactive.hpp"
  "src/xml/xml.cpp"
  "src/xml/xml.hpp"
  "src/app.cpp"
  "src/app.hpp"
  "src/element.cpp"
  "src/element.hpp"
)

target_compile_features(cpp_react_tui PUBLIC cxx_std_23)
target_include_directories(cpp_react_tui PUBLIC "src")

# An option to enable testing
option(CPP_REACT_TUI_BUILD_TESTS "Build tests" OFF)
if(CPP_REACT_TUI_BUILD_TESTS)
  enable_testing()

  # Use FetchContent to download Catch2
  include(FetchContent)
  FetchContent_Declare(catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2
    GIT_TAG v3.6.0
  )
  FetchContent_MakeAvailable(catch2)

  add_executable(cpp_react_tui_test
    "src/core/task_test.cpp"
    "src/reactive/reactive_test.cpp"
    "src/xml/xml_test.cpp"
    "src/app_test.cpp"
  )
  target_link_libraries(cpp_react_tui_test PRIVATE cpp_react_tui)
  target_link_libraries(cpp_react_tui_test PRIVATE Catch2::Catch2WithMain)
endif()
