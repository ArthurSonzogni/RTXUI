# The `rtxui` C++ module (RTXUI_BUILD_MODULE). Experimental.
#
# Code that does `import rtxui;` links rtxui::module. The module exports the
# public API; the Bind() macros, which a module cannot export, come from
# <rtxui/bind.hpp>. See docs/guide/modules.md.

if(NOT RTXUI_BUILD_MODULE)
  return()
endif()

if(CMAKE_VERSION VERSION_LESS 3.28)
  message(FATAL_ERROR
    "RTXUI_BUILD_MODULE needs CMake 3.28 or newer (this is ${CMAKE_VERSION}).")
endif()
if(NOT CMAKE_GENERATOR MATCHES "Ninja|Visual Studio")
  message(FATAL_ERROR
    "RTXUI_BUILD_MODULE needs the Ninja or Visual Studio generator; "
    "${CMAKE_GENERATOR} cannot build C++ modules.")
endif()

add_library(rtxui_module)
target_sources(rtxui_module
  PUBLIC FILE_SET CXX_MODULES FILES
  src/rtxui/rtxui.cppm
)
target_link_libraries(rtxui_module PUBLIC rtxui)
target_compile_features(rtxui_module PUBLIC cxx_std_23)
add_library(rtxui::module ALIAS rtxui_module)

# Every example, rewritten to `import rtxui;` by
# scripts/modularize_example.py, must still draw its snapshot.
if(NOT RTXUI_BUILD_EXAMPLES)
  return()
endif()
find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(module_examples_dir "${CMAKE_BINARY_DIR}/module_examples")
file(GLOB example_sources CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/example/*.cpp")
foreach(source ${example_sources})
  get_filename_component(name "${source}" NAME_WE)
  set(converted "${module_examples_dir}/src/${name}.cpp")
  add_custom_command(
    OUTPUT "${converted}"
    COMMAND Python3::Interpreter
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/modularize_example.py"
            "${source}" "${converted}"
    DEPENDS "${source}"
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/modularize_example.py"
    VERBATIM
  )
  add_executable(rtxui_module_example_${name} "${converted}")
  target_link_libraries(rtxui_module_example_${name} PRIVATE rtxui::module)
  # The same file name as the header-based build, in its own directory, so
  # scripts/snapshot_examples.py compares it with the same snapshot.
  # CXX_SCAN_FOR_MODULES: the project asks for CMake 3.24, which leaves
  # module scanning off for targets that have no module of their own.
  set_target_properties(rtxui_module_example_${name} PROPERTIES
    OUTPUT_NAME "rtxui_example_${name}"
    RUNTIME_OUTPUT_DIRECTORY "${module_examples_dir}"
    CXX_SCAN_FOR_MODULES ON
  )
endforeach()

if(RTXUI_BUILD_TESTS)
  add_test(NAME snapshot_module_examples
    COMMAND Python3::Interpreter
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/snapshot_examples.py"
            "${module_examples_dir}"
  )
endif()
