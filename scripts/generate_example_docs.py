#!/usr/bin/env python3
# Copyright 2026 Arthur Sonzogni. All rights reserved.
# Use of this source code is governed by the MIT license that can be found in
# the LICENSE file.
"""Generate individual documentation pages and index for all RTXUI examples.

This script parses header comments from each file under `example/` and
generates:
  1. `docs/guide/examples/<name>.md` for each example, embedding its WASM build
     and transcluding its C++ source code.
  2. `docs/guide/examples.md` referencing all examples categorized by topic with
     links to their individual WASM pages.
"""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
EXAMPLE_DIR = ROOT / "example"
DOCS_DIR = ROOT / "docs"
EXAMPLES_DOC_DIR = DOCS_DIR / "guide" / "examples"
INDEX_MD = DOCS_DIR / "guide" / "examples.md"

CATEGORIES = [
    ("Applications", "Complete applications, showing how individual features compose into full programs."),
    ("Core Concepts", "Components, reactivity, interpolation, conditional rendering, and slots."),
    ("Form Elements", "User input through keyboard and mouse, bound to C++ variables."),
    ("Layout & Box Model", "Positioning terminal cells with flexbox, grid, borders, and margins."),
    ("Scrolling & Overflow", "Handling scrollable regions, focus tracking, and overflow behavior."),
    ("Typography & Styling", "Colors, text decorations, pseudo-classes, transitions, and keyframe animations."),
    ("Components & Advanced Features", "Built-in HTML elements, CJK text, Markdown rendering, spatial nav, and cookbook recipes."),
]

