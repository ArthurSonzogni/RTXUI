# Contributing to RTXUI

Thank you for your interest in contributing to RTXUI!

## Development Setup

### Prerequisites

- **C++23 compiler**: GCC 14+ or Clang 18+.
- **Build system**: CMake (>= 3.24) and Ninja.

### Building & Running Tests

```bash
# Configure
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DRTXUI_BUILD_EXAMPLES=ON -DRTXUI_BUILD_TESTS=ON

# Build all targets
cmake --build build

# Run unit tests and verification checks
ctest --test-dir build --output-on-failure
```

You can also run the test runner directly:
```bash
./build/rtxui_test
./build/rtxui_test "[style]"      # Run by tag
./build/rtxui_test "test name"    # Run a specific test
```

## Engineering Standards

### Coding Style
- Follow the [Chromium C++ Coding Style](https://chromium.googlesource.com/chromium/src/+/main/styleguide/c++/c++.md).
- Keep code clean, readable, and properly formatted.
- Format all files before submitting changes:
  ```bash
  ./tools/format.sh
  ```

### Exception Handling
- C++ `try/catch` blocks are strictly **banned** in this codebase.
- Do not write `try` or `catch` in any C++ source or header files.
- Use error-checking return values, non-throwing overloads (such as `std::filesystem::last_write_time` taking `std::error_code`), and parser checks (like `std::from_chars`) instead of throwing exceptions.

### Regression Testing
- **Always add a regression test for every bug fixed.**
- Tests are written with Catch2 and live next to the code they test (`*_test.cpp`), typically in `src/rtxui/component/component_test.cpp` or `src/rtxui/style/style_test.cpp`.
- Every pull request fixing an issue must include an automated test that fails before the fix and passes after.

### Documentation Consistency
- When adding or modifying CSS properties in `src/rtxui/style/apply_style.cpp`, update `docs/css_reference.md`. This is verified automatically by `python3 scripts/verify_docs.py` in `ctest`.
- When adding new examples or built-in components, update the respective guides under `docs/`.

## Submitting Changes

1. Fork the repository and create your branch from `main`.
2. Ensure all tests pass (`ctest --test-dir build --output-on-failure`).
3. Format your code with `./tools/format.sh`.
4. Write clear, descriptive commit messages (preferably conventional commits, e.g., `feat: ...`, `fix: ...`).
5. Open a Pull Request on GitHub.
