# Copyright 2026 Arthur Sonzogni. All rights reserved.
# Use of this source code is governed by the MIT license that can be found in
# the LICENSE file.
#
# Overlay port for RTXUI.
#
# This overlay port builds the local checkout it ships inside, which works out
# of the box for local consuming projects.
#
# To submit this port to the official microsoft/vcpkg curated registry, replace
# the SOURCE_PATH block below with:
#
#   vcpkg_from_github(
#     OUT_SOURCE_PATH SOURCE_PATH
#     REPO ArthurSonzogni/RTXUI
#     REF "v${VERSION}"
#     SHA512 <sha512 of the release tarball>
#     HEAD_REF main
#   )
#
# and the same port can be submitted to microsoft/vcpkg unchanged otherwise.

# The port lives at <repo>/packaging/vcpkg/rtxui, so the repository root is
# three directories up.
get_filename_component(SOURCE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)

if(NOT EXISTS "${SOURCE_PATH}/CMakeLists.txt")
  message(FATAL_ERROR
    "Expected the RTXUI sources at ${SOURCE_PATH}. This overlay port builds "
    "the repository it is shipped inside; keep it at packaging/vcpkg/rtxui.")
endif()


vcpkg_cmake_configure(
  SOURCE_PATH "${SOURCE_PATH}"
  OPTIONS
    -DRTXUI_BUILD_TESTS=OFF
    -DRTXUI_BUILD_EXAMPLES=OFF
    -DRTXUI_BUILD_FUZZERS=OFF
    -DRTXUI_BUILD_BENCHMARKS=OFF
)

vcpkg_cmake_install()

# RTXUI installs its config as `rtxui-config.cmake` alongside the targets file
# under lib/cmake/rtxui.
vcpkg_cmake_config_fixup(PACKAGE_NAME rtxui CONFIG_PATH lib/cmake/rtxui)

vcpkg_copy_pdbs()

# A header-consuming package keeps only the headers and the CMake config from
# the debug tree; the rest would collide with the release install.
file(REMOVE_RECURSE
  "${CURRENT_PACKAGES_DIR}/debug/include"
  "${CURRENT_PACKAGES_DIR}/debug/share"
)

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
