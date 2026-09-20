# Getting Started

This page covers what you need to build RTXUI, how to add it to a CMake
project, and how to build the repository itself.

## Requirements

- **A C++23 compiler.** C++23 is required to build and use RTXUI. If available, C++26 enables reflection, which avoids having to bind C++ members manually.
- **Linux or macOS.** The terminal backend requires POSIX termios and
  `SIGWINCH`. Windows is not supported (no ConPTY backend); CMake terminates
  configuration on Windows platforms with an explicit error. RTXUI also
  targets the browser through Emscripten / WebAssembly.
- **CMake 3.24 or newer.**
- **Ninja** (optional, faster builds than Make).

On Debian or Ubuntu:

```bash
sudo apt install cmake ninja-build g++-14
```

Select the compiler per build rather than changing system defaults — pass it
to CMake explicitly:

```bash
CC=gcc-14 CXX=g++-14 cmake -B build -G Ninja
```

(For Clang, install it from your distribution or [apt.llvm.org](https://apt.llvm.org)
and set `CC=clang CXX=clang++` the same way.)

## Adding RTXUI to your project

### With FetchContent

The simplest integration downloads RTXUI at configure time:

```cmake
cmake_minimum_required(VERSION 3.24)
project(my_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)  # Use 26 to enable automatic static reflection.
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
  rtxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/RTXUI.git
  GIT_TAG        main  # Pin a tag or commit for reproducible builds.
)
FetchContent_MakeAvailable(rtxui)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE rtxui)
```

Link against the **`rtxui`** target, not `rtxui_lib`. Built-in elements
register themselves through static initializers, and the `rtxui` target
carries the linker flags that keep those initializers from being discarded.
Linking `rtxui_lib` directly can produce binaries where tags like
`<button>` silently fail to resolve.

### With an installed package

If RTXUI has been installed (`cmake --install`), consume it with
`find_package`:

```cmake
find_package(rtxui REQUIRED)
target_link_libraries(my_app PRIVATE rtxui::rtxui)
```

## First program

Create `main.cpp`:

```cpp
#include <rtxui/rtxui.hpp>

class App : public rtxui::Component<App> {
 public:
  std::string_view view = R"html(
    <div>Hello from RTXUI.</div>
  )html";
};

int main() {
  auto app = rtxui::Ref<App>::New();
  rtxui::Screen screen(app);
  screen.Loop();
}
```

Configure, build, and run:

```bash
CC=gcc-14 CXX=g++-14 cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/my_app
```

Press <kbd>Ctrl-C</kbd> to quit. The
[Hello World walkthrough](/guide/hello-world) explains each line and adds
styling.

## Building the repository

To work on RTXUI itself, or to run its examples and tests:

```bash
git clone https://github.com/ArthurSonzogni/RTXUI.git
cd RTXUI
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DRTXUI_BUILD_TESTS=ON -DRTXUI_BUILD_EXAMPLES=ON
cmake --build build
```

- Run the test suite: `./build/rtxui_test` (or `ctest --test-dir build`)
- Run an example: `./build/rtxui_example_demo`
- Sanitizer build: add `-DRTXUI_SANITIZE=ON` for AddressSanitizer and
  UndefinedBehaviorSanitizer.

The examples under `example/` are the fastest way to explore what the
engine can do; each one is a single self-contained file.
