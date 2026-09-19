# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

RTXUI is a C++26 reactive terminal UI library with web-inspired paradigms: components declare their structure as HTML templates embedded in C++ string literals, style them with CSS (`<style>` blocks, flexbox/grid/transitions), and bind C++ members into templates with `{name}` interpolation. Requires a recent Clang (Clang 18+); ccache is picked up automatically.

## Commands

All paths relative to the repo root. The build directory is `build/` (Ninja, Release, tests + examples ON).

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DRTXUI_BUILD_TESTS=ON -DRTXUI_BUILD_EXAMPLES=ON
ninja -C build                  # incremental rebuild after configuration
ninja -C build rtxui_test       # build only the test binary
```

### Tests (Catch2)

```bash
./build/rtxui_test                          # run all tests
./build/rtxui_test "some test name"         # run one test by name
./build/rtxui_test "[style]"                # run by tag
./build/rtxui_test --list-tests             # discover names
ctest --test-dir build                      # via CTest (includes verify_css_docs)
```

Test sources live next to the code they test (`*_test.cpp`, e.g. `src/rtxui/component/component_test.cpp`). New test files must be added to the `rtxui_test` target in `CMakeLists.txt`; likewise new library sources must be added to `rtxui_lib`.

**Always add a regression test for every bug fixed** (per the project's engineering standards in GEMINI.md), typically in `component_test.cpp` or `style_test.cpp`.

### Docs consistency check

Every CSS property handled in `src/rtxui/style/apply_style.cpp` (each `p == "property-name"` branch) must be documented in `docs/css_reference.md`. Enforced by `python3 scripts/verify_docs.py` (also runs as the `verify_css_docs` ctest).

### Other builds

- FuzzTest fuzzers: configure with `-DRTXUI_BUILD_FUZZERS=ON -DFUZZTEST_FUZZING_MODE=ON` and run `./build_fuzz/rtxui_fuzzer`.
- Emscripten/WASM build of examples into `docs/public/wasm/` via `emcmake cmake .. -DCMAKE_BUILD_TYPE=Release -DRTXUI_BUILD_EXAMPLES=ON`.
- Examples build as `build/rtxui_example_<name>` (interactive TUIs — they take over the terminal).

## Architecture

The rendering pipeline mirrors a browser engine. A frame flows through these stages, each in its own directory under `src/rtxui/`:

1. **Component** (`component/`) — user components inherit `rtxui::Component<Derived>` (CRTP, `include/rtxui/internal/component.hpp`). The HTML template is a `std::string_view view` member. Members are registered via `Bind(member)` in the constructor: variables become reactive state, `const` methods become computed values, methods become `onclick`-style callbacks. Change detection is snapshot-based: bound state is compared each frame and the DOM is patched on change.
2. **XML parse** (`xml/`) — parses the `view` template.
3. **DOM** (`dom/`) — `Element` tree; `slot_element` implements `<slot>` content projection; `text_element` holds interpolated text.
4. **Style** (`style/`) — `style.cpp` parses CSS (selectors, combinators, pseudo-classes, transitions); `apply_style.cpp` maps each property name onto `ComputedStyle`.
5. **Layout** (`layout/`) — `layout_tree_builder` builds boxes from the DOM, `layout.cpp` runs block/inline/flex/grid layout producing physical fragments.
6. **Paint** (`paint/`) — fragments are painted into a `Texture` of `Cell`s (glyph + color + decoration).
7. **Terminal** (`terminal/`) — `Screen` diffs textures to ANSI output and parses terminal input into events. App entry point: `Ref<MyComponent>::New()` → `Screen screen(app); screen.Loop();`.

Supporting layers: `base/` (task loop / `TaskRunner` used for scheduling, thread-safe wakeup; `Ref<T>`/`RefCounted` smart pointers), `geometry/` (logical vs physical coordinates), `markdown/` (the `<markdown>` component's parser).

### Key mechanisms to know

- **Built-in components** (`src/rtxui/component/default/<tag>/`) — every HTML tag (`<button>`, `<input>`, `<dialog>`, …) is itself an RTXUI component defined with its own `view` template using `<slot>` and a `self` CSS selector. To add a tag: create the dir, register it in `default_components_internal.hpp`, and add the .cpp to `CMakeLists.txt`.
- **WHOLE_ARCHIVE wrapper** — the public `rtxui` target wraps `rtxui_lib` with `$<LINK_LIBRARY:WHOLE_ARCHIVE,...>` so nothing gets dropped by the linker; link examples/apps against `rtxui`, tests against `rtxui_lib`.
- **C++26 reflection (optional)** — CMake probes for `<meta>` support (`-freflection-latest` on Clang, `-freflection` on GCC) and defines `RTXUI_HAS_REFLECTION`, which enables automatic member binding without `Bind()` calls. Code must still work without it.
- **Hot reload** — `EnableHotReload()` uses `std::source_location` to watch the component's own source file and re-parse the `view` template at runtime without recompiling.
- **Public vs internal headers** — the installed API is `include/rtxui/` (`#include <rtxui/rtxui.hpp>`); everything under `src/` is internal but on the include path for the library and tests.

## Docs

`docs/` is a VitePress site (deployed by `.github/workflows/deploy-docs.yml`) with per-feature guides under `docs/guide/`, plus `css_reference.md` / `html_reference.md` / `reactivity.md`. When adding CSS properties, HTML tags, or component features, update the corresponding reference doc (the CSS one is CI-enforced, see above).
