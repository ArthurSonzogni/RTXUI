# Install rules, the exported target set and the find_package() config package.
#
# include()d, not add_subdirectory()d: the install(FILES ...) calls below name
# headers by ${CMAKE_CURRENT_SOURCE_DIR}, which include() leaves pointing at the
# repository root, so the paths are the same ones the root file used.

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# A debug build gets a `d` suffix so Debug and Release can share one install
# prefix. The alternative -- a per-config subdirectory -- keeps them apart too,
# but puts the archives in lib/<Config>/ instead of lib/, which vcpkg, distro
# packagers and a plain find_package() from a normal prefix do not expect.
set_target_properties(rtxui PROPERTIES DEBUG_POSTFIX d)

# SOVERSION is what lets an ABI break be expressed at all: without it the
# SONAME is a bare librtxui.so, two incompatible builds cannot coexist,
# and a consumer linked against the old one loads the new one silently. The
# major version is the ABI generation, matching the inline namespace in the
# headers. Before 1.0, any minor release may break the ABI, so 0.x carries the
# minor version too: librtxui.so.0.1, not librtxui.so.0.
if(PROJECT_VERSION_MAJOR EQUAL 0)
  set(RTXUI_SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})
else()
  set(RTXUI_SOVERSION ${PROJECT_VERSION_MAJOR})
endif()
set_target_properties(rtxui PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${RTXUI_SOVERSION}
)

install(TARGETS rtxui
  EXPORT rtxui-targets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/generated/rtxui/rtxui_export.hpp"
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/rtxui
)

# The MIT license asks for its notice to travel with every copy.
install(FILES LICENSE
  DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/rtxui
)



install(EXPORT rtxui-targets
  FILE rtxui-targets.cmake
  NAMESPACE rtxui::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/rtxui
)

configure_package_config_file(
  cmake/rtxui-config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/rtxui-config.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/rtxui
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/rtxui-config-version.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/rtxui-config.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/rtxui-config-version.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/rtxui
)

# pkg-config, for Meson, Makefiles and distro packaging. The library name goes
# through file(GENERATE) to pick up the Debug `d` suffix.
#
# The prefix is found from where rtxui.pc is, rather than written in: a
# package is unpacked or installed somewhere else than the build's
# CMAKE_INSTALL_PREFIX.
if(IS_ABSOLUTE "${CMAKE_INSTALL_LIBDIR}")
  set(RTXUI_PC_PREFIX "${CMAKE_INSTALL_PREFIX}")
else()
  file(RELATIVE_PATH RTXUI_PC_UP "/root/${CMAKE_INSTALL_LIBDIR}/pkgconfig" "/root")
  string(REGEX REPLACE "/$" "" RTXUI_PC_UP "${RTXUI_PC_UP}")
  set(RTXUI_PC_PREFIX "\${pcfiledir}/${RTXUI_PC_UP}")
endif()
foreach(dir LIBDIR INCLUDEDIR)
  if(IS_ABSOLUTE "${CMAKE_INSTALL_${dir}}")
    set(RTXUI_PC_${dir} "${CMAKE_INSTALL_${dir}}")
  else()
    set(RTXUI_PC_${dir} "\${prefix}/${CMAKE_INSTALL_${dir}}")
  endif()
endforeach()
configure_file(cmake/rtxui.pc.in "${CMAKE_CURRENT_BINARY_DIR}/rtxui.pc.in" @ONLY)
file(GENERATE
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/$<CONFIG>/rtxui.pc"
  INPUT "${CMAKE_CURRENT_BINARY_DIR}/rtxui.pc.in"
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/$<CONFIG>/rtxui.pc"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
