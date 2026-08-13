#!/usr/bin/env python3
# Copyright 2026 Arthur Sonzogni. All rights reserved.
# Use of this source code is governed by the MIT license that can be found in
# the LICENSE file.
"""Refactor example CSS: inline single-use CSS variables across example/*.cpp.

If a CSS variable (--var-name) is declared and referenced via var(--var-name)
only once in an example file, replace var(--var-name) with its literal value
and remove the declaration. Multi-use variables are preserved.
"""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
EXAMPLE_DIR = ROOT / "example"


def refactor_file(cpp_path: Path) -> int:
    content = cpp_path.read_text(encoding="utf-8")
    
    # Match declaration lines: --var-name: value;
    # (Capturing indentation, name, value)
    decl_pattern = re.compile(r"([ \t]*)(--[a-zA-Z0-9_-]+)\s*:\s*([^;\n]+);[ \t]*\n?")
    
    declarations = decl_pattern.findall(content)
    if not declarations:
        return 0

    # Count usages of each variable
    var_usages = {}
    for indent, var_name, val in declarations:
        val_clean = val.strip()
        ref_pattern = re.compile(r"var\(\s*" + re.escape(var_name) + r"\s*\)")
        refs = ref_pattern.findall(content)
        var_usages[var_name] = {
            "value": val_clean,
            "ref_count": len(refs),
            "total_count": content.count(var_name)
        }

    # Identify single-use variables (ref_count == 1)
    to_inline = {
        var_name: info["value"]
        for var_name, info in var_usages.items()
        if info["ref_count"] == 1
    }

    if not to_inline:
        return 0

    modified = content

    # 1. Inline usage sites: var(--var-name) -> value
    for var_name, val in to_inline.items():
        ref_pattern = re.compile(r"var\(\s*" + re.escape(var_name) + r"\s*\)")
        modified = ref_pattern.sub(val, modified)

    # 2. Remove declaration lines
    for indent, var_name, val in declarations:
        if var_name in to_inline:
            # Match line exact
            line_regex = re.compile(r"([ \t]*)" + re.escape(var_name) + r"\s*:\s*" + re.escape(val) + r";[ \t]*\n?")
            modified = line_regex.sub("", modified)

    # 3. Clean up empty selector blocks created by removing declarations
    # e.g. self {\n      }  or  self {\n\n      }
    modified = re.sub(r"([a-zA-Z0-9_#.-]+|\:\:[a-z]+|\&)\s*\{\s*\n\s*\}", "", modified)

    if modified != content:
        cpp_path.write_text(modified, encoding="utf-8")
        return len(to_inline)

    return 0


def main():
    cpp_files = sorted(EXAMPLE_DIR.glob("*.cpp"))
    total_inlined = 0
    modified_files = 0

    for cpp in cpp_files:
        count = refactor_file(cpp)
        if count > 0:
            print(f"Refactored {cpp.name}: inlined {count} single-use CSS variable(s).")
            total_inlined += count
            modified_files += 1

    print(f"\nDone! Inlined {total_inlined} CSS variables across {modified_files} files.")


if __name__ == "__main__":
    main()
