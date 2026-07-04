# Getting Started

This guide walks you through the system requirements, build setup, and integrating RTXUI into your C++ projects.

---

## Prerequisites

RTXUI is built using modern C++26 features. Ensure your development environment meets the following requirements:

### Supported Compilers
*   **Clang 18+** (Recommended)
*   **GCC 13+**

### Build Tools
*   **CMake 3.24+**
*   **Ninja** (Recommended for fast builds) or **GNU Make**

---

## Environment Setup

### 1. Install Toolchain (Debian/Ubuntu)

Run the following commands to install CMake, Ninja, and the recommended Clang compiler:

```bash
# Install CMake and Ninja
sudo apt update
sudo apt install cmake ninja-build build-essential -y

# Install Clang 18
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 18

# Configure system defaults
sudo ln -sf /usr/bin/clang-18 /usr/bin/clang
sudo ln -sf /usr/bin/clang++-18 /usr/bin/clang++
```

---

## Integrating RTXUI into your Project

The easiest way to integrate RTXUI into a CMake project is using `FetchContent` to download it automatically during configuration.

### CMakeLists.txt Example

Create a `CMakeLists.txt` file for your application:

```cmake
cmake_minimum_required(VERSION 3.24)
project(my_rtxui_app LANGUAGES CXX)

# Enforce C++26 Standard (required by RTXUI)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Fetch RTXUI Dependency
include(FetchContent)
FetchContent_Declare(
  rtxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/RTXUI.git
  GIT_TAG        main # Or use a specific tag/commit hash
)
FetchContent_MakeAvailable(rtxui)

# Declare your executable
add_executable(my_app main.cpp)

# Link with RTXUI
target_link_libraries(my_app PRIVATE rtxui_lib)
```

---

## Compilation & Run

1. Configure your project with CMake and Ninja:
   ```bash
   cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
   ```

2. Compile your binary:
   ```bash
   cmake --build build
   ```

3. Run the executable:
   ```bash
   ./build/my_app
   ```
