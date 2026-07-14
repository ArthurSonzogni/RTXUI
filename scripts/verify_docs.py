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
        
    print("\n[SUCCESS] All CSS properties are documented!")
    sys.exit(0)

if __name__ == "__main__":
    main()
