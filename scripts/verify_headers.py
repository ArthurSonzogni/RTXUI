#!/usr/bin/env python3
# Copyright 2026 Arthur Sonzogni. All rights reserved.
# Use of this source code is governed by the MIT license that can be found in
# the LICENSE file.
"""Check that every public header compiles on its own.

A header that only builds because some *other* header happened to be included
first is a trap: it works in this tree and fails in a consumer's, or on a
different standard library. refcounted.hpp used std::nullptr_t without
including <cstddef> and got away with it under libstdc++ for exactly that
reason -- it only surfaced when macOS built it against libc++.

Each public header is compiled as a translation unit containing nothing but
that one include.

Note what this can and cannot do: it only catches an include the *local*
standard library does not supply transitively. libstdc++ hands out <cstddef>
through almost everything, so the bug above stays invisible on Linux and is
caught by the macOS leg running libc++. Run it on both.

Usage:
  python3 scripts/verify_headers.py [--cxx g++] [--std c++23]
"""
import argparse
import os
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

# The headers a consumer can include. Kept in step with the install() rules in
# CMakeLists.txt -- a header installed but not listed here is untested.
PUBLIC_HEADERS = [
    "rtxui/rtxui.hpp",
    "rtxui/color.hpp",
    "rtxui/paint/color.hpp",
    "rtxui/internal/class_name.hpp",
    "rtxui/internal/component.hpp",
    "rtxui/internal/event.hpp",
    "rtxui/internal/import.hpp",
    "rtxui/internal/refcounted.hpp",
    "rtxui/internal/screen.hpp",
]


def find_header(relative: str) -> Path | None:
    candidate = ROOT / "include" / relative
    if candidate.exists():
        return candidate
    return None


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--cxx", default=os.environ.get("CXX", "c++"))
    parser.add_argument("--std", default="c++23")
    parser.add_argument("--export-header-dir", default=None,
                        help="directory holding the generated rtxui_export.hpp")
    args = parser.parse_args()

    export_dir = args.export_header_dir
    if not export_dir:
        # Any configured build tree will do; the generated header is identical.
        for candidate in ROOT.glob("build*/generated"):
            export_dir = str(candidate)
            break
    if not export_dir or not Path(export_dir, "rtxui/rtxui_export.hpp").exists():
        print("[SKIP] no generated rtxui_export.hpp found; configure a build "
              "tree first or pass --export-header-dir", file=sys.stderr)
        return 0

    includes = [f"-I{ROOT / 'include'}", f"-I{ROOT / 'src'}", f"-I{export_dir}"]
    failures = []

    with tempfile.TemporaryDirectory() as tmp:
        for relative in PUBLIC_HEADERS:
            header = find_header(relative)
            if header is None:
                failures.append(f"{relative}: not found under include/ or src/")
                continue

            source = Path(tmp, "tu.cpp")
            source.write_text(f'#include <{relative}>\n', encoding="utf-8")
            result = subprocess.run(
                [args.cxx, f"-std={args.std}", "-fsyntax-only", *includes,
                 str(source)],
                capture_output=True, text=True)
            if result.returncode != 0:
                first = result.stderr.strip().splitlines()
                detail = first[0] if first else "compilation failed"
                failures.append(f"{relative}: {detail}")

    if failures:
        for failure in failures:
            print(f"[ERROR] {failure}", file=sys.stderr)
        print(f"\n{len(failures)} header(s) are not self-contained.",
              file=sys.stderr)
        return 1

    print(f"OK: {len(PUBLIC_HEADERS)} public headers each compile standalone "
          f"({args.std}).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
