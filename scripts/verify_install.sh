#!/usr/bin/env bash
# Builds and installs RTXUI, then builds the same small app against it the
# three ways a user can: find_package(), pkg-config and FetchContent. Each app
# must draw the built-in <button> and <checkbox>, which a library missing its
# built-in tags would draw as plain text.
#
#   scripts/verify_install.sh [extra CMake arguments for the library]
#   scripts/verify_install.sh -DBUILD_SHARED_LIBS=ON
#
# Uses $CXX if set. Works in a temporary directory, removed on exit.
set -euo pipefail

root=$(cd "$(dirname "$0")/.." && pwd)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
prefix="$work/prefix"

step() { echo "== $*"; }

step "Install RTXUI to $prefix"
cmake -S "$root" -B "$work/lib" -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$prefix" "$@" > /dev/null
cmake --build "$work/lib"
cmake --install "$work/lib" > /dev/null

mkdir -p "$work/app"
cat > "$work/app/main.cpp" <<'EOF'
#include <iostream>
#include <rtxui/rtxui.hpp>

class App : public rtxui::Component<App> {
 public:
  int count = 0;
  void Bump() { count++; }
  std::string_view view = R"html(
    <div>
      <button onclick="Bump">clicked {count}</button>
      <checkbox>check</checkbox>
    </div>
  )html";
  App() {
    Bind(count);
    Bind(Bump);
  }
};

int main() {
  rtxui::HeadlessScreen screen(rtxui::Ref<App>::New(), 30, 6);
  screen.Input("\t\r");
  std::cout << screen.Text();
}
EOF

check() {  # <name> <binary>
  local out
  out=$(LD_LIBRARY_PATH="$prefix/lib:${LD_LIBRARY_PATH:-}" \
        DYLD_LIBRARY_PATH="$prefix/lib:${DYLD_LIBRARY_PATH:-}" \
        RTXUI_STRICT=1 "$2")
  if [[ "$out" != *"clicked 1"* || "$out" != *"☐ check"* ]]; then
    echo "$1: unexpected output:"
    echo "$out"
    exit 1
  fi
  echo "$1: OK"
}

step "find_package"
cat > "$work/app/CMakeLists.txt" <<'EOF'
cmake_minimum_required(VERSION 3.24)
project(app LANGUAGES CXX)
find_package(rtxui CONFIG REQUIRED)
add_executable(app main.cpp)
target_link_libraries(app PRIVATE rtxui::rtxui)
EOF
cmake -S "$work/app" -B "$work/find_package" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$prefix" > /dev/null
cmake --build "$work/find_package" > /dev/null
check find_package "$work/find_package/app"

step "pkg-config"
pc_dir=$(dirname "$(find "$prefix" -name rtxui.pc)")
flags=$(PKG_CONFIG_PATH="$pc_dir" pkg-config --cflags --libs rtxui)
# Without reflection the .pc names no standard; the consumer picks C++23.
std=-std=c++23
[[ "$flags" == *-std=* ]] && std=
# shellcheck disable=SC2086  # $flags is a list of flags.
"${CXX:-c++}" $std "$work/app/main.cpp" $flags -o "$work/pkg-config-app"
check pkg-config "$work/pkg-config-app"

step "FetchContent"
cat > "$work/app/CMakeLists.txt" <<EOF
cmake_minimum_required(VERSION 3.24)
project(app LANGUAGES CXX)
include(FetchContent)
FetchContent_Declare(rtxui SOURCE_DIR "$root")
FetchContent_MakeAvailable(rtxui)
add_executable(app main.cpp)
target_link_libraries(app PRIVATE rtxui)
EOF
cmake -S "$work/app" -B "$work/fetch_content" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release "$@" > /dev/null
cmake --build "$work/fetch_content" > /dev/null
check FetchContent "$work/fetch_content/app"
