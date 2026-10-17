# Getting Started

This page covers what you need to build RTXUI, how to add it to a CMake
project, and how to build the repository itself.

## Requirements

- **A C++23 compiler.** C++23 is required to build and use RTXUI. A compiler with C++26
  reflection also lets loops read the fields of your own structs without a
  `Fields()` function (see [Loops](/guide/loops)).
- **Linux, macOS or Windows.** On Windows, RTXUI drives the console through
  its virtual terminal mode: Windows Terminal, or the console of Windows 10
  version 1809 and newer. MSVC, clang-cl and MinGW-w64 GCC are tested. RTXUI
  also targets the browser through Emscripten / WebAssembly.
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

set(CMAKE_CXX_STANDARD 23)
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

### With an installed package

If RTXUI has been installed (`cmake --install`), consume it with
`find_package`:

```cmake
find_package(rtxui REQUIRED)
target_link_libraries(my_app PRIVATE rtxui::rtxui)
```

Outside CMake (Meson, Makefiles, ...), the install also provides `rtxui.pc`
for pkg-config:

```bash
c++ -std=c++23 main.cpp $(pkg-config --cflags --libs rtxui) -o my_app
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
- Run an example: `./build/rtxui_example_counter`
- Sanitizer build: add `-DRTXUI_SANITIZE=ON` for AddressSanitizer and
  UndefinedBehaviorSanitizer.

The examples under `example/` are the fastest way to explore what the
engine can do; each one is a single self-contained file.
