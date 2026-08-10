# Benchmarks. Added from the root only when RTXUI_BUILD_BENCHMARKS is on.
# Output goes to the build root, matching the examples.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

add_executable(rtxui_benchmark rtxui_benchmark.cpp)
target_link_libraries(rtxui_benchmark PRIVATE rtxui)

add_executable(rtxui_layout_benchmark rtxui_layout_benchmark.cpp)
target_link_libraries(rtxui_layout_benchmark PRIVATE rtxui)

add_executable(rtxui_steady_state_benchmark rtxui_steady_state_benchmark.cpp)
target_link_libraries(rtxui_steady_state_benchmark PRIVATE rtxui)

add_executable(rtxui_xml_benchmark rtxui_xml_benchmark.cpp)
target_link_libraries(rtxui_xml_benchmark PRIVATE rtxui)