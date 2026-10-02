#!/usr/bin/env python3
# Copyright 2026 Arthur Sonzogni. All rights reserved.
# Use of this source code is governed by the MIT license that can be found in
# the LICENSE file.
"""Measures whether a coding agent can write working RTXUI apps.

For each task in tasks.json, a fresh directory gets a CMakeLists.txt linking an
installed RTXUI and an empty main.cpp. Claude Code then runs headless
(`claude -p`) in that directory, with the RTXUI plugin from tools/claude-plugin
loaded and permission to edit files and run ./check.sh, which builds the app
and prints its screen. Nothing else is allowed.

The result is then scored independently of what the agent claims: the app is
rebuilt, run with RTXUI_STRICT=1 and RTXUI_HEADLESS=80x24 for each check's
input, and the printed screen must contain every `expect` string and none of
the `reject` ones.

This runs real agent sessions and uses your Claude usage: roughly one session
per task. Run it when the docs, the skill or the API change, and read the
failures: each is a construct agents get wrong, i.e. something to fix in the
library, the docs or the skill.

Usage:
  python3 tools/agent_eval/run.py [--build-dir build] [--task NAME ...]
                                  [--model MODEL] [--keep]
"""
import argparse
import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TASKS = Path(__file__).with_name("tasks.json")
PLUGIN = ROOT / "tools" / "claude-plugin"

CMAKELISTS = """\
cmake_minimum_required(VERSION 3.24)
project(app LANGUAGES CXX)
find_package(rtxui CONFIG REQUIRED)
add_executable(app main.cpp)
target_link_libraries(app PRIVATE rtxui::rtxui)
"""

CHECK_SH = """\
#!/bin/bash
# Builds the app, then prints its screen after the given input.
# Usage: ./check.sh ['<input with printf escapes, e.g. \\t\\r>']
set -e
cd "$(dirname "$0")"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \\
  -DCMAKE_PREFIX_PATH='{prefix}' > /dev/null
ninja -C build > /dev/null
printf "${{1:-}}" | RTXUI_STRICT=1 RTXUI_HEADLESS=80x24 ./build/app
"""

INSTRUCTIONS = """\
{prompt}

Write the whole program in main.cpp in the current directory (CMakeLists.txt
is already set up and must not change). Run ./check.sh to build the app and
print its screen; pass terminal input as an argument with printf escapes, e.g.
./check.sh '\\t\\r' for Tab then Enter. Iterate until the screen shows what is
asked, for every input mentioned.
"""


def decode(escaped):
    """Turns "\\t\\r"-style escapes into the bytes a terminal would send."""
    return escaped.encode("latin-1").decode("unicode_escape").encode("latin-1")


def screen(workdir, escaped_input):
    env = dict(os.environ, RTXUI_STRICT="1", RTXUI_HEADLESS="80x24")
    result = subprocess.run([str(workdir / "build" / "app")], env=env,
                            input=decode(escaped_input), capture_output=True,
                            timeout=60, check=False)
    output = result.stdout.decode("utf-8", "replace")
    if result.returncode != 0:
        error = result.stderr.decode("utf-8", "replace").strip()
        return None, f"exited with {result.returncode}: {error}"
    return output, None


def score(task, workdir, prefix):
    """Returns a list of failure messages; empty means the task passed."""
    build = subprocess.run(
        f"cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release "
        f"-DCMAKE_PREFIX_PATH='{prefix}' > /dev/null && ninja -C build",
        shell=True, cwd=workdir, capture_output=True, text=True, check=False)
    if build.returncode != 0:
        lines = [l for l in build.stdout.splitlines() if "error" in l]
        return [f"does not build: {lines[0] if lines else build.stdout[-300:]}"]
    failures = []
    for check in task["checks"]:
        label = repr(check["input"]) if check["input"] else "no input"
        output, error = screen(workdir, check["input"])
        if error:
            failures.append(f"[{label}] {error}")
            continue
        for text in check.get("expect", []):
            if text not in output:
                failures.append(f"[{label}] missing {text!r}")
        for text in check.get("reject", []):
            if text in output:
                failures.append(f"[{label}] should not show {text!r}")
    return failures


def run_agent(task, workdir, model):
    command = [
        "claude", "-p", INSTRUCTIONS.format(prompt=task["prompt"]),
        "--plugin-dir", str(PLUGIN),
        "--allowedTools", "Read", "Write", "Edit", "Glob", "Grep",
        "Bash(./check.sh)", "Bash(./check.sh *)",
        "--output-format", "json",
        "--no-session-persistence",
    ]
    if model:
        command += ["--model", model]
    result = subprocess.run(command, cwd=workdir, capture_output=True,
                            text=True, timeout=1800, check=False)
    try:
        report = json.loads(result.stdout)
    except json.JSONDecodeError:
        return {"error": result.stderr.strip() or result.stdout[-300:]}
    return report


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", default="build",
                        help="configured RTXUI build to install from")
    parser.add_argument("--task", action="append",
                        help="run only these tasks (repeatable)")
    parser.add_argument("--model", help="model for the agent")
    parser.add_argument("--keep", action="store_true",
                        help="keep the task directories for inspection")
    args = parser.parse_args()

    if not shutil.which("claude"):
        print("The claude CLI is not on PATH.", file=sys.stderr)
        return 1
    tasks = json.loads(TASKS.read_text(encoding="utf-8"))
    if args.task:
        tasks = [t for t in tasks if t["name"] in args.task]

    base = Path(tempfile.mkdtemp(prefix="rtxui_agent_eval_"))
    prefix = base / "install"
    subprocess.run(["cmake", "--install", str(ROOT / args.build_dir),
                    "--prefix", str(prefix)], check=True,
                   capture_output=True)

    results = []
    for task in tasks:
        workdir = base / task["name"]
        workdir.mkdir()
        (workdir / "CMakeLists.txt").write_text(CMAKELISTS, encoding="utf-8")
        (workdir / "main.cpp").write_text("", encoding="utf-8")
        check = workdir / "check.sh"
        check.write_text(CHECK_SH.format(prefix=prefix), encoding="utf-8")
        check.chmod(0o755)

        print(f"=== {task['name']}: running the agent...", flush=True)
        report = run_agent(task, workdir, args.model)
        failures = (["agent failed: " + report["error"]] if "error" in report
                    else score(task, workdir, prefix))
        cost = report.get("total_cost_usd")
        turns = report.get("num_turns")
        status = "PASS" if not failures else "FAIL"
        print(f"{status} {task['name']} (turns: {turns}, "
              f"cost: {'$%.2f' % cost if cost is not None else '?'})")
        for failure in failures:
            print(f"    {failure}")
        results.append(not failures)

    passed = sum(results)
    print(f"\n{passed}/{len(results)} tasks passed.")
    if args.keep:
        print(f"Task directories kept in {base}")
    else:
        shutil.rmtree(base)
    return 0 if passed == len(results) else 1


if __name__ == "__main__":
    sys.exit(main())
