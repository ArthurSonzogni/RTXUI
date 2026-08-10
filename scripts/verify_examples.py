#!/usr/bin/env python3
# Copyright 2026 Arthur Sonzogni. All rights reserved.
# Use of this source code is governed by the MIT license that can be found in
# the LICENSE file.
"""Keep example/, the CMake example targets and the docs references in sync.

The docs embed examples in two ways, and both break silently when an example is
renamed or removed:

  * `<<< @/../example/foo.cpp` transcludes the source. A stale path fails the
    VitePress build, which breaks the docs deploy.
  * `src="/wasm/rtxui_example_foo.js"` embeds the WASM build. The artifact is
    produced by the `rtxui_example_foo` CMake target, so a missing target
    yields a 404 at runtime rather than a build error.

This script checks both directions and is wired up as the `verify_examples`
ctest.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXAMPLE_DIR = ROOT / "example"
CMAKELISTS = EXAMPLE_DIR / "CMakeLists.txt"
DOCS = ROOT / "docs"
INDEX = DOCS / "guide" / "examples.md"

# `add_executable(rtxui_example_foo foo.cpp)`, in example/CMakeLists.txt, where
# the source paths are relative to that directory.
TARGET_RE = re.compile(r"add_executable\(\s*rtxui_example_(\w+)\s+([\w.]+)\s*\)")
# `<<< @/../example/foo.cpp`
TRANSCLUDE_RE = re.compile(r"<<<\s*@/\.\./example/([\w.]+)")
# `src="/wasm/rtxui_example_foo.js"`
WASM_RE = re.compile(r"/wasm/rtxui_example_(\w+)\.js")


def main() -> int:
    errors = []

    on_disk = {p.name for p in EXAMPLE_DIR.glob("*.cpp")}
    if not on_disk:
        print(f"[ERROR] No examples found under {EXAMPLE_DIR}", file=sys.stderr)
        return 1

    cmake = CMAKELISTS.read_text(encoding="utf-8")
    targets = dict(TARGET_RE.findall(cmake))  # name -> source file
    target_sources = set(targets.values())

    # 1. Every example has a build target, and every target has a source.
    for source in sorted(on_disk - target_sources):
        errors.append(
            f"example/{source} has no add_executable() in example/CMakeLists.txt"
        )
    for name, source in sorted(targets.items()):
        if source not in on_disk:
            errors.append(
                f"target rtxui_example_{name} builds example/{source}, "
                f"which does not exist"
            )

    # 2. Every docs reference resolves.
    for md in sorted(DOCS.rglob("*.md")):
        if ".vitepress" in md.parts:
            continue
        text = md.read_text(encoding="utf-8")
        rel = md.relative_to(ROOT)
        for source in TRANSCLUDE_RE.findall(text):
            if source not in on_disk:
                errors.append(
                    f"{rel} transcludes example/{source}, which does not exist"
                )
        for name in WASM_RE.findall(text):
            if name not in targets:
                errors.append(
                    f"{rel} embeds /wasm/rtxui_example_{name}.js, but there is "
                    f"no rtxui_example_{name} target"
                )

    # 3. Every example is listed in the index page.
    index_text = INDEX.read_text(encoding="utf-8")
    for source in sorted(on_disk):
        if source not in index_text:
            errors.append(
                f"example/{source} is missing from "
                f"{INDEX.relative_to(ROOT)}"
            )

    # 4. Every example opens with the licence header followed by a comment
    #    saying what it teaches. Examples are read far more often than they are
    #    run, so an unexplained one is only half an example.
    for source in sorted(on_disk):
        lines = (EXAMPLE_DIR / source).read_text(encoding="utf-8").splitlines()
        if len(lines) < 5 or not lines[0].startswith("// Copyright"):
            errors.append(f"example/{source} is missing the licence header")
            continue
        # Licence is 3 lines, then a `//` separator, then the description.
        if not (lines[3].startswith("//") and lines[4].startswith("// ")):
            errors.append(
                f"example/{source} has no header comment explaining what it "
                f"demonstrates (expected `//` lines after the licence)"
            )

    if errors:
        for error in errors:
            print(f"[ERROR] {error}", file=sys.stderr)
        print(
            f"\n{len(errors)} example/docs consistency error(s).", file=sys.stderr
        )
        return 1

    print(
        f"OK: {len(on_disk)} examples, {len(targets)} targets, "
        f"all docs references resolve."
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
