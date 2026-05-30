#!/bin/bash
set -e

# Format all C++ source and header files under src, include, and example directories.
find src include example \
  -type f \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" -o -name "*.cc" \) \
  -exec clang-format -i {} +

echo "clang-format execution finished successfully!"
