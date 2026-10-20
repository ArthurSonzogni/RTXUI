# Compatibility

RTXUI is in its 0.x series: the API is still settling. This page says what
each kind of release may change, so you know what an upgrade can break.

## Versions

| Release | API (source) | ABI (binary) | Templates & CSS |
| :--- | :--- | :--- | :--- |
| `0.Y.Z` → `0.Y.Z+1` (patch) | kept | kept | bug fixes only |
| `0.Y` → `0.Y+1` (minor) | may break | may break | may change |
| `X` → `X+1` (major, from 1.0) | may break | may break | may change |

From 1.0, minor releases add to the API and ABI without breaking them.

**ABI.** The shared library's soname carries the series: `librtxui.so.0.1`
during 0.1, then `librtxui.so.0.2`. An app built against one minor release
never loads another by accident. Within a series, CI compares each patch
release with the one before it (abidiff), and fails on an incompatible
change. A static build has no ABI to keep: rebuild your app when you upgrade.

**API.** Code that compiles against `0.Y.Z` compiles against `0.Y.Z+1`. A
breaking change in a minor release is listed in the release notes, with what
to write instead.

**Templates and CSS.** What a template means is API too: tag and attribute
names, `{...}` interpolation, `<for>` and `<if>`, and the CSS properties and
how they lay out. A patch release only fixes bugs here. Making a property
behave like a browser's counts as a fix, and can still move a layout that
relied on the old behavior; such changes are listed in the release notes.
Run your app headless (see [Headless Rendering](/guide/headless)) and diff
its screens to see what an upgrade changes.

## What is covered

The public API is what `<rtxui/rtxui.hpp>` declares in the `rtxui`
namespace, as documented in the [C++ API](/cpp_api) reference.

Not covered, and free to change in any release:

- The `rtxui::internal` namespace, and the headers under
  `include/rtxui/internal/`. Public headers need them, but your code should
  not use them directly.
- Anything under `src/`, which is not installed.
- Private members of public classes.
- The text of diagnostics, and the order in which they are reported. Their
  [kind](/guide/diagnostics) is covered.
- The exact text `Print()` produces for events, meant for logs.

## Extending a component

A component is a class deriving from `rtxui::Component<T>`, and so from
`ComponentBase`, whose virtual functions are part of the ABI. Override only
the documented ones: `OnEvent()` and `Digest()`. `ComponentBase` keeps spare
virtual slots, so a patch release could add an overridable hook without
breaking components already compiled.
