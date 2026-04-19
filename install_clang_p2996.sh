#!/bin/bash
set -e

# --- Configuration ---
INSTALL_DIR="$HOME/.local/clang-p2996"
BUILD_DIR="clang-p2996/build"
REPO_URL="https://github.com/bloomberg/clang-p2996.git"
BRANCH="p2996"
JOBS=4 # Limit cores to 4 to avoid out-of-memory errors (LLVM uses ~2GB RAM/core)

echo "-------------------------------------------------------"
echo "RTXUI: Bloomberg Clang P2996 (Reflection) Installer"
echo "-------------------------------------------------------"

# 1. Install dependencies
echo "[1/4] Checking and installing build dependencies..."
sudo apt update
sudo apt install -y cmake ninja-build build-essential python3 git curl

# 2. Clone the repository
if [ ! -d "clang-p2996" ]; then
    echo "[2/4] Cloning bloomberg/clang-p2996 (branch: $BRANCH)..."
    git clone --depth 1 --branch "$BRANCH" "$REPO_URL"
else
    echo "[2/4] Repository already exists, skipping clone."
fi

# 3. Configure the build
echo "[3/4] Configuring the build with CMake..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Lean configuration:
# - Release mode for performance
# - X86 only to reduce build time (30 min vs 3 hours)
# - Enable Clang and LLD (linker)
cmake -G Ninja ../llvm \
  -DLLVM_ENABLE_PROJECTS="clang;lld" \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_TARGETS_TO_BUILD="X86" \
  -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
  -DLLVM_INCLUDE_TESTS=OFF \
  -DLLVM_INCLUDE_EXAMPLES=OFF

# 4. Build
echo "[4/4] Starting build (Ninja, Jobs: $JOBS). This may take 30-60 minutes..."
echo "Note: You can monitor progress below. Your system is safe to use."
ninja -j "$JOBS" clang lld

echo "-------------------------------------------------------"
echo "Build Successful!"
echo "-------------------------------------------------------"
echo "The reflection-capable compiler is located at:"
echo "  $(pwd)/bin/clang++"
echo ""
echo "To test your Transparent Reactivity PoC, run:"
echo "  $(pwd)/bin/clang++ -std=c++26 -freflection-latest src/reflection/transparent_test.cpp -o transparent_test"
echo "  ./transparent_test"
echo "-------------------------------------------------------"
