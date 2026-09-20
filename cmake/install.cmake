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
set_target_properties(rtxui rtxui_lib PROPERTIES DEBUG_POSTFIX d)

# SOVERSION is what lets an ABI break be expressed at all: without it the
# SONAME is a bare librtxui_lib.so, two incompatible builds cannot coexist,
# and a consumer linked against the old one loads the new one silently. The
# major version is the ABI generation, matching the inline namespace in the
# headers.
set_target_properties(rtxui rtxui_lib PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
)

install(TARGETS rtxui rtxui_lib
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
