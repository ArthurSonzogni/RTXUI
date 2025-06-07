cmake_minimum_required(VERSION 3.24)

project(rtxui
  LANGUAGES CXX
  VERSION 5.0.0
  DESCRIPTION "C++ reactive terminal renderering"
)

option(RTXUI_BUILD_TESTS "Build tests" OFF)
option(RTXUI_BUILD_EXAMPLES "Build examples" OFF)
option(RTXUI_BUILD_FUZZERS "Build fuzzers" OFF)

include(FetchContent)

add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set the default build type to 'Release'
if (CMAKE_BUILD_TYPE STREQUAL "")
  set(CMAKE_BUILD_TYPE "Release")
endif()

add_library(rtxui_lib STATIC
  "src/cell/cell.cpp"
  "src/cell/cell.hpp"
  "src/components/component.cpp"
  "src/components/component.hpp"
  "src/components/import.hpp"
  "src/components/default_components.hpp"
  "src/core/expected.hpp"
  "src/core/likely.hpp"
  "src/core/refcounted.hpp"
  "src/core/string.cpp"
  "src/core/string.hpp"
  "src/core/task.cpp"
  "src/core/task.hpp"
  "src/core/task_queue.cpp"
  "src/core/task_queue.hpp"
  "src/core/task_runner.cpp"
  "src/core/task_runner.hpp"
  "src/dom/computed_style.hpp"
  "src/dom/element.cpp"
  "src/dom/element.hpp"
  "src/dom/text_element.cpp"
  "src/dom/text_element.hpp"
  "src/dom/slot_element.cpp"
  "src/dom/slot_element.hpp"
  "src/paint/cell.cpp"
  "src/paint/cell.hpp"
  "src/paint/color.hpp"
  "src/paint/texture.cpp"
  "src/paint/texture.hpp"
  "src/reactive/reactive.cpp"
  "src/reactive/reactive.hpp"
  "src/register.cpp"
  "src/register.hpp"
  "src/layout/LayoutObject.hpp"
  "src/layout/LayoutObject.cpp"
  "src/layout/LayoutText.hpp"
  "src/layout/LayoutText.cpp"
  "src/layout/LayoutBox.hpp"
  "src/layout/LayoutBox.cpp"
  "src/xml/xml.cpp"
  "src/xml/xml.hpp"
)

target_compile_features(rtxui_lib PUBLIC cxx_std_23)
target_include_directories(rtxui_lib PUBLIC "src")

# See https://stackoverflow.com/questions/53071878/using-whole-archive-linker-option-with-cmake-and-libraries-with-other-library
add_library(rtxui "src/dummy.cpp")
target_link_libraries(rtxui PUBLIC "$<LINK_LIBRARY:WHOLE_ARCHIVE,rtxui_lib>")

# An option to enable testing
if(RTXUI_BUILD_TESTS)
  enable_testing()

  # Catch2
  FetchContent_Declare(catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2
    GIT_TAG v3.6.0
    EXCLUDE_FROM_ALL
    GIT_PROGRESS TRUE
  )
  FetchContent_MakeAvailable(catch2)

  add_executable(rtxui_test
    "src/cell/cell_test.cpp"
    "src/components/component_test.cpp"
    "src/core/refcounted_test.cpp"
    "src/core/task_test.cpp"
    "src/reactive/reactive_test.cpp"
    "src/reflection/reflection_test.cpp"
    "src/xml/xml_test.cpp"
  )
  target_link_libraries(rtxui_test PRIVATE rtxui)
  target_link_libraries(rtxui_test PRIVATE Catch2::Catch2WithMain)
  list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
  include(CTest)
  include(Catch)
  catch_discover_tests(rtxui_test)

endif()

if (RTXUI_BUILD_FUZZERS)
  enable_testing()

  # FuzzTest
  FetchContent_Declare(fuzztest
    GIT_REPOSITORY https://github.com/google/fuzztest.git
    GIT_TAG e576caaece16bd1f8dcd196736743c36474f3c16
    EXCLUDE_FROM_ALL
    GIT_PROGRESS TRUE
  )
  FetchContent_MakeAvailable(fuzztest)

  fuzztest_setup_fuzzing_flags()
  add_executable(rtxui_fuzzer
    "src/xml/xml_fuzzer.cpp"
  )
  include(GoogleTest)
  target_link_libraries(rtxui_fuzzer PRIVATE rtxui)
  link_fuzztest(rtxui_fuzzer)
  gtest_discover_tests(rtxui_fuzzer)
endif()
