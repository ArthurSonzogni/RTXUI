#!/usr/bin/env python3
# Copyright 2026 Arthur Sonzogni. All rights reserved.
# Use of this source code is governed by the MIT license that can be found in
# the LICENSE file.
"""Generates docs/public/llms.txt and docs/public/llms-full.txt.

llms.txt (https://llmstxt.org) is an index of the documentation for language
models: a summary, then one link per page, in sidebar order. llms-full.txt is
every page concatenated as plain Markdown, so a model can be given the whole
manual in one file. Both are derived from the VitePress sidebar, so a new page
appears in them without anyone remembering to add it.

Vue components are flattened: <CssProperty> cards become list items, and
embedded example sources become a link to the file instead of its full text.

Usage: python3 scripts/generate_llms_txt.py
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DOCS = ROOT / "docs"
SITE = "https://arthursonzogni.github.io/RTXUI"
SOURCE = "https://github.com/ArthurSonzogni/RTXUI/blob/main"

SUMMARY = """\
# RTXUI

> RTXUI is a C++23 library for terminal user interfaces. A component is a C++
> class whose HTML template and CSS live in a string member; bound members are
> interpolated with `{name}`, and the interface updates when they change.

Before writing RTXUI code, read "Common Mistakes": RTXUI templates are not Vue
or React. `{}` holds a bound name, never an expression; conditions and loops
use `if="{name}"` and `<for each="{list}" as="item">`; handlers are
`onclick="Method"`; CSS lengths are terminal cells. Run an app with
`RTXUI_STRICT=1` to fail on anything RTXUI would silently ignore, and with
`RTXUI_HEADLESS=80x24` to print its screen as text.
"""


def sidebar():
    """Returns [(section, [(title, link), ...]), ...] from the config."""
    config = (DOCS / ".vitepress" / "config.ts").read_text(encoding="utf-8")
    body = config[config.index("sidebar:"):]
    sections = []
    for match in re.finditer(
            r"text: '([^']+)',\s*(?:collapsed: \w+,\s*)?items: \[(.*?)\]",
            body, re.S):
        items = re.findall(r"\{ text: '([^']+)', link: '([^']+)' \}",
                           match.group(2))
        sections.append((match.group(1), items))
    return sections


def page_path(link):
    path = DOCS / (link.strip("/") + ".md")
    return path if path.exists() else None


def flatten(markdown):
    """Turns VitePress-specific markup into plain Markdown."""
    out = []
    in_code = False
    for line in markdown.splitlines():
        if line.startswith("```"):
            in_code = not in_code
        if in_code:
            out.append(line)
            continue
        include = re.match(r"<<< @/\.\./(\S+)", line)
        if include:
            path = include.group(1)
            out.append(f"Example source: [{path}]({SOURCE}/{path})")
            continue
        prop = re.match(r'<CssProperty name="([^"]+)"(.*?)/>', line)
        if prop:
            attrs = dict(re.findall(r'(\w+)="([^"]*)"', prop.group(2)))
            values = attrs.get("values", "")
            out.append(f"- `{prop.group(1)}`: `{values}`. "
                       f"{attrs.get('description', '')}".rstrip())
            continue
        # The wrapper around embedded examples, not RTXUI's own <template.x>.
        if re.match(r"<ExampleTabs\b|</ExampleTabs>|<template #source>|"
                    r"</template>$", line):
            continue
        out.append(line)
    text = "\n".join(out)
    return re.sub(r"\n{3,}", "\n\n", text).strip() + "\n"


def main():
    index = [SUMMARY]
    full = [SUMMARY]
    for section, items in sidebar():
        index.append(f"\n## {section}\n")
        for title, link in items:
            path = page_path(link)
            if not path:
                continue
            index.append(f"- [{title}]({SITE}{link}.html)")
            full.append(f"\n---\n\n<!-- {SITE}{link}.html -->\n\n"
                        + flatten(path.read_text(encoding="utf-8")))
    (DOCS / "public" / "llms.txt").write_text(
        "\n".join(index) + "\n", encoding="utf-8")
    (DOCS / "public" / "llms-full.txt").write_text(
        "\n".join(full), encoding="utf-8")
    print(f"Wrote docs/public/llms.txt ({len(index)} lines) and "
          f"docs/public/llms-full.txt.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