EXAMPLES_META = {
    # Applications
    "app_dashboard.cpp": {"category": "Applications", "guide": "/reactivity", "cols": 100, "rows": 28},
    "app_filebrowser.cpp": {"category": "Applications", "guide": "/guide/scrolling", "cols": 100, "rows": 28},
    "demo.cpp": {"category": "Applications", "guide": "/reactivity", "cols": 110, "rows": 30},
    "playground.cpp": {"category": "Applications", "guide": "/guide/playground", "cols": 120, "rows": 32},

    # Core Concepts
    "helloworld.cpp": {"category": "Core Concepts", "guide": "/guide/hello-world"},
    "counter.cpp": {"category": "Core Concepts", "guide": "/guide/interpolation"},
    "conditional.cpp": {"category": "Core Concepts", "guide": "/guide/conditionals"},
    "loop_simple.cpp": {"category": "Core Concepts", "guide": "/guide/loops"},
    "loop_complex.cpp": {"category": "Core Concepts", "guide": "/guide/loops"},
    "slots.cpp": {"category": "Core Concepts", "guide": "/guide/cpp/slots"},

    # Form Elements
    "input.cpp": {"category": "Form Elements", "guide": "/guide/forms"},
    "textarea.cpp": {"category": "Form Elements", "guide": "/guide/forms"},
    "checkbox.cpp": {"category": "Form Elements", "guide": "/guide/forms"},
    "radio.cpp": {"category": "Form Elements", "guide": "/guide/forms"},
    "label.cpp": {"category": "Form Elements", "guide": "/guide/forms"},
    "slider.cpp": {"category": "Form Elements", "guide": "/guide/forms"},
    "progress.cpp": {"category": "Form Elements", "guide": "/guide/forms"},
    "select.cpp": {"category": "Form Elements", "guide": "/guide/forms"},
    "fieldset.cpp": {"category": "Form Elements", "guide": "/html_reference"},

    # Layout & Box Model
    "layout.cpp": {"category": "Layout & Box Model", "guide": "/guide/css/flexbox"},
    "layout_flex.cpp": {"category": "Layout & Box Model", "guide": "/guide/css/flexbox", "cols": 100, "rows": 28},
    "grid.cpp": {"category": "Layout & Box Model", "guide": "/guide/css/grid"},
    "borders.cpp": {"category": "Layout & Box Model", "guide": "/guide/css/box-model"},
    "border_scroll_demo.cpp": {"category": "Layout & Box Model", "guide": "/guide/css/box-model"},
    "positioning.cpp": {"category": "Layout & Box Model", "guide": "/guide/css/positioning"},
    "sticky.cpp": {"category": "Layout & Box Model", "guide": "/guide/css/positioning"},

    # Scrolling & Overflow
    "focus_scroll.cpp": {"category": "Scrolling & Overflow", "guide": "/guide/scrolling"},
    "horizontal_scroll.cpp": {"category": "Scrolling & Overflow", "guide": "/guide/scrolling"},
    "nested_scroll.cpp": {"category": "Scrolling & Overflow", "guide": "/guide/scrolling"},
    "scroll_behavior.cpp": {"category": "Scrolling & Overflow", "guide": "/guide/scrolling"},
    "anchor.cpp": {"category": "Scrolling & Overflow", "guide": "/guide/scrolling"},

    # Typography & Styling
    "colors.cpp": {"category": "Typography & Styling", "guide": "/guide/typography"},
    "opacity.cpp": {"category": "Typography & Styling", "guide": "/guide/typography"},
    "text_align.cpp": {"category": "Typography & Styling", "guide": "/guide/typography"},
    "text_decoration.cpp": {"category": "Typography & Styling", "guide": "/guide/typography"},
    "pseudo_classes.cpp": {"category": "Typography & Styling", "guide": "/guide/css/animations"},
    "transitions.cpp": {"category": "Typography & Styling", "guide": "/guide/css/animations"},
    "animation.cpp": {"category": "Typography & Styling", "guide": "/guide/css/animations"},

    # Components & Advanced Features
    "tabs.cpp": {"category": "Components & Advanced Features", "guide": "/html_reference"},
    "dialog.cpp": {"category": "Components & Advanced Features", "guide": "/html_reference"},
    "details.cpp": {"category": "Components & Advanced Features", "guide": "/html_reference"},
    "lists.cpp": {"category": "Components & Advanced Features", "guide": "/html_reference"},
    "tooltip.cpp": {"category": "Components & Advanced Features", "guide": "/html_reference"},
    "hr.cpp": {"category": "Components & Advanced Features", "guide": "/html_reference"},
    "table.cpp": {"category": "Components & Advanced Features", "guide": "/html_reference"},
    "cjk.cpp": {"category": "Components & Advanced Features", "guide": "/guide/unicode"},
    "markdown.cpp": {"category": "Components & Advanced Features", "guide": "/guide/markdown", "cols": 100, "rows": 28},
    "media.cpp": {"category": "Components & Advanced Features", "guide": "/guide/css/media-queries"},
    "spatial_navigation.cpp": {"category": "Components & Advanced Features", "guide": "/guide/html/focus"},
    "tabindex.cpp": {"category": "Components & Advanced Features", "guide": "/guide/html/focus"},
    "cookbook_tabs.cpp": {"category": "Components & Advanced Features", "guide": "/guide/cookbook"},
    "cookbook_async.cpp": {"category": "Components & Advanced Features", "guide": "/guide/cookbook"},
    "cookbook_dialog.cpp": {"category": "Components & Advanced Features", "guide": "/guide/cookbook"},
}


def sanitize_tags(text: str) -> str:
    # Replace <tag...> with &amp;lt;tag...&amp;gt; so markdown-it and Vue SFC parser keep them as text entities
    text = re.sub(r'<([a-zA-Z][^>]*)>', r'&amp;lt;\1&amp;gt;', text)
    # Wrap @directives like @keyframes, @media in backticks so Vue parser doesn't treat as v-on
    text = re.sub(r'(?<!`)(@[a-zA-Z0-9_\-]+)(?!`)', r'`\1`', text)
    return text


