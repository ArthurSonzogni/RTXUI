# Examples. Added from the root only when RTXUI_BUILD_EXAMPLES is on, so this
# file needs no guard of its own.
#
# The binaries stay in the top of the build tree rather than build/example/:
# docs and examples both point at build/rtxui_example_<name>.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

add_executable(rtxui_example_demo demo.cpp)
target_link_libraries(rtxui_example_demo PRIVATE rtxui)

add_executable(rtxui_example_counter counter.cpp)
target_link_libraries(rtxui_example_counter PRIVATE rtxui)

add_executable(rtxui_example_layout layout.cpp)
target_link_libraries(rtxui_example_layout PRIVATE rtxui)

add_executable(rtxui_example_layout_flex layout_flex.cpp)
target_link_libraries(rtxui_example_layout_flex PRIVATE rtxui)

add_executable(rtxui_example_borders borders.cpp)
target_link_libraries(rtxui_example_borders PRIVATE rtxui)

add_executable(rtxui_example_border_scroll_demo border_scroll_demo.cpp)
target_link_libraries(rtxui_example_border_scroll_demo PRIVATE rtxui)

add_executable(rtxui_example_nested_scroll nested_scroll.cpp)
target_link_libraries(rtxui_example_nested_scroll PRIVATE rtxui)

add_executable(rtxui_example_horizontal_scroll horizontal_scroll.cpp)
target_link_libraries(rtxui_example_horizontal_scroll PRIVATE rtxui)

add_executable(rtxui_example_scroll_behavior scroll_behavior.cpp)
target_link_libraries(rtxui_example_scroll_behavior PRIVATE rtxui)

add_executable(rtxui_example_helloworld helloworld.cpp)
target_link_libraries(rtxui_example_helloworld PRIVATE rtxui)

add_executable(rtxui_example_loop_simple loop_simple.cpp)
target_link_libraries(rtxui_example_loop_simple PRIVATE rtxui)

add_executable(rtxui_example_loop_complex loop_complex.cpp)
target_link_libraries(rtxui_example_loop_complex PRIVATE rtxui)

add_executable(rtxui_example_conditional conditional.cpp)
target_link_libraries(rtxui_example_conditional PRIVATE rtxui)

add_executable(rtxui_example_colors colors.cpp)
target_link_libraries(rtxui_example_colors PRIVATE rtxui)

add_executable(rtxui_example_cjk cjk.cpp)
target_link_libraries(rtxui_example_cjk PRIVATE rtxui)

add_executable(rtxui_example_text_align text_align.cpp)
target_link_libraries(rtxui_example_text_align PRIVATE rtxui)

add_executable(rtxui_example_text_decoration text_decoration.cpp)
target_link_libraries(rtxui_example_text_decoration PRIVATE rtxui)

add_executable(rtxui_example_input input.cpp)
target_link_libraries(rtxui_example_input PRIVATE rtxui)

add_executable(rtxui_example_textarea textarea.cpp)
target_link_libraries(rtxui_example_textarea PRIVATE rtxui)

add_executable(rtxui_example_checkbox checkbox.cpp)
target_link_libraries(rtxui_example_checkbox PRIVATE rtxui)

add_executable(rtxui_example_label label.cpp)
target_link_libraries(rtxui_example_label PRIVATE rtxui)

add_executable(rtxui_example_tooltip tooltip.cpp)
target_link_libraries(rtxui_example_tooltip PRIVATE rtxui)

add_executable(rtxui_example_slider slider.cpp)
target_link_libraries(rtxui_example_slider PRIVATE rtxui)

add_executable(rtxui_example_progress progress.cpp)
target_link_libraries(rtxui_example_progress PRIVATE rtxui)

add_executable(rtxui_example_details details.cpp)
target_link_libraries(rtxui_example_details PRIVATE rtxui)

add_executable(rtxui_example_fieldset fieldset.cpp)
target_link_libraries(rtxui_example_fieldset PRIVATE rtxui)

