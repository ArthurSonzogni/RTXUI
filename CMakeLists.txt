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

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
endif()


add_library(rtxui_lib STATIC
  "src/rtxui/terminal/terminal_input_parser.cpp"
  "src/rtxui/terminal/terminal_input_parser.hpp"
  "src/rtxui/terminal/event.cpp"
  "include/rtxui/internal/event.hpp"
  "src/rtxui/terminal/screen.cpp"
  "include/rtxui/internal/screen.hpp"
  "src/rtxui/component/component.cpp"
  "include/rtxui/internal/component.hpp"
  "src/rtxui/component/default/button/button.cpp"
  "src/rtxui/component/default/checkbox/checkbox.cpp"
  "src/rtxui/component/default/div/div.cpp"
  "src/rtxui/component/default/h1/h1.cpp"
  "src/rtxui/component/default/hr/hr.cpp"
  "src/rtxui/component/default/input/input.cpp"
  "src/rtxui/component/default/input/text_input_base.cpp"
  "src/rtxui/component/default/li/li.cpp"
  "src/rtxui/component/default/ol/ol.cpp"
  "src/rtxui/component/default/option/option.cpp"
  "src/rtxui/component/default/p/p.cpp"
  "src/rtxui/component/default/progress/progress.cpp"
  "src/rtxui/component/default/select/select.cpp"
  "src/rtxui/component/default/slider/slider.cpp"
  "src/rtxui/component/default/span/span.cpp"
  "src/rtxui/component/default/b/b.cpp"
  "src/rtxui/component/default/strong/strong.cpp"
  "src/rtxui/component/default/textarea/textarea.cpp"
  "src/rtxui/component/default/markdown/markdown.cpp"
  "src/rtxui/component/default/ul/ul.cpp"
  "include/rtxui/internal/import.hpp"
  "src/rtxui/core/expected.hpp"
  "src/rtxui/core/likely.hpp"
  "include/rtxui/internal/refcounted.hpp"
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
  "src/rtxui/markdown/markdown.cpp"
  "src/rtxui/markdown/markdown.hpp"
)

target_compile_features(rtxui_lib PUBLIC cxx_std_26)
target_include_directories(rtxui_lib PUBLIC "include" "src")

if(EMSCRIPTEN)
  add_compile_options("-sASYNCIFY")
  add_link_options("-sASYNCIFY" "-sEXPORTED_RUNTIME_METHODS=[ccall,cwrap]" "-sASYNCIFY_IMPORTS=[emscripten_asm_const_int]")
endif()

