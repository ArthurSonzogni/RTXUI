#!/usr/bin/env python3
# Copyright 2026 Arthur Sonzogni. All rights reserved.
# Use of this source code is governed by the MIT license that can be found in
# the LICENSE file.
"""Check that the C++ snippets in the docs compile.

Docs drift: an API is renamed, a snippet keeps the old name, and readers --
people and LLMs alike -- copy code that no longer builds. Every ```cpp block
that defines a component (mentions `Component<`) is compiled against the
public headers only, as a consumer would, wrapped in:

    #include <rtxui/rtxui.hpp>
    using namespace rtxui;

A block that is deliberately incomplete opts out with this comment on the line
before its opening fence:

    <!-- snippet: fragment -->

Usage:
  python3 scripts/verify_doc_snippets.py [--cxx c++] [--std c++23]
                                         [--export-header-dir DIR]
"""
import argparse
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DOCS = [p for p in sorted((ROOT / "docs").rglob("*.md"))
        if "node_modules" not in p.parts and ".vitepress" not in p.parts]
FENCE = re.compile(r"^```cpp\s*$")
OPT_OUT = "<!-- snippet: fragment -->"
PRELUDE = """#include <rtxui/rtxui.hpp>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>
using namespace rtxui;
"""


def snippets(path):
    """Yields (line number, code) for each block that should compile."""
    lines = path.read_text(encoding="utf-8").splitlines()
    i = 0
    while i < len(lines):
        if FENCE.match(lines[i]):
            start = i
            i += 1
            body = []
            while i < len(lines) and not lines[i].startswith("```"):
                body.append(lines[i])
                i += 1
            previous = lines[start - 1].strip() if start > 0 else ""
            code = "\n".join(body)
            if previous != OPT_OUT and "Component<" in code:
                yield start + 1, code
        i += 1


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--cxx", default=os.environ.get("CXX", "c++"))
    parser.add_argument("--std", default="c++23")
    parser.add_argument("--export-header-dir", default=None)
    args = parser.parse_args()

    export_dir = args.export_header_dir
    if not export_dir:
        for candidate in ROOT.glob("build*/generated"):
            export_dir = str(candidate)
            break
    if not export_dir or not Path(export_dir, "rtxui/rtxui_export.hpp").exists():
        print("[SKIP] no generated rtxui_export.hpp found; configure a build "
              "tree first or pass --export-header-dir", file=sys.stderr)
        return 0

    includes = [f"-I{ROOT / 'include'}", f"-I{export_dir}"]
    failures = []
    count = 0
    with tempfile.TemporaryDirectory() as tmp:
        source = Path(tmp, "snippet.cpp")
        for path in DOCS:
            for line, code in snippets(path):
                count += 1
                source.write_text(PRELUDE + code + "\n", encoding="utf-8")
                result = subprocess.run(
                    [args.cxx, f"-std={args.std}", "-fsyntax-only",
                     *includes, str(source)],
                    capture_output=True, text=True, check=False)
                if result.returncode != 0:
                    errors = [l for l in result.stderr.splitlines()
                              if "error" in l]
                    detail = errors[0] if errors else "compilation failed"
                    detail = detail.replace(str(source), "snippet")
                    failures.append(
                        f"{path.relative_to(ROOT)}:{line}: {detail}")

    for failure in failures:
        print(f"[ERROR] {failure}", file=sys.stderr)
    if failures:
        print(f"{len(failures)} of {count} doc snippets do not compile. Fix "
              f"them, or mark a deliberate fragment with {OPT_OUT}",
              file=sys.stderr)
        return 1
    print(f"[OK] {count} doc snippets compile.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
