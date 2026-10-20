#!/usr/bin/env python3
"""Checks that every file naming RTXUI's version names the same one.

    scripts/verify_version.py           # the files agree
    scripts/verify_version.py v0.1.0    # ... and match this release tag
"""

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def main() -> int:
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    conan = (ROOT / "conanfile.py").read_text(encoding="utf-8")
    bazel = (ROOT / "MODULE.bazel").read_text(encoding="utf-8")
    vcpkg = json.loads((ROOT / "packaging/vcpkg/rtxui/vcpkg.json")
                       .read_text(encoding="utf-8"))
    versions = {
        "CMakeLists.txt":
            re.search(r"project\(rtxui\b[^)]*\bVERSION\s+(\S+)", cmake).group(1),
        "conanfile.py":
            re.search(r'^\s*version\s*=\s*"([^"]+)"', conan, re.M).group(1),
        "packaging/vcpkg/rtxui/vcpkg.json": vcpkg["version-semver"],
        "MODULE.bazel":
            re.search(r'module\([^)]*\bversion\s*=\s*"([^"]+)"', bazel).group(1),
    }
    if len(sys.argv) > 1:
        versions["release tag"] = sys.argv[1].removeprefix("v")

    if len(set(versions.values())) == 1:
        return 0
    print("RTXUI's version differs between:", file=sys.stderr)
    for where, version in versions.items():
        print(f"  {version:10} {where}", file=sys.stderr)
    return 1


if __name__ == "__main__":
    sys.exit(main())