add_executable(rtxui_example_radio radio.cpp)
target_link_libraries(rtxui_example_radio PRIVATE rtxui)

add_executable(rtxui_example_tabs tabs.cpp)
target_link_libraries(rtxui_example_tabs PRIVATE rtxui)

add_executable(rtxui_example_dialog dialog.cpp)
target_link_libraries(rtxui_example_dialog PRIVATE rtxui)

add_executable(rtxui_example_select select.cpp)
target_link_libraries(rtxui_example_select PRIVATE rtxui)

add_executable(rtxui_example_hr hr.cpp)
target_link_libraries(rtxui_example_hr PRIVATE rtxui)

add_executable(rtxui_example_pseudo_classes pseudo_classes.cpp)
target_link_libraries(rtxui_example_pseudo_classes PRIVATE rtxui)

add_executable(rtxui_example_transitions transitions.cpp)
target_link_libraries(rtxui_example_transitions PRIVATE rtxui)

add_executable(rtxui_example_animation animation.cpp)
target_link_libraries(rtxui_example_animation PRIVATE rtxui)

add_executable(rtxui_example_positioning positioning.cpp)
target_link_libraries(rtxui_example_positioning PRIVATE rtxui)

add_executable(rtxui_example_sticky sticky.cpp)
target_link_libraries(rtxui_example_sticky PRIVATE rtxui)

add_executable(rtxui_example_anchor anchor.cpp)
target_link_libraries(rtxui_example_anchor PRIVATE rtxui)

add_executable(rtxui_example_tabindex tabindex.cpp)
target_link_libraries(rtxui_example_tabindex PRIVATE rtxui)

add_executable(rtxui_example_media media.cpp)
target_link_libraries(rtxui_example_media PRIVATE rtxui)

add_executable(rtxui_example_focus_scroll focus_scroll.cpp)
target_link_libraries(rtxui_example_focus_scroll PRIVATE rtxui_lib)

add_executable(rtxui_example_markdown markdown.cpp)
target_link_libraries(rtxui_example_markdown PRIVATE rtxui_lib)

add_executable(rtxui_example_opacity opacity.cpp)
target_link_libraries(rtxui_example_opacity PRIVATE rtxui)

add_executable(rtxui_example_table table.cpp)
target_link_libraries(rtxui_example_table PRIVATE rtxui)

add_executable(rtxui_example_grid grid.cpp)
target_link_libraries(rtxui_example_grid PRIVATE rtxui)

add_executable(rtxui_example_lists lists.cpp)
target_link_libraries(rtxui_example_lists PRIVATE rtxui)

add_executable(rtxui_example_slots slots.cpp)
target_link_libraries(rtxui_example_slots PRIVATE rtxui)

add_executable(rtxui_example_spatial_navigation spatial_navigation.cpp)
target_link_libraries(rtxui_example_spatial_navigation PRIVATE rtxui)

add_executable(rtxui_example_cookbook_tabs cookbook_tabs.cpp)
target_link_libraries(rtxui_example_cookbook_tabs PRIVATE rtxui)

add_executable(rtxui_example_cookbook_async cookbook_async.cpp)
target_link_libraries(rtxui_example_cookbook_async PRIVATE rtxui)

add_executable(rtxui_example_cookbook_dialog cookbook_dialog.cpp)
target_link_libraries(rtxui_example_cookbook_dialog PRIVATE rtxui)

add_executable(rtxui_example_app_filebrowser app_filebrowser.cpp)
target_link_libraries(rtxui_example_app_filebrowser PRIVATE rtxui)

add_executable(rtxui_example_app_dashboard app_dashboard.cpp)
target_link_libraries(rtxui_example_app_dashboard PRIVATE rtxui)

add_executable(rtxui_example_playground playground.cpp)
target_link_libraries(rtxui_example_playground PRIVATE rtxui)