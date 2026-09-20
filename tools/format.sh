#!/bin/bash
set -e

# Format all C++ source and header files under src, include, example, and benchmarks directories.
find src include example benchmarks \
  -type f \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" -o -name "*.cc" \) \
  -exec clang-format -i {} +

echo "clang-format execution finished successfully!"
