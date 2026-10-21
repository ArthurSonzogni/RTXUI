cmake_minimum_required(VERSION 3.24)

project(rtxui
  LANGUAGES CXX
  VERSION 0.1.1
  DESCRIPTION "C++ reactive terminal rendering"
)

option(RTXUI_BUILD_TESTS "Build tests" OFF)
option(RTXUI_BUILD_EXAMPLES "Build examples" OFF)
option(RTXUI_BUILD_MODULE "Build the rtxui C++ module (CMake 3.28+, Ninja)" OFF)
option(RTXUI_BUILD_FUZZERS "Build fuzzers" OFF)
option(RTXUI_SANITIZE "Build with Address and UndefinedBehavior sanitizers" OFF)

include(FetchContent)
include(cmake/compiler.cmake)

if(EMSCRIPTEN)
  add_compile_options(
    "-sASYNCIFY"
    "-fno-exceptions"
  )
  add_link_options(
    "-sASYNCIFY"
    "-fno-exceptions"
    "-sEXPORTED_RUNTIME_METHODS=[ccall,cwrap]"
    "-sASYNCIFY_IMPORTS=[emscripten_asm_const_int]"
    # The default heap is a fixed 16 MB, which virtual_list's 100,000 lines
    # outgrow.
    "-sALLOW_MEMORY_GROWTH=1"
  )
  string(TOLOWER "${CMAKE_BUILD_TYPE}" cmake_build_type_lower)
  if(cmake_build_type_lower MATCHES "release|relwithdebinfo")
    add_compile_options("-O3" "-flto")
    add_link_options("-O3" "-flto")
  endif()
endif()

