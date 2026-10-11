#!/bin/bash
# The gate every change must pass before it is pushed to main. Run by the
# pre-push hook and by Claude Code before `git push`; CI runs the lint half.
#
# Usage: scripts/check.sh [--lint-only]
#
# Environment:
#   BUILD_DIR  build directory to use (default: build)
#   BASE       ref to diff against for clang-tidy (default: origin/main)
#   JOBS       build and test parallelism (default: 2)
#   TIDY_JOBS  clang-tidy parallelism (default: 8; about 0.7 GB each at most)
#
# Progress goes to $BUILD_DIR/check-progress as it runs, for
# tools/claude/check_progress.sh to show in Claude Code's status line.
set -uo pipefail
cd "$(dirname "$0")/.."

BUILD_DIR=${BUILD_DIR:-build}
BASE=${BASE:-origin/main}
JOBS=${JOBS:-2}
TIDY_JOBS=${TIDY_JOBS:-8}
LINT_ONLY=0
[[ "${1:-}" == "--lint-only" ]] && LINT_ONLY=1

failed=()
total_steps=5
[[ $LINT_ONLY == 1 ]] && total_steps=3
step_index=0
step_name=""
started=$(date +%s)
progress_file="$BUILD_DIR/check-progress"
mkdir -p "$BUILD_DIR"

# Rewrites the progress file whole, through a rename, so a reader never sees
# half of it. Arguments: state, then done/total within the step (optional).
progress() {
  local failures
  failures=$(IFS=,; echo "${failed[*]:-}")
  printf 'pid=%s\nstate=%s\nstep=%s\nstep_index=%s\nsteps=%s\ndone=%s\ntotal=%s\nrunning=%s\nstarted=%s\nupdated=%s\nfailed=%s\n' \
    "$$" "$1" "$step_name" "$step_index" "$total_steps" "${2:-}" "${3:-}" \
    "${4:-}" "$started" "$(date +%s)" "$failures" >"$progress_file.tmp" &&
    mv -f "$progress_file.tmp" "$progress_file"
}

# Passes a command's output through, recording the "[done/total]" ninja
# prints and the "done/total Test #n" ctest prints, and which tests ctest has
# started but not finished: the last one can take minutes on its own.
track() {
  local line done="" total="" running
  local -A started_tests=()
  while IFS= read -r line; do
    printf '%s\n' "$line"
    if [[ $line =~ ^\[([0-9]+)/([0-9]+)\] ]]; then
      done=${BASH_REMATCH[1]} total=${BASH_REMATCH[2]}
    elif [[ $line =~ ^\ *Start\ +([0-9]+):\ (.*)$ ]]; then
      started_tests[${BASH_REMATCH[1]}]=${BASH_REMATCH[2]}
    elif [[ $line =~ ^\ *([0-9]+)/([0-9]+)\ Test\ +#([0-9]+): ]]; then
      done=${BASH_REMATCH[1]} total=${BASH_REMATCH[2]}
      unset "started_tests[${BASH_REMATCH[3]}]"
    else
      continue
    fi
    running=$(IFS=,; echo "${started_tests[*]:-}")
    progress running "$done" "$total" "$running"
  done
}

step() {
  echo; echo "==> $1"
  step_name=$1
  step_index=$((step_index + 1))
  progress running
}
fail() { echo "FAILED: $1"; failed+=("$1"); }
# An interrupted run is not left showing as running.
trap 'progress interrupted; exit 130' INT TERM

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
    'example/*.cpp' ':!*_fuzzer.cpp')
  if [[ -n "$changed" ]]; then
    echo "$changed" |
      xargs -P "$TIDY_JOBS" -n 1 clang-tidy -p "$BUILD_DIR" --quiet \
        --warnings-as-errors='*' 2>/dev/null || fail "clang-tidy"
  fi
fi

if [[ $LINT_ONLY == 0 ]]; then
  step "Build"
  if NINJA_STATUS='[%f/%t] ' ninja -C "$BUILD_DIR" -j "$JOBS" | track; then
    step "Tests (unit, docs, examples, headers)"
    ctest --test-dir "$BUILD_DIR" -j "$JOBS" --output-on-failure | track ||
      fail "ctest"
  else
    fail "build"
  fi
fi

echo
if [[ ${#failed[@]} -gt 0 ]]; then
  printf 'check.sh: %s\n' "${failed[@]}"
  progress failed
  exit 1
fi
progress passed
echo "check.sh: all checks passed"
