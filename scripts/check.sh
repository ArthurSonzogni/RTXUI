#!/bin/bash
# The gate every change must pass before it is pushed to main. Run by the
# pre-push hook and by Claude Code before `git push`; CI runs the lint half.
#
# Usage: scripts/check.sh [--lint-only]
#
# Environment:
#   BUILD_DIR  build directory to use (default: build)
#   BASE       ref to diff against for clang-tidy (default: origin/main)
#   JOBS       parallelism (default: 2)
set -uo pipefail
cd "$(dirname "$0")/.."

BUILD_DIR=${BUILD_DIR:-build}
BASE=${BASE:-origin/main}
JOBS=${JOBS:-2}
LINT_ONLY=0
[[ "${1:-}" == "--lint-only" ]] && LINT_ONLY=1

failed=()
step() { echo; echo "==> $1"; }
fail() { echo "FAILED: $1"; failed+=("$1"); }

cpp_files() {
  git ls-files -- src include example benchmarks |
    grep -E '\.(cpp|hpp|h|cc)$'
}

step "clang-format"
cpp_files | xargs clang-format --dry-run --Werror ||
  fail "clang-format (fix with tools/format.sh)"

step "No C++ exceptions"
if git grep -nE '\btry\s*\{|\bcatch\s*\(' -- src include example benchmarks; then
  fail "try/catch is banned (see AGENTS.md)"
fi

step "clang-tidy on files changed since $BASE"
if [[ ! -f "$BUILD_DIR/compile_commands.json" ]]; then
  echo "skipped: no $BUILD_DIR/compile_commands.json" \
    "(configure with -DCMAKE_EXPORT_COMPILE_COMMANDS=ON)"
else
  # Fuzzers are only in the compile database of a fuzzing build.
  changed=$(git diff --name-only --diff-filter=d "$BASE" -- 'src/*.cpp' \
    ':!*_fuzzer.cpp')
  if [[ -n "$changed" ]]; then
    echo "$changed" |
      xargs -P "$JOBS" -n 1 clang-tidy -p "$BUILD_DIR" --quiet \
        --warnings-as-errors='*' 2>/dev/null || fail "clang-tidy"
  fi
fi

if [[ $LINT_ONLY == 0 ]]; then
  step "Build"
  if ninja -C "$BUILD_DIR" -j "$JOBS"; then
    step "Tests (unit, docs, examples, headers)"
    ctest --test-dir "$BUILD_DIR" -j "$JOBS" --output-on-failure || fail "ctest"
  else
    fail "build"
  fi
fi

echo
if [[ ${#failed[@]} -gt 0 ]]; then
  printf 'check.sh: %s\n' "${failed[@]}"
  exit 1
fi
echo "check.sh: all checks passed"
