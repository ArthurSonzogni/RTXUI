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

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the default build type to 'Release'
if (CMAKE_BUILD_TYPE STREQUAL "")
  set(CMAKE_BUILD_TYPE "Release")
endif()

add_library(rtxui_lib STATIC
  "src/rtxui/terminal/terminal_input_parser.cpp"
  "src/rtxui/terminal/terminal_input_parser.hpp"
  "src/rtxui/terminal/event.cpp"
  "src/rtxui/terminal/event.hpp"
  "src/rtxui/terminal/screen.cpp"
  "src/rtxui/terminal/screen.hpp"
  "src/rtxui/component/component.cpp"
  "src/rtxui/component/component.hpp"
  "src/rtxui/component/default_components.hpp"
  "src/rtxui/component/import.hpp"
  "src/rtxui/core/expected.hpp"
  "src/rtxui/core/likely.hpp"
  "src/rtxui/core/refcounted.hpp"
  "src/rtxui/core/string.cpp"
  "src/rtxui/core/string.hpp"
  "src/rtxui/core/task.cpp"
  "src/rtxui/core/task.hpp"
  "src/rtxui/core/task_queue.cpp"
  "src/rtxui/core/task_queue.hpp"
  "src/rtxui/core/task_runner.cpp"
  "src/rtxui/core/task_runner.hpp"
  "src/rtxui/dom/computed_style.hpp"
  "src/rtxui/dom/element.cpp"
  "src/rtxui/dom/element.hpp"
  "src/rtxui/dom/slot_element.cpp"
  "src/rtxui/dom/slot_element.hpp"
  "src/rtxui/dom/text_element.cpp"
  "src/rtxui/dom/text_element.hpp"
  "src/rtxui/geometry/LogicalSize.cpp"
  "src/rtxui/geometry/LogicalSize.hpp"
  "src/rtxui/geometry/PhysicalDirection.cpp"
  "src/rtxui/geometry/PhysicalDirection.hpp"
  "src/rtxui/geometry/PhysicalOffset.cpp"
  "src/rtxui/geometry/PhysicalOffset.hpp"
  "src/rtxui/geometry/PhysicalSize.cpp"
  "src/rtxui/geometry/PhysicalSize.hpp"
  "src/rtxui/layout/layout.cpp"
  "src/rtxui/layout/layout.hpp"
  "src/rtxui/layout/layout_box.cpp"
  "src/rtxui/layout/layout_box.hpp"
  "src/rtxui/layout/layout_tree_builder.cpp"
  "src/rtxui/layout/layout_tree_builder.hpp"
  "src/rtxui/layout/physical_fragment.hpp"
  "src/rtxui/layout/style.hpp"
  "src/rtxui/paint/cell.cpp"
  "src/rtxui/paint/cell.hpp"
  "src/rtxui/paint/color.cpp"
  "src/rtxui/paint/color.hpp"
  "src/rtxui/paint/paint.cpp"
  "src/rtxui/paint/paint.hpp"
  "src/rtxui/paint/texture.cpp"
  "src/rtxui/paint/texture.hpp"
  "src/rtxui/style/apply_style.cpp"
  "src/rtxui/style/apply_style.hpp"
  "src/rtxui/style/style.cpp"
  "src/rtxui/style/style.hpp"
  "src/rtxui/xml/xml.cpp"
  "src/rtxui/xml/xml.hpp"
)

target_compile_features(rtxui_lib PUBLIC cxx_std_26)
target_include_directories(rtxui_lib PUBLIC "src" "include")

# Enable experimental reflection if supported by the compiler
include(CheckCXXCompilerFlag)
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  check_cxx_compiler_flag("-freflection-latest" HAS_REFLECTION_CLANG)
  if(HAS_REFLECTION_CLANG)
    target_compile_options(rtxui_lib PUBLIC "-freflection-latest")
    add_definitions(-DRTXUI_HAS_REFLECTION)
    message(STATUS "RTXUI: C++26 Reflection enabled (-freflection-latest)")
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  check_cxx_compiler_flag("-freflection" HAS_REFLECTION_GCC)
  if(HAS_REFLECTION_GCC)
    target_compile_options(rtxui_lib PUBLIC "-freflection")
    add_definitions(-DRTXUI_HAS_REFLECTION)
    message(STATUS "RTXUI: C++26 Reflection enabled (-freflection)")
  endif()
endif()

# See https://stackoverflow.com/questions/53071878/using-whole-archive-linker-option-with-cmake-and-libraries-with-other-library
add_library(rtxui "src/rtxui/dummy.cpp")
target_link_libraries(rtxui PUBLIC "$<LINK_LIBRARY:WHOLE_ARCHIVE,rtxui_lib>")

if(RTXUI_BUILD_EXAMPLES)
  add_executable(rtxui_example_demo example/demo.cpp)
  target_link_libraries(rtxui_example_demo PRIVATE rtxui)

  add_executable(rtxui_example_counter example/counter.cpp)
  target_link_libraries(rtxui_example_counter PRIVATE rtxui)

  add_executable(rtxui_example_layout example/layout.cpp)
  target_link_libraries(rtxui_example_layout PRIVATE rtxui)

  add_executable(rtxui_example_borders example/borders.cpp)
  target_link_libraries(rtxui_example_borders PRIVATE rtxui)
endif()

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
    "src/rtxui/terminal/terminal_input_parser_test.cpp"
    "src/rtxui/terminal/screen_test.cpp"
    "src/rtxui/component/component_test.cpp"
    "src/rtxui/component/interpolation_test.cpp"
    "src/rtxui/core/refcounted_test.cpp"
    "src/rtxui/core/task_test.cpp"
    "src/rtxui/core/unicode_test.cpp"
    "src/rtxui/layout/layout_test.cpp"
    "src/rtxui/paint/border_style_test.cpp"
    "src/rtxui/reflection/reflection_test.cpp"
    "src/rtxui/style/style_test.cpp"
    "src/rtxui/xml/xml_test.cpp"
  )
  target_link_libraries(rtxui_test PRIVATE rtxui_lib)
  target_link_libraries(rtxui_test PRIVATE Catch2::Catch2WithMain)
  list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
  include(CTest)
  include(Catch)
  catch_discover_tests(rtxui_test)

  add_executable(rtxui_terminal_input_parser
    "src/rtxui/terminal/terminal_input_parser_main.cpp"
  )
  target_link_libraries(rtxui_terminal_input_parser PRIVATE rtxui_lib)

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
    "src/rtxui/style/style_fuzzer.cpp"
    "src/rtxui/terminal/terminal_input_parser_test_fuzzer.cpp"
    "src/rtxui/xml/xml_fuzzer.cpp"
  )
  include(GoogleTest)
  target_link_libraries(rtxui_fuzzer PRIVATE rtxui_lib)
  link_fuzztest(rtxui_fuzzer)
  gtest_discover_tests(rtxui_fuzzer)

  # FUZZ test requires a definition of to run in fuzzing mode
  target_compile_definitions(rtxui_fuzzer PRIVATE FUZZTEST_FUZZING_ENABLED)

endif()