# Enable experimental reflection if supported by the compiler
include(CheckCXXSourceCompiles)
set(SAFE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
set(CMAKE_REQUIRED_FLAGS "-std=c++26")

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(CMAKE_REQUIRED_FLAGS "-std=c++26 -freflection-latest")
  check_cxx_source_compiles("
    #include <meta>
    struct S { int x; };
    int main() {
      constexpr auto m = std::meta::nonstatic_data_members_of(^^S, std::meta::access_context::unchecked()).size();
      return 0;
    }
  " HAS_REFLECTION_CLANG)
  if(HAS_REFLECTION_CLANG)
    target_compile_options(rtxui_lib PUBLIC "-freflection-latest")
    add_definitions(-DRTXUI_HAS_REFLECTION)
    message(STATUS "RTXUI: C++26 Reflection enabled (-freflection-latest)")
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(CMAKE_REQUIRED_FLAGS "-std=c++26 -freflection")
  check_cxx_source_compiles("
    #include <meta>
    struct S { int x; };
    int main() {
      constexpr auto m = std::meta::nonstatic_data_members_of(^^S, std::meta::access_context::unchecked()).size();
      return 0;
    }
  " HAS_REFLECTION_GCC)
  if(HAS_REFLECTION_GCC)
    target_compile_options(rtxui_lib PUBLIC "-freflection")
    add_definitions(-DRTXUI_HAS_REFLECTION)
    message(STATUS "RTXUI: C++26 Reflection enabled (-freflection)")
  endif()
endif()
set(CMAKE_REQUIRED_FLAGS "${SAFE_CMAKE_REQUIRED_FLAGS}")

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

  add_executable(rtxui_example_nested_scroll example/nested_scroll.cpp)
  target_link_libraries(rtxui_example_nested_scroll PRIVATE rtxui)

  add_executable(rtxui_example_horizontal_scroll example/horizontal_scroll.cpp)
  target_link_libraries(rtxui_example_horizontal_scroll PRIVATE rtxui)

  add_executable(rtxui_example_scroll_behavior example/scroll_behavior.cpp)
  target_link_libraries(rtxui_example_scroll_behavior PRIVATE rtxui)

  add_executable(rtxui_example_helloworld example/helloworld.cpp)
  target_link_libraries(rtxui_example_helloworld PRIVATE rtxui)

  add_executable(rtxui_example_loop example/loop.cpp)
  target_link_libraries(rtxui_example_loop PRIVATE rtxui)

  add_executable(rtxui_example_loop_simple example/loop_simple.cpp)
  target_link_libraries(rtxui_example_loop_simple PRIVATE rtxui)

  add_executable(rtxui_example_loop_complex example/loop_complex.cpp)
  target_link_libraries(rtxui_example_loop_complex PRIVATE rtxui)

  add_executable(rtxui_example_conditional example/conditional.cpp)
  target_link_libraries(rtxui_example_conditional PRIVATE rtxui)

  add_executable(rtxui_example_colors example/colors.cpp)
  target_link_libraries(rtxui_example_colors PRIVATE rtxui)

  add_executable(rtxui_example_cjk example/cjk.cpp)
  target_link_libraries(rtxui_example_cjk PRIVATE rtxui)

  add_executable(rtxui_example_text_align example/text_align.cpp)
  target_link_libraries(rtxui_example_text_align PRIVATE rtxui)

  add_executable(rtxui_example_text_decoration example/text_decoration.cpp)
  target_link_libraries(rtxui_example_text_decoration PRIVATE rtxui)

  add_executable(rtxui_example_input example/input.cpp)
  target_link_libraries(rtxui_example_input PRIVATE rtxui)

  add_executable(rtxui_example_textarea example/textarea.cpp)
  target_link_libraries(rtxui_example_textarea PRIVATE rtxui)

  add_executable(rtxui_example_checkbox example/checkbox.cpp)
  target_link_libraries(rtxui_example_checkbox PRIVATE rtxui)

  add_executable(rtxui_example_slider example/slider.cpp)
  target_link_libraries(rtxui_example_slider PRIVATE rtxui)

  add_executable(rtxui_example_progress example/progress.cpp)
  target_link_libraries(rtxui_example_progress PRIVATE rtxui)

  add_executable(rtxui_example_select example/select.cpp)
  target_link_libraries(rtxui_example_select PRIVATE rtxui)

  add_executable(rtxui_example_hr example/hr.cpp)
  target_link_libraries(rtxui_example_hr PRIVATE rtxui)

  add_executable(rtxui_example_pseudo_classes example/pseudo_classes.cpp)
  target_link_libraries(rtxui_example_pseudo_classes PRIVATE rtxui)

  add_executable(rtxui_example_transitions example/transitions.cpp)
  target_link_libraries(rtxui_example_transitions PRIVATE rtxui)

  add_executable(rtxui_example_animation example/animation.cpp)
  target_link_libraries(rtxui_example_animation PRIVATE rtxui)

  add_executable(rtxui_example_positioning example/positioning.cpp)
  target_link_libraries(rtxui_example_positioning PRIVATE rtxui)

  add_executable(rtxui_example_sticky example/sticky.cpp)
  target_link_libraries(rtxui_example_sticky PRIVATE rtxui)

  add_executable(rtxui_example_tabindex example/tabindex.cpp)
  target_link_libraries(rtxui_example_tabindex PRIVATE rtxui)

  add_executable(rtxui_example_media example/media.cpp)
  target_link_libraries(rtxui_example_media PRIVATE rtxui)

  add_executable(rtxui_example_focus_scroll example/focus_scroll.cpp)
  target_link_libraries(rtxui_example_focus_scroll PRIVATE rtxui_lib)

  add_executable(rtxui_example_markdown example/markdown.cpp)
  target_link_libraries(rtxui_example_markdown PRIVATE rtxui_lib)

  add_executable(rtxui_example_opacity example/opacity.cpp)
  target_link_libraries(rtxui_example_opacity PRIVATE rtxui)

  add_executable(rtxui_example_table example/table.cpp)
  target_link_libraries(rtxui_example_table PRIVATE rtxui)

  add_executable(rtxui_example_lists example/lists.cpp)
  target_link_libraries(rtxui_example_lists PRIVATE rtxui)

  add_executable(rtxui_example_slots example/slots.cpp)
  target_link_libraries(rtxui_example_slots PRIVATE rtxui)

  add_executable(rtxui_example_spatial_navigation example/spatial_navigation.cpp)
  target_link_libraries(rtxui_example_spatial_navigation PRIVATE rtxui)
endif()

option(RTXUI_BUILD_BENCHMARKS "Build benchmarks" ON)
if(RTXUI_BUILD_BENCHMARKS)
  add_executable(rtxui_benchmark benchmarks/rtxui_benchmark.cpp)
  target_link_libraries(rtxui_benchmark PRIVATE rtxui)

  add_executable(rtxui_layout_benchmark benchmarks/rtxui_layout_benchmark.cpp)
  target_link_libraries(rtxui_layout_benchmark PRIVATE rtxui)

  add_executable(rtxui_steady_state_benchmark benchmarks/rtxui_steady_state_benchmark.cpp)
  target_link_libraries(rtxui_steady_state_benchmark PRIVATE rtxui)

  add_executable(rtxui_xml_benchmark benchmarks/rtxui_xml_benchmark.cpp)
  target_link_libraries(rtxui_xml_benchmark PRIVATE rtxui)
endif()


# An option to enable testing
if(RTXUI_BUILD_TESTS)
  enable_testing()

  # Verify that all CSS properties are documented
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME verify_css_docs
      COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/scripts/verify_docs.py"
    )
  endif()

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
    "src/rtxui/markdown/markdown_test.cpp"
  )
  target_link_libraries(rtxui_test PRIVATE rtxui_lib)
  target_link_libraries(rtxui_test PRIVATE Catch2::Catch2WithMain)
  target_include_directories(rtxui_test PRIVATE "src")
  list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
  include(CTest)
  include(Catch)
  catch_discover_tests(rtxui_test
    PROPERTIES SKIP_RETURN_CODE 4
  )

  add_executable(rtxui_terminal_input_parser
    "src/rtxui/terminal/terminal_input_parser_main.cpp"
  )
  target_link_libraries(rtxui_terminal_input_parser PRIVATE rtxui_lib)
  target_include_directories(rtxui_terminal_input_parser PRIVATE "src")

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
  target_include_directories(rtxui_fuzzer PRIVATE "src")
  link_fuzztest(rtxui_fuzzer)
  gtest_discover_tests(rtxui_fuzzer)

  # FUZZ test requires a definition of to run in fuzzing mode
  target_compile_definitions(rtxui_fuzzer PRIVATE FUZZTEST_FUZZING_ENABLED)

endif()