# No explicit STATIC: the kind follows BUILD_SHARED_LIBS so a consumer or a
# distro can ask for a shared build. See the visibility settings and the
# generated export header below -- a shared build only exports what is marked.
add_library(rtxui
  "src/rtxui/terminal/terminal_input_parser.cpp"
  "src/rtxui/terminal/terminal_input_parser.hpp"
  "src/rtxui/terminal/event.cpp"
  "include/rtxui/event.hpp"
  "src/rtxui/terminal/screen.cpp"
  "include/rtxui/screen.hpp"
  "include/rtxui/task.hpp"
  "include/rtxui/diagnostic.hpp"
  "include/rtxui/headless.hpp"
  "include/rtxui/element.hpp"
  "src/rtxui/component/component.cpp"
  "include/rtxui/component.hpp"
  "include/rtxui/field_sink.hpp"
  "include/rtxui/internal/binding.hpp"
  "include/rtxui/internal/convert.hpp"
  "src/rtxui/component/default/button/button.cpp"
  "src/rtxui/component/default/checkbox/checkbox.cpp"
  "src/rtxui/component/default/label/label.cpp"
  "src/rtxui/component/default/tooltip/tooltip.cpp"
  "src/rtxui/component/default/details/details.cpp"
  "src/rtxui/component/default/summary/summary.cpp"
  "src/rtxui/component/default/dialog/dialog.cpp"
  "src/rtxui/component/default/toast/toast.cpp"
  "src/rtxui/component/default/tree_item/tree_item.cpp"
  "src/rtxui/component/default/fieldset/fieldset.cpp"
  "src/rtxui/component/default/legend/legend.cpp"
  "src/rtxui/component/default/radio/radio.cpp"
  "src/rtxui/component/default/tabs/tabs.cpp"
  "src/rtxui/component/default/tab_pane/tab_pane.cpp"
  "src/rtxui/component/default/div/div.cpp"
  "src/rtxui/component/default/h1/h1.cpp"
  "src/rtxui/component/default/hr/hr.cpp"
  "src/rtxui/component/default/i/i.cpp"
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
  "src/rtxui/component/default/a/a.cpp"
  "src/rtxui/component/default/blockquote/blockquote.cpp"
  "src/rtxui/component/default/strong/strong.cpp"
  "src/rtxui/component/default/textarea/textarea.cpp"
  "src/rtxui/component/default/markdown/markdown.cpp"
  "src/rtxui/component/default/ul/ul.cpp"
  "src/rtxui/component/default/u/u.cpp"
  "src/rtxui/component/default/s/s.cpp"
  "src/rtxui/component/default/code/code.cpp"
  "src/rtxui/component/default/kbd/kbd.cpp"
  "src/rtxui/component/default/semantic/semantic.cpp"
  "src/rtxui/component/default/pre/pre.cpp"
  "src/rtxui/base/diagnostic.cpp"
  "src/rtxui/base/expected.hpp"
  "src/rtxui/base/likely.hpp"
  "include/rtxui/refcounted.hpp"
  "src/rtxui/base/string.cpp"
  "src/rtxui/base/string.hpp"
  "src/rtxui/base/task.cpp"
  "src/rtxui/base/task.hpp"
  "src/rtxui/base/clock.cpp"
  "src/rtxui/base/task_queue.cpp"
  "src/rtxui/base/task_queue.hpp"
  "src/rtxui/base/task_runner.cpp"
  "src/rtxui/base/task_runner.hpp"
  "src/rtxui/dom/element.cpp"
  "src/rtxui/dom/element_handle.cpp"
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
  "src/rtxui/layout/length_expr.cpp"
  "src/rtxui/layout/layout_tree_builder.hpp"
  "src/rtxui/layout/physical_fragment.hpp"
  "src/rtxui/layout/style.hpp"
  "src/rtxui/paint/cell.cpp"
  "src/rtxui/paint/cell.hpp"
  "src/rtxui/paint/color.cpp"
  "include/rtxui/color.hpp"
  "src/rtxui/paint/glyph_fallback.cpp"
  "src/rtxui/paint/glyph_fallback.hpp"
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

target_compile_features(rtxui PUBLIC cxx_std_23)
if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
  target_compile_options(rtxui PRIVATE /W4)
else()
  target_compile_options(rtxui PRIVATE -Wall -Wextra)
endif()

# Everything is hidden unless marked RTXUI_EXPORT, so the shared library's ABI
# surface is the public API rather than every symbol that happens to be
# external. generate_export_header also defines RTXUI_STATIC_DEFINE for the
# static build, where the macro expands to nothing.
include(GenerateExportHeader)
set_target_properties(rtxui PROPERTIES
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
)
generate_export_header(rtxui
  BASE_NAME RTXUI
  EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/generated/rtxui/rtxui_export.hpp"
)
target_include_directories(rtxui PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
# Internal headers stay private: were src/ propagated, the examples could
# include what an installed consumer cannot, and nothing would notice.
target_include_directories(rtxui PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_include_directories(rtxui SYSTEM INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
  $<INSTALL_INTERFACE:include>
)

include(cmake/reflection.cmake)

add_library(rtxui::rtxui ALIAS rtxui)

# The Windows console's window and font are asked whether it has the glyphs
# RTXUI draws.
if(WIN32)
  target_link_libraries(rtxui PRIVATE gdi32 user32)
endif()

if(RTXUI_BUILD_EXAMPLES)
  add_subdirectory(example)
endif()

include(cmake/install.cmake)
if(PROJECT_IS_TOP_LEVEL)
  include(cmake/package.cmake)
endif()

option(RTXUI_BUILD_BENCHMARKS "Build benchmarks" OFF)
if(RTXUI_BUILD_BENCHMARKS)
  add_subdirectory(benchmarks)
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

    # Verify that examples, their build targets and the docs stay in sync
    add_test(NAME verify_examples
      COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/scripts/verify_examples.py"
    )

    # Verify that the CMake, Conan, vcpkg and Bazel files name the same version
    add_test(NAME verify_version
      COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/scripts/verify_version.py"
    )

    # Start every built example and check it draws a frame. The unit tests
    # never link an example nor run main(), so nothing else noticed when one
    # stopped starting.
    # smoke_examples drives each example through a PTY, which is POSIX.
    if(RTXUI_BUILD_EXAMPLES AND NOT WIN32)
      add_test(NAME smoke_examples
        COMMAND Python3::Interpreter
                "${CMAKE_CURRENT_SOURCE_DIR}/scripts/smoke_examples.py"
                "$<TARGET_FILE_DIR:rtxui_example_counter>"
      )
      # Each example gets a PTY, a second to draw, and a burst of input to
      # survive. Release finishes in about a minute; the sanitizer build takes
      # twice that, because the heaviest examples need ten seconds apiece just
      # to work through the burst.
      # RTXUI_STRICT aborts on the first diagnostic (unknown CSS, unbound
      # name, ...), so an example cannot teach a construct that does nothing.
      # RTXUI_VERIFY_LAYOUT_REUSE aborts when a frame that kept the last
      # layout draws anything a fresh one would not.
      set_tests_properties(smoke_examples PROPERTIES TIMEOUT 600
                           ENVIRONMENT "RTXUI_STRICT=1;RTXUI_VERIFY_LAYOUT_REUSE=1")
    endif()

    if(RTXUI_BUILD_EXAMPLES)
      # Compare what each example draws with example/snapshots/, so a
      # change that alters an interface shows up as a text diff.
      add_test(NAME snapshot_examples
        COMMAND Python3::Interpreter
                "${CMAKE_CURRENT_SOURCE_DIR}/scripts/snapshot_examples.py"
                "$<TARGET_FILE_DIR:rtxui_example_counter>"
      )
    endif()

    # Both checks below call the compiler with GCC-style flags.
    if(NOT MSVC)
      # Verify each public header compiles on its own. Most valuable where the
      # standard library differs from the one used day to day.
      add_test(NAME verify_headers
        COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/scripts/verify_headers.py"
                --cxx "${CMAKE_CXX_COMPILER}"
                --export-header-dir "${CMAKE_CURRENT_BINARY_DIR}/generated"
      )

      # Compile the C++ snippets of the docs against the public headers, so
      # the code readers copy keeps up with the API.
      add_test(NAME verify_doc_snippets
        COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/scripts/verify_doc_snippets.py"
                --cxx "${CMAKE_CXX_COMPILER}"
                --export-header-dir "${CMAKE_CURRENT_BINARY_DIR}/generated"
      )
    endif()
  endif()

  # Catch2
  #
  # FETCHCONTENT_QUIET defaults to ON, which discards the clone's output --
  # including the GIT_PROGRESS ticker below. A first configure then sits there
  # silently for as long as the download takes and looks hung. Turning it off
  # makes the clone report progress. This holds for the rest of the file, so
  # the fuzztest fetch further down reports progress too.
  set(FETCHCONTENT_QUIET OFF)
  FetchContent_Declare(catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2
    GIT_TAG v3.6.0
    EXCLUDE_FROM_ALL
    GIT_PROGRESS TRUE
    # The full history is ~33MB of objects for a pinned tag we never diff
    # against. Depth 1 fetches only v3.6.0's tree. This works because GIT_TAG
    # is a tag name: git cannot shallow-fetch a raw commit hash unless the
    # server opts in, so don't copy this to a SHA-pinned dependency.
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(catch2)

  add_executable(rtxui_test
    "src/rtxui/terminal/terminal_input_parser_test.cpp"
    "src/rtxui/terminal/terminal_capture_test.cpp"
    "src/rtxui/terminal/headless_test.cpp"
    "src/rtxui/terminal/inline_test.cpp"
    "src/rtxui/terminal/layout_reuse_test.cpp"
    "src/rtxui/terminal/screen_test.cpp"
    "src/rtxui/component/animation_test.cpp"
    "src/rtxui/component/binding_test.cpp"
    "src/rtxui/component/component_test.cpp"
    "src/rtxui/testing/render.cpp"
    "src/rtxui/component/default/checkbox/checkbox_test.cpp"
    "src/rtxui/component/default/default_components_test.cpp"
    "src/rtxui/component/default/details/details_test.cpp"
    "src/rtxui/component/default/fieldset/fieldset_test.cpp"
    "src/rtxui/component/default/dialog/dialog_test.cpp"
    "src/rtxui/component/default/input/input_test.cpp"
    "src/rtxui/component/default/label/label_test.cpp"
    "src/rtxui/component/default/markdown/markdown_test.cpp"
    "src/rtxui/component/default/pre/pre_test.cpp"
    "src/rtxui/component/default/select/select_test.cpp"
    "src/rtxui/component/default/slider/slider_test.cpp"
    "src/rtxui/component/default/tabs/tabs_test.cpp"
    "src/rtxui/component/default/textarea/textarea_test.cpp"
    "src/rtxui/component/default/tooltip/tooltip_test.cpp"
    "src/rtxui/layout/component_layout_test.cpp"
    "src/rtxui/paint/glyph_fallback_test.cpp"
    "src/rtxui/paint/text_paint_test.cpp"
    "src/rtxui/style/css_test.cpp"
    "src/rtxui/style/selector_test.cpp"
    "src/rtxui/component/diagnostic_test.cpp"
    "src/rtxui/component/generated_content_test.cpp"
    "src/rtxui/component/virtual_list_test.cpp"
    "src/rtxui/component/default/toast/toast_test.cpp"
    "src/rtxui/component/default/tree_item/tree_item_test.cpp"
    "src/rtxui/component/interpolation_test.cpp"
    "src/rtxui/base/expected_test.cpp"
    "src/rtxui/base/refcounted_test.cpp"
    "src/rtxui/dom/element_test.cpp"
    "src/rtxui/dom/element_handle_test.cpp"
    "src/rtxui/base/task_test.cpp"
    "src/rtxui/base/task_queue_test.cpp"
    "src/rtxui/base/task_runner_test.cpp"
    "src/rtxui/base/unicode_test.cpp"
    "src/rtxui/layout/layout_test.cpp"
    "src/rtxui/layout/rowspan_paint_test.cpp"
    "src/rtxui/paint/border_style_test.cpp"
    "src/rtxui/paint/outline_test.cpp"
    "src/rtxui/paint/hyperlink_test.cpp"
    "src/rtxui/paint/translate_test.cpp"
    "src/rtxui/terminal/timeline_screen_test.cpp"
    "src/rtxui/paint/scrollbar_color_test.cpp"
    "src/rtxui/paint/border_scrollbar_test.cpp"
    "src/rtxui/reflection/reflection_test.cpp"
    "src/rtxui/style/style_test.cpp"
    "src/rtxui/xml/xml_test.cpp"
    "src/rtxui/markdown/markdown_test.cpp"
  )
  # The unit tests exercise internals that hidden visibility deliberately keeps
  # out of the shared library's ABI. Link the objects straight into the test
  # binary rather than the shared object: linking both would give the process
  # two copies of the global component registry.
  if(BUILD_SHARED_LIBS)
    target_sources(rtxui_test PRIVATE $<TARGET_OBJECTS:rtxui>)
    target_include_directories(rtxui_test PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/include"
      "${CMAKE_CURRENT_BINARY_DIR}/generated"
    )
    target_compile_features(rtxui_test PRIVATE cxx_std_23)
    target_compile_definitions(rtxui_test PRIVATE RTXUI_STATIC_DEFINE)
  else()
    target_link_libraries(rtxui_test PRIVATE rtxui)
  endif()
  target_link_libraries(rtxui_test PRIVATE Catch2::Catch2WithMain)
  target_compile_definitions(rtxui_test PRIVATE
    RTXUI_TERMINAL_CAPTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/src/rtxui/terminal/captures")
  target_include_directories(rtxui_test PRIVATE "src")
  # Test files each define their own file-local helpers under the same names.
  # The library supports CMAKE_UNITY_BUILD; its tests don't need to.
  set_target_properties(rtxui_test PROPERTIES UNITY_BUILD OFF)
  list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
  include(CTest)
  include(Catch)
  catch_discover_tests(rtxui_test
    PROPERTIES SKIP_RETURN_CODE 4
               ENVIRONMENT RTXUI_VERIFY_LAYOUT_REUSE=1
  )

  # Prints what the parser makes of each key: a POSIX raw-mode tool.
  if(NOT WIN32)
    add_executable(rtxui_terminal_input_parser
      "src/rtxui/terminal/terminal_input_parser_main.cpp"
    )
    # A development tool driving an internal class: give it the objects rather
    # than exporting TerminalInputParser and widening the library's ABI for it.
    if(BUILD_SHARED_LIBS)
      target_sources(rtxui_terminal_input_parser PRIVATE $<TARGET_OBJECTS:rtxui>)
      target_include_directories(rtxui_terminal_input_parser PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_BINARY_DIR}/generated"
      )
      target_compile_features(rtxui_terminal_input_parser PRIVATE cxx_std_23)
      target_compile_definitions(rtxui_terminal_input_parser PRIVATE RTXUI_STATIC_DEFINE)
    else()
      target_link_libraries(rtxui_terminal_input_parser PRIVATE rtxui)
    endif()
    target_include_directories(rtxui_terminal_input_parser PRIVATE "src")
  endif()

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
    "src/rtxui/layout/layout_fuzzer.cpp"
    "src/rtxui/markdown/markdown_fuzzer.cpp"
    "src/rtxui/style/selector_fuzzer.cpp"
    "src/rtxui/style/style_fuzzer.cpp"
    "src/rtxui/terminal/app_fuzzer.cpp"
    "src/rtxui/terminal/event_fuzzer.cpp"
    "src/rtxui/terminal/terminal_input_parser_test_fuzzer.cpp"
    "src/rtxui/xml/xml_fuzzer.cpp"
  )
  include(GoogleTest)
  target_link_libraries(rtxui_fuzzer PRIVATE rtxui)
  target_include_directories(rtxui_fuzzer PRIVATE "src")
  link_fuzztest(rtxui_fuzzer)
  gtest_discover_tests(rtxui_fuzzer)

  # FUZZ test requires a definition of to run in fuzzing mode
  target_compile_definitions(rtxui_fuzzer PRIVATE FUZZTEST_FUZZING_ENABLED)

endif()

include(cmake/modules.cmake)
