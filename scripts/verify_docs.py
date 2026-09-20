#!/usr/bin/env python3
# Copyright 2026 Arthur Sonzogni. All rights reserved.
# Use of this source code is governed by the MIT license that can be found in
# the LICENSE file.
import re
import sys
from pathlib import Path

# Paths
ROOT = Path(__file__).resolve().parents[1]
APPLY_STYLE = ROOT / "src/rtxui/style/apply_style.cpp"
CSS_DOCS = ROOT / "docs/css_reference.md"
COMPONENTS_DIR = ROOT / "src/rtxui/component/default"
HTML_DOCS = ROOT / "docs/html_reference.md"

NATIVE_HTML_TAGS = {
    "table", "thead", "tbody", "tfoot", "tr", "th", "td", "slot", "template",
    "for", "if"
}

def extract_code_css_properties():
    if not APPLY_STYLE.exists():
        print(f"[ERROR] Could not find {APPLY_STYLE}", file=sys.stderr)
        sys.exit(1)
        
    content = APPLY_STYLE.read_text(encoding="utf-8")
    # Matches strings in: if (p == "property-name")
    pattern = r'p\s*==\s*"(?P<prop>[a-zA-Z0-9\-]+)"'
    matches = re.finditer(pattern, content)
    return set(m.group("prop") for m in matches)

def check_docs_contain_properties(props):
    if not CSS_DOCS.exists():
        print(f"[ERROR] Could not find {CSS_DOCS}", file=sys.stderr)
        sys.exit(1)
        
    doc_text = CSS_DOCS.read_text(encoding="utf-8").lower()
    missing = []
    for prop in props:
        # Documented either as inline code (`prop`) or as a property card
        # (<CssProperty name="prop" ...).
        if (f"`{prop}`" not in doc_text
                and f'<cssproperty name="{prop}"' not in doc_text):
            missing.append(prop)
    return missing

def extract_code_border_styles():
    """Every keyword ParseBorderStyle accepts, aliases included.

    Property names are only half the documented surface: a property taking a
    fixed set of keywords is not really documented until the keywords are
    listed too. Border styles are the largest such set and the one users cannot
    guess, since the keyword says nothing about which glyphs it draws.
    """
    content = APPLY_STYLE.read_text(encoding="utf-8")
    body = content[content.index("std::optional<BorderStyle> ParseBorderStyle"):]
    body = body[:body.index("\n}\n")]
    # Matches: if (v == "name") {  and  if (v == "name" || v == "alias") {
    return set(m.group("kw")
               for m in re.finditer(r'v\s*==\s*"(?P<kw>[a-z0-9\-]+)"', body))

def check_docs_contain_values(values):
    doc_text = CSS_DOCS.read_text(encoding="utf-8").lower()
    return [v for v in values if f"`{v}`" not in doc_text]

def extract_code_html_tags():
    if not COMPONENTS_DIR.exists():
        print(f"[ERROR] Could not find {COMPONENTS_DIR}", file=sys.stderr)
        sys.exit(1)

    tags = set(NATIVE_HTML_TAGS)
    pattern = r'RegisterGlobalComponent\(\s*"(?P<tag>[a-zA-Z0-9\-]+)"'
    for file_path in COMPONENTS_DIR.rglob("*.cpp"):
        content = file_path.read_text(encoding="utf-8")
        for match in re.finditer(pattern, content):
            tags.add(match.group("tag"))
    return tags

def check_docs_contain_html_tags(tags):
    if not HTML_DOCS.exists():
        print(f"[ERROR] Could not find {HTML_DOCS}", file=sys.stderr)
        sys.exit(1)

    doc_text = HTML_DOCS.read_text(encoding="utf-8")
    missing = []
    for tag in sorted(tags):
        # A tag is considered documented if referenced as an HTML element:
        # `<tag>`, `<tag `, `<tag.`, or `` `<tag>` ``
        if (f"<{tag}>" not in doc_text
                and f"<{tag} " not in doc_text
                and f"<{tag}." not in doc_text
                and f"`<{tag}>`" not in doc_text):
            missing.append(tag)
    return missing

def main():
    print("Extracting CSS properties from apply_style.cpp...")
    code_props = extract_code_css_properties()
    print(f"Found {len(code_props)} CSS properties: {', '.join(sorted(code_props))}")

    print("Checking docs/css_reference.md...")
    missing = check_docs_contain_properties(code_props)

    if missing:
        print("\n[ERROR] The following CSS properties are not documented in css_reference.md:", file=sys.stderr)
        for p in missing:
            print(f"  - {p}", file=sys.stderr)
        sys.exit(1)

    print("Extracting border-style keywords from apply_style.cpp...")
    code_styles = extract_code_border_styles()
    print(f"Found {len(code_styles)} border styles: {', '.join(sorted(code_styles))}")

    missing_styles = check_docs_contain_values(code_styles)
    if missing_styles:
        print("\n[ERROR] The following border styles are not documented in css_reference.md:", file=sys.stderr)
        for v in sorted(missing_styles):
            print(f"  - {v}", file=sys.stderr)
        sys.exit(1)

    print("Extracting HTML tags and component registrations...")
    html_tags = extract_code_html_tags()
    print(f"Found {len(html_tags)} HTML elements/components: {', '.join(sorted(html_tags))}")

    print("Checking docs/html_reference.md...")
    missing_tags = check_docs_contain_html_tags(html_tags)
    if missing_tags:
        print("\n[ERROR] The following HTML tags are not documented in html_reference.md:", file=sys.stderr)
        for tag in sorted(missing_tags):
            print(f"  - <{tag}>", file=sys.stderr)
        sys.exit(1)

    print("\n[SUCCESS] All CSS properties, border styles, and HTML tags are fully documented!")
    sys.exit(0)

if __name__ == "__main__":
    main()

