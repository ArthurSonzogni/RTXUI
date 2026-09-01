# Toolchain-wide settings: sanitizers, language standard, build type, ccache.
#
# include()d rather than add_subdirectory()d, so the variables set here land in
# the root scope and the add_compile_options() below are inherited by every
# subdirectory added after the include.

if(RTXUI_SANITIZE)
  # float-cast-overflow is named explicitly because GCC's `undefined` group
  # leaves it out. Layout resolves lengths as floats and hands them to code
  # that counts cells in ints, so narrowing one that does not fit is exactly
  # the mistake this build should be catching -- and it went unreported until
  # the check was asked for by name.
  set(RTXUI_SANITIZERS address,undefined,float-cast-overflow)
  add_compile_options(-fsanitize=${RTXUI_SANITIZERS} -fno-omit-frame-pointer -g)
  add_link_options(-fsanitize=${RTXUI_SANITIZERS})
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# C++23 is the requirement. C++26 is optional: when the compiler offers
# reflection, RTXUI uses it to read struct fields in bound collections without
# a manual mapper, and the targets that need it are raised to 26 by
# cmake/reflection.cmake. The interface requirement stays at 23 so consumers
# are not dragged along.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the default build type to 'Release'
if (CMAKE_BUILD_TYPE STREQUAL "")
  set(CMAKE_BUILD_TYPE "Release")
endif()

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
endif()
