# Build RTXUI's own tests, examples and benchmarks with C++26 reflection when
# the compiler supports it, so the reflection code in the public headers is
# exercised.
#
# The library itself is left alone: its code never uses reflection, and
# <rtxui/component.hpp> detects it per translation unit (__cpp_impl_reflection).
# A C++23 build of the library then serves applications compiled with or
# without reflection, and an installed package asks nothing of them.
#
# include()d after the rtxui target is defined, not add_subdirectory()d: the
# flags below reach the targets defined after this point in this directory and
# its subdirectories, and never the rtxui target or a parent project.

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
    set(RTXUI_REFLECTION_FLAG "-freflection-latest")
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
    set(RTXUI_REFLECTION_FLAG "-freflection")
    message(STATUS "RTXUI: C++26 Reflection enabled (-freflection)")
  endif()
endif()
set(CMAKE_REQUIRED_FLAGS "${SAFE_CMAKE_REQUIRED_FLAGS}")
set(CMAKE_CXX_STANDARD "${SAFE_CMAKE_CXX_STANDARD}")

if(RTXUI_REFLECTION_FLAG)
  # Reflection is a C++26 feature: it does not compile under -std=c++23.
  set(CMAKE_CXX_STANDARD 26)
  add_compile_options("${RTXUI_REFLECTION_FLAG}")
endif()
