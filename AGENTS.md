# AGENTS.md

Instructions for coding agents (Claude Code, Gemini, Codex, …) and humans
working on this repository. `CLAUDE.md` and `GEMINI.md` import this file.

## What this is

RTXUI is a C++23 reactive terminal UI library with web-inspired paradigms:
components declare their structure as HTML templates embedded in C++ string
literals, style them with CSS (`<style>` blocks, flexbox/grid/transitions), and
bind C++ members into templates with `{name}` interpolation. Builds with GCC 14+
or Clang 18+; C++26 reflection is used when available. ccache is picked up
automatically.

## Before you push: `scripts/check.sh`

Every change must pass `scripts/check.sh` before it reaches `main`. It runs:

1. clang-format on all tracked C++ files (fix with `tools/format.sh`),
2. the no-exceptions check,
3. clang-tidy on the `.cpp` files changed since `origin/main`,
4. a full build and `ctest` (unit tests, docs/examples/headers verification,
   example smoke test).

`scripts/check.sh --lint-only` runs steps 1–3 only. Claude Code runs the full
check automatically before `git push` (see `.claude/settings.json`) and blocks
the push if it fails. Do not bypass it; fix the cause.

## Rules

These are enforced by `scripts/check.sh` and CI unless noted.

- **Style**: Chromium C++ style, formatted by clang-format.
- **No exceptions**: `try`/`catch` are banned. Use return values, non-throwing
  overloads (e.g. `std::filesystem::last_write_time` with `std::error_code`)
  and parser checks (e.g. `std::from_chars`).
- **clang-tidy clean**: no warnings. A `NOLINT` needs a reason on the same line.
- **Regression tests** (enforced by review): every bug fix comes with a test
  that fails before the fix and passes after, typically in
  `src/rtxui/component/component_test.cpp` or `src/rtxui/style/style_test.cpp`.
- **Docs stay in sync**: every CSS property handled in
  `src/rtxui/style/apply_style.cpp` (each `p == "property-name"` branch) must
  be documented in `docs/css_reference.md` (`scripts/verify_docs.py`). When
  adding HTML tags or component features, update `docs/html_reference.md`,
  `docs/reactivity.md` or the relevant guide under `docs/guide/`.
- **No silent no-ops**: constructs that parse but do nothing are reported
  through `ReportDiagnostic` (`include/rtxui/diagnostic.hpp`, see
  `docs/guide/diagnostics.md`). Each property branch in `ApplyStyle` must
  `return` once it has applied a supported value; falling off the end reports
  the declaration as unsupported. Examples run under `RTXUI_STRICT=1`, which
  aborts on any diagnostic.
- **Commits**: conventional commits (`feat:`, `fix:`, `docs:`, `build:`, …).

## Commands

The build directory is `build/` (Ninja, Release, tests + examples ON).

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DRTXUI_BUILD_TESTS=ON \
  -DRTXUI_BUILD_EXAMPLES=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
