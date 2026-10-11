if (NOT RTXUI_BUILD_MODULE)
  return()
endif()

add_library(rtxui_module)

target_sources(rtxui_module
  PUBLIC FILE_SET CXX_MODULES FILES
  src/rtxui/rtxui.cppm
  )

target_link_libraries(rtxui_module
  PUBLIC
  rtxui_lib
  )

target_compile_features(rtxui_module PUBLIC cxx_std_20)
# TODO: Explain why this is needed.
if (CMAKE_COMPILER_IS_GNUCXX)
  target_compile_options(rtxui_module PUBLIC -fmodules-ts)
endif ()

add_library(rtxui::module ALIAS rtxui_module)
