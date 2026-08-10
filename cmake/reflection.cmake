# Enable experimental C++26 reflection if the compiler supports it.
#
# Separate from cmake/compiler.cmake because this one touches rtxui_lib, so it
# has to be included after the target exists. include()d, not
# add_subdirectory()d: RTXUI_REFLECTION_STANDARD and the add_definitions() below
# have to reach the root scope and the subdirectories added after it.

include(CheckCXXSourceCompiles)
set(SAFE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
set(CMAKE_REQUIRED_FLAGS "-std=c++26")

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(CMAKE_REQUIRED_FLAGS "-std=c++26 -freflection-latest")
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
    add_definitions(-DRTXUI_HAS_REFLECTION)
    # Reflection is a C++26 feature: compiling it under -std=c++23 would not
    # work, so raise the standard for the targets that use it.
    set(RTXUI_REFLECTION_STANDARD 26)
    message(STATUS "RTXUI: C++26 Reflection enabled (-freflection-latest)")
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(CMAKE_REQUIRED_FLAGS "-std=c++26 -freflection")
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
    add_definitions(-DRTXUI_HAS_REFLECTION)
    set(RTXUI_REFLECTION_STANDARD 26)
    message(STATUS "RTXUI: C++26 Reflection enabled (-freflection)")
  endif()
endif()
set(CMAKE_REQUIRED_FLAGS "${SAFE_CMAKE_REQUIRED_FLAGS}")

if(RTXUI_REFLECTION_STANDARD)
  set(CMAKE_CXX_STANDARD ${RTXUI_REFLECTION_STANDARD})
  set_target_properties(rtxui_lib PROPERTIES CXX_STANDARD ${RTXUI_REFLECTION_STANDARD})
endif()
