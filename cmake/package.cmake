# Binary packages, made by CPack from what `cmake --install` installs.
#
# The release workflow builds:
# - an archive per platform (.tar.gz, or .zip on Windows) holding the Debug
#   and Release libraries: Ninja Multi-Config, then
#   `cpack -G TGZ -C "Debug;Release"`. Debug goes first: rtxui.pc names the
#   library of the last configuration installed.
# - a .deb and an .rpm holding the Release library, as distribution packages
#   do, each from a tree configured with CMAKE_INSTALL_PREFIX=/usr and the
#   distribution's library directory.
#
# Only when RTXUI is the top-level project: a project fetching RTXUI makes its
# own packages.

# rtxui-<version>-<os>-<arch>, e.g. rtxui-0.1.1-linux-x64.
if(APPLE)
  set(RTXUI_PACKAGE_OS macos)
else()
  string(TOLOWER "${CMAKE_SYSTEM_NAME}" RTXUI_PACKAGE_OS)
endif()
list(LENGTH CMAKE_OSX_ARCHITECTURES RTXUI_OSX_ARCHITECTURE_COUNT)
if(RTXUI_OSX_ARCHITECTURE_COUNT GREATER 1)
  set(RTXUI_PACKAGE_ARCH universal)
elseif(CMAKE_OSX_ARCHITECTURES)
  set(RTXUI_PACKAGE_ARCH "${CMAKE_OSX_ARCHITECTURES}")
else()
  string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" RTXUI_PACKAGE_ARCH)
endif()
if(RTXUI_PACKAGE_ARCH MATCHES "^(x86_64|amd64)$")
  set(RTXUI_PACKAGE_ARCH x64)
elseif(RTXUI_PACKAGE_ARCH STREQUAL "aarch64")
  set(RTXUI_PACKAGE_ARCH arm64)
endif()

set(CPACK_PACKAGE_NAME rtxui)
set(CPACK_PACKAGE_VENDOR "Arthur Sonzogni")
set(CPACK_PACKAGE_CONTACT "Arthur Sonzogni <sonzogniarthur@gmail.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "C++ terminal UI library: HTML templates, CSS and reactive state")
set(CPACK_PACKAGE_DESCRIPTION
    "RTXUI builds terminal user interfaces the way a browser builds pages: \
components declare HTML templates, style them with CSS (flexbox, grid, \
transitions) and bind C++ members into them. Static library, headers, CMake \
package and pkg-config file. Needs a C++23 compiler.")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/ArthurSonzogni/RTXUI")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_FILE_NAME
    "rtxui-${PROJECT_VERSION}-${RTXUI_PACKAGE_OS}-${RTXUI_PACKAGE_ARCH}")
set(CPACK_STRIP_FILES OFF)
if(WIN32)
  set(CPACK_GENERATOR ZIP)
elseif(APPLE)
  set(CPACK_GENERATOR TGZ)
else()
  set(CPACK_GENERATOR TGZ DEB RPM)
endif()

# Debian: librtxui-dev_<version>_<arch>.deb.
set(CPACK_DEBIAN_PACKAGE_NAME librtxui-dev)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_SECTION libdevel)
set(CPACK_DEBIAN_PACKAGE_PRIORITY optional)

# Fedora and other RPM distributions: rtxui-devel-<version>-1.<arch>.rpm.
set(CPACK_RPM_PACKAGE_NAME rtxui-devel)
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
set(CPACK_RPM_PACKAGE_LICENSE MIT)
set(CPACK_RPM_PACKAGE_GROUP "Development/Libraries")
set(CPACK_RPM_PACKAGE_URL "${CPACK_PACKAGE_HOMEPAGE_URL}")
set(CPACK_RPM_PACKAGE_DESCRIPTION "${CPACK_PACKAGE_DESCRIPTION}")
# Directories other packages own (cmake-filesystem, pkgconf): listing them
# here too would make the two packages conflict.
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
    /usr/lib/cmake /usr/lib/pkgconfig /usr/lib64/cmake /usr/lib64/pkgconfig)

include(CPack)
