#!/usr/bin/env python3
# Copyright 2026 Arthur Sonzogni. All rights reserved.
# Use of this source code is governed by the MIT license that can be found in
# the LICENSE file.
"""Rewrites an example to use `import rtxui;` instead of the headers.

Usage: modularize_example.py <example.cpp> <output.cpp>

cmake/modules.cmake builds every example this way, and the snapshot test
checks each one draws exactly what its header-based build does. The examples
are the module's specification: a name they need that the module does not
export fails to compile, and the module exports nothing they do not need.

The examples include every standard header they use (clang-tidy's
misc-include-cleaner checks it), as an importer must: the module makes none
of them visible. The import goes after them, as GCC requires a header to be
included before a module that also includes it is imported.
"""
import pathlib
import re
import sys

# Any public header, with <> or "".
RTXUI_INCLUDE = re.compile(r'#include\s*[<"]rtxui/[a-z_]+\.hpp[>"]')
# The opening of a raw string literal, R"delimiter(.
RAW_STRING_START = re.compile(r'R"([^(\s]*)\(')

def outside_raw_strings(lines):
    """Yields whether each line is code, rather than inside a raw string.

    An example can show an include inside a template or a Markdown code
    block, as markdown.cpp does, and that one must stay.
    """
    closing = None
    for line in lines:
        if closing is not None:
            yield False
            if closing in line:
                closing = None
            continue
        yield True
        start = RAW_STRING_START.search(line)
        if start:
            end = ")" + start.group(1) + '"'
            if end not in line[start.end():]:
                closing = end


def main(source, output):
    lines = pathlib.Path(source).read_text(encoding="utf-8").splitlines()
    kept = [(line, code) for line, code in zip(lines, outside_raw_strings(lines))
            if not (code and RTXUI_INCLUDE.match(line))]
    if len(kept) == len(lines):
        print(f"{source}: includes no RTXUI header", file=sys.stderr)
        return 1
    lines = [line for line, _ in kept]

    # After the example's own includes, so all of them precede the import.
    last_include = max((i for i, (line, code) in enumerate(kept)
                        if code and line.startswith("#include")),
                       default=-1)
    insert_at = last_include + 1 if last_include >= 0 else next(
        i for i, line in enumerate(lines) if not line.startswith("//"))
    # <memory>: GCC 15 crashes at -O2 (in nonnull_arg_p) on an importer that
    # includes neither it nor <string>. See docs/guide/modules.md.
    lines[insert_at:insert_at] = [
        "#include <memory>", "#include <rtxui/bind.hpp>", "import rtxui;"]

    out = pathlib.Path(output)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(__doc__, file=sys.stderr)
        sys.exit(2)
    sys.exit(main(sys.argv[1], sys.argv[2]))
