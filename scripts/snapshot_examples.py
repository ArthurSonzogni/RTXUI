#!/usr/bin/env python3
# Copyright 2026 Arthur Sonzogni. All rights reserved.
# Use of this source code is governed by the MIT license that can be found in
# the LICENSE file.
"""Compares what every example draws with its snapshot in example/snapshots/.

Each example runs headless (RTXUI_HEADLESS), which prints its first screen as
plain text and exits, so a change anywhere in the engine that alters what an
example shows -- layout, borders, wrapping, a style no longer applied -- shows
up as a diff of that text. Run with --update to accept the new output after
checking the diff is intended.

Usage: snapshot_examples.py <directory with rtxui_example_* binaries> [--update]
"""
import difflib
import os
import pathlib
import subprocess
import sys

SIZE = "80x24"
ROOT = pathlib.Path(__file__).resolve().parent.parent
SNAPSHOTS = ROOT / "example" / "snapshots"


def render(binary):
    env = dict(os.environ, RTXUI_HEADLESS=SIZE, RTXUI_STRICT="1")
    result = subprocess.run([str(binary)], env=env, stdin=subprocess.DEVNULL,
                            capture_output=True, timeout=60, check=False)
    if result.returncode != 0:
        raise RuntimeError(
            f"exited with {result.returncode}: "
            f"{result.stderr.decode('utf-8', 'replace').strip()}")
    return result.stdout.decode("utf-8")


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    update = "--update" in sys.argv[1:]
    if len(args) != 1:
        print(__doc__)
        return 2
    binaries = sorted(pathlib.Path(args[0]).glob("rtxui_example_*"))
    binaries = [b for b in binaries if b.is_file() and os.access(b, os.X_OK)]
    if not binaries:
        print(f"No rtxui_example_* binaries in {args[0]}")
        return 1

    SNAPSHOTS.mkdir(exist_ok=True)
    failures = []
    for binary in binaries:
        name = binary.name.removeprefix("rtxui_example_")
        snapshot = SNAPSHOTS / f"{name}.txt"
        try:
            actual = render(binary)
        except (RuntimeError, subprocess.TimeoutExpired) as error:
            failures.append(name)
            print(f"FAIL {name}: {error}")
            continue
        if update:
            if not snapshot.exists() or snapshot.read_text("utf-8") != actual:
                snapshot.write_text(actual, "utf-8")
                print(f"updated {snapshot.relative_to(ROOT)}")
            continue
        expected = snapshot.read_text("utf-8") if snapshot.exists() else ""
        if actual != expected:
            failures.append(name)
            print(f"FAIL {name}: output differs from "
                  f"{snapshot.relative_to(ROOT)}")
            sys.stdout.writelines(difflib.unified_diff(
                expected.splitlines(keepends=True),
                actual.splitlines(keepends=True),
                "expected", "actual"))

    if failures:
        print(f"\n{len(failures)} of {len(binaries)} examples differ. If the "
              "change is intended, run:\n  python3 scripts/snapshot_examples.py "
              f"{args[0]} --update")
        return 1
    print(f"{len(binaries)} examples match their snapshots.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
