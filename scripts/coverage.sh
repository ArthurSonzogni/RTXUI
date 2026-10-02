#!/bin/bash
# Code coverage of the library by rtxui_test, with Clang source-based coverage.
# Prints a per-directory summary (Markdown) and writes it to
# $BUILD_DIR/coverage.md.
#
# Usage: scripts/coverage.sh [--html]
#   --html  also write a browsable report to $BUILD_DIR/html/index.html
#
# Environment:
#   BUILD_DIR      build directory (default: build_coverage)
#   JOBS           parallelism (default: 2)
#   CC, CXX        compilers for the first configure (default: clang, clang++)
#   LLVM_PROFDATA  must match the compiler's LLVM version (default: llvm-profdata)
#   LLVM_COV       must match the compiler's LLVM version (default: llvm-cov)
set -euo pipefail
cd "$(dirname "$0")/.."

BUILD_DIR=${BUILD_DIR:-build_coverage}
JOBS=${JOBS:-2}
LLVM_PROFDATA=${LLVM_PROFDATA:-llvm-profdata}
LLVM_COV=${LLVM_COV:-llvm-cov}

if [[ ! -f "$BUILD_DIR/build.ninja" ]]; then
  CC=${CC:-clang} CXX=${CXX:-clang++} cmake -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug -DRTXUI_BUILD_TESTS=ON -DRTXUI_COVERAGE=ON
fi
ninja -C "$BUILD_DIR" -j "$JOBS" rtxui_test

rm -f "$BUILD_DIR"/*.profraw
LLVM_PROFILE_FILE="$BUILD_DIR/rtxui_test-%p.profraw" \
  "$BUILD_DIR/rtxui_test" >/dev/null 2>&1 ||
  echo "warning: some tests failed; coverage is still reported" >&2
"$LLVM_PROFDATA" merge -sparse "$BUILD_DIR"/*.profraw \
  -o "$BUILD_DIR/rtxui.profdata"

# Only the library counts: its sources and public headers, minus tests and
# fuzzers. Listing the directories also keeps out Catch2 and the examples
# some tests compile in.
cov_args=(-instr-profile="$BUILD_DIR/rtxui.profdata"
  -ignore-filename-regex='(_test|_fuzzer)\.cpp$')
sources=("$PWD/src/rtxui" "$PWD/include")

"$LLVM_COV" export "$BUILD_DIR/rtxui_test" "${cov_args[@]}" -summary-only \
  "${sources[@]}" |
  python3 -c '
import collections, json, os, sys
root = os.getcwd() + "/"
dirs = collections.defaultdict(lambda: [0, 0])
for f in json.load(sys.stdin)["data"][0]["files"]:
    path = f["filename"].removeprefix(root)
    parts = path.split("/")
    key = "/".join(parts[:3]) if parts[0] == "src" else parts[0]
    lines = f["summary"]["lines"]
    dirs[key][0] += lines["covered"]
    dirs[key][1] += lines["count"]
def row(name, covered, count):
    pct = 100 * covered / count if count else 100
    return f"| {name} | {covered} / {count} | {pct:.1f}% |"
print("| Directory | Lines | Coverage |")
print("|---|---:|---:|")
for name in sorted(dirs):
    print(row(name, *dirs[name]))
total = [sum(v[i] for v in dirs.values()) for i in (0, 1)]
print(row("**Total**", *total))
' | tee "$BUILD_DIR/coverage.md"

if [[ "${1:-}" == "--html" ]]; then
  "$LLVM_COV" show "$BUILD_DIR/rtxui_test" "${cov_args[@]}" \
    -format=html -output-dir="$BUILD_DIR/html" "${sources[@]}"
  echo "HTML report: $BUILD_DIR/html/index.html"
fi
