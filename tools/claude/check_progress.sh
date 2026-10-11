#!/bin/bash
# Prints the progress of scripts/check.sh as one status line segment, or
# nothing when no check ran recently.
#
# Usage: tools/claude/check_progress.sh [build directory]
#
# Claude Code's pre-push hook runs scripts/check.sh and shows nothing until it
# ends. Called from a status line command with "refreshInterval": 1, this
# shows the step, a progress bar and the elapsed time while it runs, then the
# result for a few minutes.

file="${1:-build}/check-progress"
[[ -f $file ]] || exit 0

declare -A p
while IFS='=' read -r key value; do
  p[$key]=$value
done <"$file"

now=$(date +%s)
state=${p[state]}
# A run killed without its trap (SIGKILL, a closed terminal) leaves "running".
if [[ $state == running ]] && ! kill -0 "${p[pid]}" 2>/dev/null; then
  state=interrupted
fi
# The result stays up this long after the run ends.
if [[ $state != running ]] && ((now - ${p[updated]:-0} > 300)); then
  exit 0
fi

elapsed() {
  local s=$1
  if ((s >= 60)); then
    printf '%dm%02ds' $((s / 60)) $((s % 60))
  else
    printf '%ds' "$s"
  fi
}

bar() {
  local done=$1 total=$2 width=12 filled
  filled=$((done * width / total))
  printf '%s%s' "$(printf '%*s' "$filled" '' | tr ' ' '#')" \
    "$(printf '%*s' $((width - filled)) '' | tr ' ' '-')"
}

took=$(elapsed $((${p[updated]:-$now} - ${p[started]:-$now})))
case $state in
  running)
    # "Tests (unit, docs, ...)" reads as "Tests".
    printf '\033[33mcheck %s/%s %s\033[0m' \
      "${p[step_index]}" "${p[steps]}" "${p[step]%% (*}"
    if [[ -n ${p[total]} && ${p[total]} -gt 0 ]]; then
      printf ' [%s] %s/%s' "$(bar "${p[done]}" "${p[total]}")" \
        "${p[done]}" "${p[total]}"
    fi
    if [[ -n ${p[running]} ]]; then
      IFS=, read -ra running <<<"${p[running]}"
      printf ' %s' "${running[0]}"
      ((${#running[@]} > 1)) && printf ', %s' "${running[1]}"
      ((${#running[@]} > 2)) && printf ' +%d' $((${#running[@]} - 2))
    fi
    [[ -n ${p[failed]} ]] && printf ' \033[31m(failed: %s)\033[0m' "${p[failed]}"
    printf ' %s' "$(elapsed $((now - ${p[started]:-$now})))"
    ;;
  passed)
    printf '\033[32mcheck passed\033[0m in %s' "$took"
    ;;
  failed)
    printf '\033[31mcheck failed: %s\033[0m after %s' "${p[failed]}" "$took"
    ;;
  *)
    printf '\033[31mcheck interrupted\033[0m at %s' "${p[step]%% (*}"
    ;;
esac
