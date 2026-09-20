# RTXUI — Reactive Terminal User Interfaces for C++

[![Test](https://github.com/ArthurSonzogni/RTXUI/actions/workflows/test.yml/badge.svg)](https://github.com/ArthurSonzogni/RTXUI/actions/workflows/test.yml)
[![Shared library](https://github.com/ArthurSonzogni/RTXUI/actions/workflows/shared.yml/badge.svg)](https://github.com/ArthurSonzogni/RTXUI/actions/workflows/shared.yml)
[![Docs](https://github.com/ArthurSonzogni/RTXUI/actions/workflows/deploy-docs.yml/badge.svg)](https://arthursonzogni.github.io/RTXUI/)
[![Live Demo](https://img.shields.io/badge/demo-interactive_wasm-brightgreen.svg)](https://arthursonzogni.github.io/RTXUI/#interactive-playground)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)

**RTXUI** is a modern, high-performance C++ library for building beautiful, reactive Terminal User Interfaces (TUIs) utilizing web-inspired paradigms (HTML templates, CSS layouts, and live data binding).

<p align="center">
  <img src="docs/public/img/dashboard.svg" alt="An RTXUI service-health dashboard running in a terminal" width="100%">
</p>

<p align="center">
  <a href="https://arthursonzogni.github.io/RTXUI/#interactive-playground">
    <img src="https://img.shields.io/badge/🎮_Interactive_Demo-Try_in_Browser-success?style=for-the-badge&logo=webassembly" alt="Try Interactive Demo in Browser" />
  </a>
</p>

<p align="center">
  <em>
    <a href="example/app_dashboard.cpp">example/app_dashboard.cpp</a> —
    flexbox, a bound collection, computed values, conditional rendering and
    transitions, in one file.
    Regenerate with <code>tools/ansi_to_svg.py</code>.
  </em>
</p>

---

## 🌟 Key Features

*   **HTML & CSS in C++:** Declare TUI structures using semantic HTML tags and layout/style them using standard CSS (Flexbox, Grid, margins, paddings, borders, colors, and transitions).
*   **Reactive State Binding:** Bind C++ member variables and callbacks directly to template variables for seamless, reactive DOM updates.
*   **🔥 Live HTML/CSS Hot-Reloading:** Modify your component templates and styles inside your C++ source code and watch the running terminal application update **instantly without recompiling** or restarting!
*   **WebAssembly (WASM) Compatibility:** Easily compile your C++ TUI applications to WebAssembly to run them directly in web browsers.
*   **Keyboard & Spatial Navigation:** Full out-of-the-box support for sequential Tab/Shift-Tab cycling and dynamic Arrow key spatial navigation.
*   **Sleek Built-In Components:** Includes styled components like `<input>`, `<textarea>`, `<checkbox>`, `<radio>`, `<select>`, `<slider>`, `<progress>`, `<details>`, `<fieldset>`, `<tabs>`, and overlay modal `<dialog>`s.

---

## RTXUI or FTXUI?

Both are terminal UI libraries by the same author, and RTXUI is not a
replacement for [FTXUI](https://github.com/ArthurSonzogni/FTXUI) — they take
different bets.

**FTXUI** composes an interface from C++ elements: `hbox`, `vbox`, `border`,
and a `Renderer`/`Component` pair. Layout is expressed by nesting those calls,
which is direct and type-checked, and the library is mature, widely used, and
supported on Windows.

**RTXUI** runs a browser-style pipeline instead. Structure is an HTML template
in a string literal, styling is real CSS in a `<style>` block — a cascade with
selectors, specificity, pseudo-classes, custom properties, media queries and
transitions — and layout is block/inline/flex/grid producing physical
fragments. State is plain C++ members: mutate one and the DOM is diffed and
repainted.

Pick **FTXUI** when you want a mature, portable library with a compact C++ API,
need Windows, or your UI is simple enough that a layout engine is overhead.

Pick **RTXUI** when you already think in HTML and CSS, want to restyle without
touching C++ (including [hot reload](#-live-htmlcss-hot-reloading) of templates
in a running program), need real flexbox or grid, or want the same code to run
in a browser through WebAssembly. It is younger, and Linux/macOS only.

---

## 🚀 Live HTML/CSS Hot-Reloading

RTXUI leverages **`std::source_location`** to offer instantaneous hot-reloading of HTML/CSS templates during local development.

### How to use it:
Simply call `EnableHotReload()` in your component's constructor:

```cpp
class MyComponent : public Component<MyComponent> {
 public:
  std::string_view view = R"html(
    <div class="card">
      <h1>Hello, World!</h1>
      <p>Edit this template and save the file to see live updates!</p>
    </div>
    <style>
      .card {
        border: solid;
        border-color: rgb(59, 130, 246);
        padding: 1;
      }
      h1 { color: rgb(34, 197, 94); }
    </style>
  )html";

  MyComponent() {
    // Automatically maps to the correct source file and variable
    EnableHotReload(); 
  }
};
```

Run your compiled application. If you make changes to the `view` template in your editor and save, the running terminal UI will update in **<10ms** without recompilation.

---

## 🛠️ Installation & Build Setup

### Prerequisites

RTXUI runs on **Linux and macOS** (the terminal backend is POSIX), and in the
browser via **WebAssembly**. Windows is not supported yet.

#### 1. Install a C++23 compiler & build tools

RTXUI requires **C++23** (GCC 14+ or Clang 18+) and **CMake 3.24+** with **Ninja**. C++26 is optional: when the compiler provides reflection, RTXUI uses it to read struct fields in bound collections without a manual mapper, and falls back cleanly when it does not.

* **Debian / Ubuntu**:
  ```bash
  sudo apt install g++-14 ninja-build cmake
  ```
* **Fedora**:
  ```bash
  sudo dnf install gcc-c++ ninja-build cmake
  ```
* **Arch Linux**:
  ```bash
  sudo pacman -S gcc ninja cmake
  ```
* **macOS** (Homebrew):
  ```bash
  brew install ninja cmake
  # Uses Apple Clang (Xcode 16+) or GCC from Homebrew:
  # brew install gcc
  ```

Select your compiler explicitly per build rather than changing system defaults:

```bash
CC=gcc-14 CXX=g++-14 cmake -B build -G Ninja
# or with Clang:
# CC=clang CXX=clang++ cmake -B build -G Ninja
```

### Building the Project

Configure and compile the project using CMake and Ninja:
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DRTXUI_BUILD_EXAMPLES=ON -DRTXUI_BUILD_TESTS=ON
cmake --build build
```

Run the unit tests:
```bash
ctest --test-dir build --output-on-failure
# or run the Catch2 test runner directly:
./build/rtxui_test
```

Run any of the examples:
```bash
./build/rtxui_example_checkbox
./build/rtxui_example_demo
```

---

## 📦 Adding RTXUI to Your Project

### CMake FetchContent

You can integrate RTXUI directly into your CMake project:

```cmake
include(FetchContent)
FetchContent_Declare(
  rtxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/RTXUI.git
  GIT_TAG        main # Or pin a release tag / commit
)
FetchContent_MakeAvailable(rtxui)

add_executable(my_app main.cpp)
# Link against `rtxui` (not rtxui_lib) to preserve static element registrations
target_link_libraries(my_app PRIVATE rtxui)
```

See [Getting Started](docs/guide/getting-started.md) for full instructions and a minimal complete example.

---

## 🤝 Contributing

Contributions are welcome! Check out the [Contributing Guide](CONTRIBUTING.md) for build instructions, coding standards, and testing guidelines.

## 📄 License

Distributed under the MIT License. See [LICENSE](LICENSE) for more information.

