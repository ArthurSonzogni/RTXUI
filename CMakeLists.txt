cmake_minimum_required(VERSION 3.24)

project(rtxui
  LANGUAGES CXX
  VERSION 0.1.0
  DESCRIPTION "C++ reactive terminal renderering"
)

option(RTXUI_BUILD_TESTS "Build tests" OFF)
option(RTXUI_BUILD_EXAMPLES "Build examples" OFF)
option(RTXUI_BUILD_FUZZERS "Build fuzzers" OFF)
option(RTXUI_SANITIZE "Build with Address and UndefinedBehavior sanitizers" OFF)

include(FetchContent)
include(cmake/compiler.cmake)

# No explicit STATIC: the kind follows BUILD_SHARED_LIBS so a consumer or a
# distro can ask for a shared build. See the visibility settings and the
# generated export header below -- a shared build only exports what is marked.
add_library(rtxui_lib
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
  "src/rtxui/component/default/label/label.cpp"
  "src/rtxui/component/default/tooltip/tooltip.cpp"
  "src/rtxui/component/default/details/details.cpp"
  "src/rtxui/component/default/summary/summary.cpp"
  "src/rtxui/component/default/dialog/dialog.cpp"
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
  "src/rtxui/component/default/pre/pre.cpp"
  "include/rtxui/internal/import.hpp"
  "src/rtxui/base/expected.hpp"
  "src/rtxui/base/likely.hpp"
  "include/rtxui/internal/refcounted.hpp"
  "src/rtxui/base/string.cpp"
  "src/rtxui/base/string.hpp"
  "src/rtxui/base/task.cpp"
  "src/rtxui/base/task.hpp"
  "src/rtxui/base/task_queue.cpp"
  "src/rtxui/base/task_queue.hpp"
  "src/rtxui/base/task_runner.cpp"
  "src/rtxui/base/task_runner.hpp"
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
  "src/rtxui/layout/length_expr.cpp"
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

target_compile_features(rtxui_lib PUBLIC cxx_std_23)
target_compile_options(rtxui_lib PRIVATE
  -Wall
  -Wextra
  -Wno-missing-field-initializers
  -Wno-unused-local-typedefs
  -Wno-unused-parameter
  -Wno-sign-compare
  -Wno-unused-variable
  -Wno-unused-function
)

# Everything is hidden unless marked RTXUI_EXPORT, so the shared library's ABI
# surface is the public API rather than every symbol that happens to be
# external. generate_export_header also defines RTXUI_STATIC_DEFINE for the
# static build, where the macro expands to nothing.
include(GenerateExportHeader)
set_target_properties(rtxui_lib PROPERTIES
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
)
generate_export_header(rtxui_lib
  BASE_NAME RTXUI
  EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/generated/rtxui/rtxui_export.hpp"
)
target_include_directories(rtxui_lib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
  $<INSTALL_INTERFACE:include>
)
target_include_directories(rtxui_lib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<INSTALL_INTERFACE:include>
)

if(EMSCRIPTEN)
  add_compile_options("-sASYNCIFY")
  add_link_options(
    "-sASYNCIFY"
    "-sEXPORTED_RUNTIME_METHODS=[ccall,cwrap]"
    "-sASYNCIFY_IMPORTS=[emscripten_asm_const_int]"
  )
endif()

include(cmake/reflection.cmake)

# See https://stackoverflow.com/questions/53071878/using-whole-archive-linker-option-with-cmake-and-libraries-with-other-library
add_library(rtxui "src/rtxui/dummy.cpp")

# The namespaced alias matters for the installed package: install(EXPORT)
# rewrites plain target names in INTERFACE_LINK_LIBRARIES to their namespaced
# form, but not names sitting inside a $<LINK_LIBRARY:...> expression. Exported
# as bare `rtxui_lib` it resolved to no target in a consumer's project, so it
# degraded to a raw -lrtxui_lib and rtxui_lib's include directories never
# propagated -- consumers got "rtxui/rtxui.hpp: No such file or directory".
add_library(rtxui::rtxui_lib ALIAS rtxui_lib)
if(BUILD_SHARED_LIBS)
  # A shared object runs its static initialisers on load, so the built-in
  # components register themselves without the archive trick below.
  target_link_libraries(rtxui PUBLIC rtxui::rtxui_lib)
else()
  target_link_libraries(rtxui PUBLIC "$<LINK_LIBRARY:WHOLE_ARCHIVE,rtxui::rtxui_lib>")
endif()

if(RTXUI_BUILD_EXAMPLES)
  add_subdirectory(example)
endif()

include(cmake/install.cmake)

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

    # Start every built example and check it draws a frame. The unit tests
    # never link an example nor run main(), so nothing else noticed when one
    # stopped starting.
    if(RTXUI_BUILD_EXAMPLES)
      add_test(NAME smoke_examples
        COMMAND Python3::Interpreter
                "${CMAKE_CURRENT_SOURCE_DIR}/scripts/smoke_examples.py"
                "$<TARGET_FILE_DIR:rtxui_example_demo>"
      )
      # Each example gets a PTY, a second to draw, and a burst of input to
      # survive. Release finishes in about a minute; the sanitizer build takes
      # twice that, because the heaviest examples need ten seconds apiece just
      # to work through the burst.
      set_tests_properties(smoke_examples PROPERTIES TIMEOUT 600)
    endif()

    # Verify each public header compiles on its own. Most valuable where the
    # standard library differs from the one used day to day.
    add_test(NAME verify_headers
      COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/scripts/verify_headers.py"
              --cxx "${CMAKE_CXX_COMPILER}"
              --export-header-dir "${CMAKE_CURRENT_BINARY_DIR}/generated"
    )
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
    "src/rtxui/terminal/screen_test.cpp"
    "src/rtxui/component/component_test.cpp"
    "src/rtxui/component/interpolation_test.cpp"
    "src/rtxui/base/expected_test.cpp"
    "src/rtxui/base/refcounted_test.cpp"
    "src/rtxui/dom/element_test.cpp"
    "src/rtxui/base/task_test.cpp"
    "src/rtxui/base/task_queue_test.cpp"
    "src/rtxui/base/task_runner_test.cpp"
    "src/rtxui/base/unicode_test.cpp"
    "src/rtxui/layout/layout_test.cpp"
    "src/rtxui/layout/rowspan_paint_test.cpp"
    "src/rtxui/paint/border_style_test.cpp"
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
    target_sources(rtxui_test PRIVATE $<TARGET_OBJECTS:rtxui_lib>)
    target_include_directories(rtxui_test PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/include"
      "${CMAKE_CURRENT_BINARY_DIR}/generated"
    )
    target_compile_features(rtxui_test PRIVATE cxx_std_23)
    target_compile_definitions(rtxui_test PRIVATE RTXUI_STATIC_DEFINE)
    if(HAS_REFLECTION_CLANG)
      target_compile_options(rtxui_test PRIVATE "-freflection-latest")
    elseif(HAS_REFLECTION_GCC)
      target_compile_options(rtxui_test PRIVATE "-freflection")
    endif()
  else()
    target_link_libraries(rtxui_test PRIVATE rtxui_lib)
  endif()
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
  # A development tool driving an internal class: give it the objects rather
  # than exporting TerminalInputParser and widening the library's ABI for it.
  if(BUILD_SHARED_LIBS)
    target_sources(rtxui_terminal_input_parser PRIVATE $<TARGET_OBJECTS:rtxui_lib>)
    target_include_directories(rtxui_terminal_input_parser PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/include"
      "${CMAKE_CURRENT_BINARY_DIR}/generated"
    )
    target_compile_features(rtxui_terminal_input_parser PRIVATE cxx_std_23)
    target_compile_definitions(rtxui_terminal_input_parser PRIVATE RTXUI_STATIC_DEFINE)
  else()
    target_link_libraries(rtxui_terminal_input_parser PRIVATE rtxui_lib)
  endif()
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
    "src/rtxui/layout/layout_fuzzer.cpp"
    "src/rtxui/markdown/markdown_fuzzer.cpp"
    "src/rtxui/style/selector_fuzzer.cpp"
    "src/rtxui/style/style_fuzzer.cpp"
    "src/rtxui/terminal/event_fuzzer.cpp"
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