def parse_header(cpp_file: Path):
    lines = cpp_file.read_text(encoding="utf-8").splitlines()
    desc_lines = []
    idx = 4
    while idx < len(lines) and lines[idx].startswith("//"):
        comment = lines[idx][2:].strip()
        desc_lines.append(comment)
        idx += 1

    if not desc_lines:
        return cpp_file.stem.replace("_", " ").title(), "", ""

    raw_title = desc_lines[0].rstrip(".")
    body_paragraphs = []
    current_p = []

    for l in desc_lines[1:]:
        if l == "":
            if current_p:
                body_paragraphs.append(" ".join(current_p))
                current_p = []
        else:
            current_p.append(l)

    if current_p:
        body_paragraphs.append(" ".join(current_p))

    raw_body = "\n\n".join(body_paragraphs)
    raw_one_liner = " ".join(body_paragraphs[0].split()) if body_paragraphs else raw_title

    title = sanitize_tags(raw_title)
    body = sanitize_tags(raw_body)
    one_liner = sanitize_tags(raw_one_liner)
    return title, body, one_liner


def main():
    EXAMPLES_DOC_DIR.mkdir(parents=True, exist_ok=True)
    cpp_files = sorted(EXAMPLE_DIR.glob("*.cpp"))

    print(f"Generating individual WASM example pages in {EXAMPLES_DOC_DIR}...")
    parsed_examples = []

    for cpp in cpp_files:
        stem = cpp.stem
        meta = EXAMPLES_META.get(cpp.name, {"category": "Components & Advanced Features", "guide": "/guide/examples"})
        title, body, one_liner = parse_header(cpp)
        
        category = meta.get("category", "Components & Advanced Features")
        guide = meta.get("guide", "/guide/examples")
        cols = meta.get("cols", 80)
        rows = meta.get("rows", 24)

        target_name = f"rtxui_example_{stem}"
        wasm_js = f"/wasm/{target_name}.js"
        
        # 1. Write individual example markdown page
        page_md = EXAMPLES_DOC_DIR / f"{stem}.md"
        page_content = f"""# {title}

{body if body else title + '.'}

<ExampleTabs src="{wasm_js}" :cols="{cols}" :rows="{rows}">
<template #source>

<<< @/../example/{cpp.name}

</template>
</ExampleTabs>

---

* Source file: [`example/{cpp.name}`](https://github.com/ArthurSonzogni/RTXUI/blob/main/example/{cpp.name})
* Guide: [Relevant Documentation]({guide})
* [← Back to Examples Index](/guide/examples)
"""
        page_md.write_text(page_content, encoding="utf-8")
        parsed_examples.append({
            "name": cpp.name,
            "stem": stem,
            "title": title,
            "body": body,
            "one_liner": one_liner,
            "category": category,
            "guide": guide,
        })

    # 2. Generate the main examples index page (docs/guide/examples.md)
    print(f"Generating main examples index page at {INDEX_MD}...")
    index_lines = [
        "# Examples Index",
        "",
        "Every example in the repository's [example/](https://github.com/ArthurSonzogni/RTXUI/tree/main/example) "
        "directory, running live in WebAssembly and accompanied by its C++ source code.",
        "",
    ]

    for cat_name, cat_desc in CATEGORIES:
        index_lines.append(f"## {cat_name}")
        index_lines.append("")
        index_lines.append(cat_desc)
        index_lines.append("")
        index_lines.append("| Example | Demonstrates | Guide |")
        index_lines.append("| :--- | :--- | :--- |")

        cat_examples = [e for e in parsed_examples if e["category"] == cat_name]
        for e in cat_examples:
            source_file = e["name"]
            stem = e["stem"]
            desc_text = e["title"]
            if e["one_liner"] and e["one_liner"] != e["title"]:
                desc_text += f". {e['one_liner']}"
            desc_text = desc_text.replace("|", "\\|")
            guide_link = f"[Guide]({e['guide']})" if e["guide"] != "/guide/examples" else "-"
            index_lines.append(
                f"| [{source_file}](/guide/examples/{stem}) | {desc_text} | {guide_link} |"
            )
        index_lines.append("")

    INDEX_MD.write_text("\n".join(index_lines), encoding="utf-8")
    print(f"Successfully generated {len(cpp_files)} individual example pages and updated index page.")


if __name__ == "__main__":
    main()
