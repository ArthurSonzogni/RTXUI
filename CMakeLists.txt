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
  "src/terminal/terminal_input_parser.cpp"
  "src/terminal/terminal_input_parser.hpp"
  "src/terminal/event.cpp"
  "src/terminal/event.hpp"
  "src/terminal/screen.cpp"
  "src/terminal/screen.hpp"
  "src/cell/cell.cpp"
  "src/cell/cell.hpp"
  "src/component/component.cpp"
  "src/component/component.hpp"
  "src/component/default_components.hpp"
  "src/component/import.hpp"
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
  "src/dom/slot_element.cpp"
  "src/dom/slot_element.hpp"
  "src/dom/text_element.cpp"
  "src/dom/text_element.hpp"
  "src/geometry/LogicalSize.cpp"
  "src/geometry/LogicalSize.hpp"
  "src/geometry/PhysicalDirection.cpp"
  "src/geometry/PhysicalDirection.hpp"
  "src/geometry/PhysicalOffset.cpp"
  "src/geometry/PhysicalOffset.hpp"
  "src/geometry/PhysicalSize.cpp"
  "src/geometry/PhysicalSize.hpp"
  "src/layout/layout.cpp"
  "src/layout/layout.hpp"
  "src/layout/layout_box.cpp"
  "src/layout/layout_box.hpp"
  "src/layout/layout_tree_builder.cpp"
  "src/layout/layout_tree_builder.hpp"
  "src/layout/physical_fragment.hpp"
  "src/layout/style.hpp"
  "src/paint/cell.cpp"
  "src/paint/cell.hpp"
  "src/paint/color.cpp"
  "src/paint/color.hpp"
  "src/paint/paint.cpp"
  "src/paint/paint.hpp"
  "src/paint/texture.cpp"
  "src/paint/texture.hpp"
  "src/register.cpp"
  "src/register.hpp"
  "src/style/apply_style.cpp"
  "src/style/apply_style.hpp"
  "src/style/style.cpp"
  "src/style/style.hpp"
  "src/xml/xml.cpp"
  "src/xml/xml.hpp"
)

target_compile_features(rtxui_lib PUBLIC cxx_std_26)
target_include_directories(rtxui_lib PUBLIC "src")

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
add_library(rtxui "src/dummy.cpp")
target_link_libraries(rtxui PUBLIC "$<LINK_LIBRARY:WHOLE_ARCHIVE,rtxui_lib>")

add_executable(rtxui_main main.cpp)
target_link_libraries(rtxui_main PRIVATE rtxui_lib)

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
    "src/terminal/terminal_input_parser_test.cpp"
    "src/terminal/screen_test.cpp"
    "src/component/component_test.cpp"
    "src/component/interpolation_test.cpp"
    "src/core/refcounted_test.cpp"    "src/core/task_test.cpp"
    "src/core/unicode_test.cpp"
    "src/layout/layout_test.cpp"
    "src/paint/border_style_test.cpp"
    "src/reflection/reflection_test.cpp"
    "src/style/style_test.cpp"
    "src/xml/xml_test.cpp"
  )
  target_link_libraries(rtxui_test PRIVATE rtxui_lib)
  target_link_libraries(rtxui_test PRIVATE Catch2::Catch2WithMain)
  list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
  include(CTest)
  include(Catch)
  catch_discover_tests(rtxui_test)

  add_executable(rtxui_terminal_input_parser
    "src/terminal/terminal_input_parser_main.cpp"
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
    "src/style/style_fuzzer.cpp"
    "src/terminal/terminal_input_parser_test_fuzzer.cpp"
    "src/xml/xml_fuzzer.cpp"
  )
  include(GoogleTest)
  target_link_libraries(rtxui_fuzzer PRIVATE rtxui_lib)
  link_fuzztest(rtxui_fuzzer)
  gtest_discover_tests(rtxui_fuzzer)

  # FUZZ test requires a definition of to run in fuzzing mode
  target_compile_definitions(rtxui_fuzzer PRIVATE FUZZTEST_FUZZING_ENABLED)

endif()

