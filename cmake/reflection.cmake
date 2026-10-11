# Enable experimental C++26 reflection if the compiler supports it.
#
# Separate from cmake/compiler.cmake because this one touches rtxui_lib, so it
# has to be included after the target exists. include()d, not
# add_subdirectory()d: RTXUI_REFLECTION_STANDARD has to reach the root scope
# and the subdirectories added after it.
#
# The flag, the RTXUI_HAS_REFLECTION define and C++26 are all PUBLIC on
# rtxui_lib: the public headers use reflection in templates the application
# instantiates, so the application must compile them the way the library did.
# A define limited to this build left an application on the other branch of
# those templates: two definitions of one template in a single program.

include(CheckCXXSourceCompiles)
set(SAFE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
# The probes compile with the project's CMAKE_CXX_STANDARD, whose -std= comes
# after CMAKE_REQUIRED_FLAGS and wins. Raise it for them: under the project's
# C++23 every probe failed, and reflection was never enabled at all.
set(SAFE_CMAKE_CXX_STANDARD "${CMAKE_CXX_STANDARD}")
set(CMAKE_CXX_STANDARD 26)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(CMAKE_REQUIRED_FLAGS "-freflection-latest")
  check_cxx_source_compiles("
    #include <meta>
    struct S { int x; };
    int main() {
      constexpr auto m = std::meta::nonstatic_data_members_of(^^S, std::meta::access_context::unchecked()).size();
      return 0;
    }
  " HAS_REFLECTION_CLANG)
  if(HAS_REFLECTION_CLANG)
    target_compile_options(rtxui_lib PUBLIC "-freflection-latest")
    target_compile_definitions(rtxui_lib PUBLIC RTXUI_HAS_REFLECTION)
    # Reflection is a C++26 feature: compiling it under -std=c++23 would not
    # work, so raise the standard for the targets that use it.
    set(RTXUI_REFLECTION_STANDARD 26)
    message(STATUS "RTXUI: C++26 Reflection enabled (-freflection-latest)")
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(CMAKE_REQUIRED_FLAGS "-freflection")
  check_cxx_source_compiles("
    #include <meta>
    struct S { int x; };
    int main() {
      constexpr auto m = std::meta::nonstatic_data_members_of(^^S, std::meta::access_context::unchecked()).size();
      return 0;
    }
  " HAS_REFLECTION_GCC)
  if(HAS_REFLECTION_GCC)
    target_compile_options(rtxui_lib PUBLIC "-freflection")
    target_compile_definitions(rtxui_lib PUBLIC RTXUI_HAS_REFLECTION)
    set(RTXUI_REFLECTION_STANDARD 26)
    message(STATUS "RTXUI: C++26 Reflection enabled (-freflection)")
  endif()
endif()
set(CMAKE_REQUIRED_FLAGS "${SAFE_CMAKE_REQUIRED_FLAGS}")
set(CMAKE_CXX_STANDARD "${SAFE_CMAKE_CXX_STANDARD}")

if(RTXUI_REFLECTION_STANDARD)
  set(CMAKE_CXX_STANDARD ${RTXUI_REFLECTION_STANDARD})
  set_target_properties(rtxui_lib PROPERTIES CXX_STANDARD ${RTXUI_REFLECTION_STANDARD})
  target_compile_features(rtxui_lib PUBLIC cxx_std_26)
endif()