ninja -C build -j2               # incremental rebuild
ninja -C build -j2 rtxui_test    # build only the test binary
```

### Tests (Catch2)

```bash
./build/rtxui_test                          # run all tests
./build/rtxui_test "some test name"         # run one test by name
./build/rtxui_test "[style]"                # run by tag
./build/rtxui_test --list-tests             # discover names
ctest --test-dir build                      # everything, incl. verify_* scripts
```

Test sources live next to the code they test (`*_test.cpp`). New test files
must be added to the `rtxui_test` target in `CMakeLists.txt`; new library
sources to `rtxui_lib`.

### Other builds

- Coverage: `scripts/coverage.sh [--html]` builds `build_coverage/` with
  Clang source-based coverage, runs `rtxui_test`, and prints line coverage per
  directory. Use it to find untested code; there is no threshold, so write
  tests that assert behavior rather than tests that only execute lines.
- FuzzTest fuzzers: configure with `-DRTXUI_BUILD_FUZZERS=ON
  -DFUZZTEST_FUZZING_MODE=ON` and run `./build_fuzz/rtxui_fuzzer`. Turn every
  crash into a regression test.
- Emscripten/WASM build of examples into `docs/public/wasm/` via
  `emcmake cmake .. -DCMAKE_BUILD_TYPE=Release -DRTXUI_BUILD_EXAMPLES=ON`.
- Examples build as `build/rtxui_example_<name>` (interactive TUIs — they take
  over the terminal; drive them through a PTY, as `scripts/smoke_examples.py`
  does).

## Architecture

The rendering pipeline mirrors a browser engine. A frame flows through these
stages, each in its own directory under `src/rtxui/`:

1. **Component** (`component/`) — user components inherit
   `rtxui::Component<Derived>` (CRTP, `include/rtxui/internal/component.hpp`).
   The HTML template is either a `std::string_view view` member or a
   `std::string_view Setup()` method (non-virtual, no `override`), never both;
   `GetView()` finds them at compile time. Members are
   registered via `Bind(member)` in the constructor: variables become reactive
   state, `const` methods become computed values, methods become
   `onclick`-style callbacks. Change detection is snapshot-based: bound state
   is compared each frame and the DOM is patched on change.
2. **XML parse** (`xml/`) — parses the `view` template.
3. **DOM** (`dom/`) — `Element` tree; `slot_element` implements `<slot>`
   content projection; `text_element` holds interpolated text.
4. **Style** (`style/`) — `style.cpp` parses CSS (selectors, combinators,
   pseudo-classes, transitions); `apply_style.cpp` maps each property name onto
   `ComputedStyle`.
5. **Layout** (`layout/`) — `layout_tree_builder` builds boxes from the DOM,
   `layout.cpp` runs block/inline/flex/grid layout producing physical
   fragments.
6. **Paint** (`paint/`) — fragments are painted into a `Texture` of `Cell`s
   (glyph + color + decoration).
7. **Terminal** (`terminal/`) — `Screen` diffs textures to ANSI output and
   parses terminal input into events. App entry point:
   `Ref<MyComponent>::New()` → `Screen screen(app); screen.Loop();`.

Supporting layers: `base/` (task loop / `TaskRunner` used for scheduling,
thread-safe wakeup; `Ref<T>`/`RefCounted` smart pointers), `geometry/`
(logical vs physical coordinates), `markdown/` (the `<markdown>` component's
parser).

### Key mechanisms to know

- **Built-in components** (`src/rtxui/component/default/<tag>/`) — every HTML
  tag (`<button>`, `<input>`, `<dialog>`, …) is itself an RTXUI component
  defined with its own `view` template using `<slot>` and a `self` CSS
  selector. To add a tag: create the dir, register it in
  `default_components_internal.hpp`, and add the .cpp to `CMakeLists.txt`.
- **WHOLE_ARCHIVE wrapper** — the public `rtxui` target wraps `rtxui_lib` with
  `$<LINK_LIBRARY:WHOLE_ARCHIVE,...>` so nothing gets dropped by the linker;
  link examples/apps against `rtxui`, tests against `rtxui_lib`.
- **C++26 reflection (optional)** — CMake probes for `<meta>` support
  (`-freflection-latest` on Clang, `-freflection` on GCC) and defines
  `RTXUI_HAS_REFLECTION`, which enables automatic member binding without
  `Bind()` calls. Code must still work without it.
- **Hot reload** — `EnableHotReload()` uses `std::source_location` to watch the
  component's own source file and re-parse the `view` template at runtime
  without recompiling.
- **Public vs internal headers** — the installed API is `include/rtxui/`
  (`#include <rtxui/rtxui.hpp>`); everything under `src/` is internal but on
  the include path for the library and tests.

## Docs

`docs/` is a VitePress site (deployed by `.github/workflows/deploy-docs.yml`)
with per-feature guides under `docs/guide/`, plus `css_reference.md` /
`html_reference.md` / `reactivity.md`.
