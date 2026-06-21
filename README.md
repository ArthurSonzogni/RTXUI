# RTXUI — Reactive Terminal User Interfaces for C++

**RTXUI** is a modern, high-performance C++ library for building beautiful, reactive Terminal User Interfaces (TUIs) utilizing web-inspired paradigms (HTML templates, CSS layouts, and live data binding).

---

## 🌟 Key Features

*   **HTML & CSS in C++:** Declare TUI structures using semantic HTML tags and layout/style them using standard CSS (Flexbox, Grid, margins, paddings, borders, colors, and transitions).
*   **Reactive State Binding:** Bind C++ member variables and callbacks directly to template variables for seamless, reactive DOM updates.
*   **🔥 Live HTML/CSS Hot-Reloading:** Modify your component templates and styles inside your C++ source code and watch the running terminal application update **instantly without recompiling** or restarting!
*   **WebAssembly (WASM) Compatibility:** Easily compile your C++ TUI applications to WebAssembly to run them directly in web browsers.
*   **Keyboard & Spatial Navigation:** Full out-of-the-box support for sequential Tab/Shift-Tab cycling and dynamic Arrow key spatial navigation.
*   **Sleek Built-In Components:** Includes styled components like `<input>`, `<textarea>`, `<checkbox>`, `<radio>`, `<select>`, `<slider>`, `<progress>`, `<details>`, `<fieldset>`, `<tabs>`, and overlay modal `<dialog>`s.

---

## 🚀 Live HTML/CSS Hot-Reloading

RTXUI leverages modern **C++20 `std::source_location`** to offer instantaneous hot-reloading of HTML/CSS templates during local development.

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

#### 1. Install Clang 18 (C++20 Support)
```bash
# Download and install Clang 18
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 18
sudo ln -sf /usr/bin/clang-18 /usr/bin/clang
sudo ln -sf /usr/bin/clang++-18 /usr/bin/clang++
```

#### 2. Install Ninja Build System
```bash
sudo apt install ninja-build cmake
```

### Building the Project

Configure and compile the project using the helper script:
```bash
./build_project.sh
```

Run the unit tests:
```bash
./build/rtxui_test
```

Run any of the examples:
```bash
./build/rtxui_example_checkbox
./build/rtxui_example_demo
```
