# RTXUI

[![Test](https://github.com/ArthurSonzogni/RTXUI/actions/workflows/test.yml/badge.svg)](https://github.com/ArthurSonzogni/RTXUI/actions/workflows/test.yml)
[![Fuzz](https://github.com/ArthurSonzogni/RTXUI/actions/workflows/fuzz.yml/badge.svg)](https://github.com/ArthurSonzogni/RTXUI/actions/workflows/fuzz.yml)
[![Docs](https://github.com/ArthurSonzogni/RTXUI/actions/workflows/deploy-docs.yml/badge.svg)](https://arthursonzogni.github.io/RTXUI/)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)

**A browser engine for your terminal, in C++.**

Write the interface in HTML. Style it with real CSS: flexbox, grid, media
queries, transitions, keyframe animations, `:hover`, `:focus`. Bind it to
plain C++ members, change a member, and the screen follows. Save the file
and the running app reloads its template, with no recompile.

*From the author of [FTXUI](https://github.com/ArthurSonzogni/FTXUI).*

https://github.com/user-attachments/assets/b270494a-074c-4ddf-b34e-7c7fdf530f03

<p align="center">
  <a href="https://arthursonzogni.github.io/RTXUI/guide/examples/showcase"><b>▶ Run this exact app in your browser</b></a>
  &nbsp;·&nbsp;
  <a href="example/showcase.cpp">Read its source (one file)</a>
  &nbsp;·&nbsp;
  <a href="https://arthursonzogni.github.io/RTXUI/">Documentation</a>
</p>

---

## The idea

Terminal UI libraries make you lay out boxes by hand, or learn yet another
layout DSL. The web solved layout long ago, and most developers already know
the solution. RTXUI brings it to the terminal, cell for cell:

```cpp
#include <rtxui/rtxui.hpp>

class Counter : public rtxui::Component<Counter> {
 public:
  int count = 0;
  void Increment() { count++; }

  Counter() {
    Bind(count);
    Bind(Increment);
    EnableHotReload();  // Edit the template below while the app runs.
  }

  std::string_view view = R"html(
    <div class="card">
      <h1>Count: {count}</h1>
      <button onclick="Increment">Increment</button>
    </div>
    <style>
      .card { border: round; border-color: cyan; padding: 1 2; }
      h1 { color: yellow; margin-bottom: 1; }
      button { border: solid; padding: 0 1; transition: all 0.2s ease; }
      button:hover, button:focus { border-color: green; color: green; }
    </style>
  )html";
};

int main() {
  rtxui::Screen screen(rtxui::Ref<Counter>::New());
  screen.Loop();
}
```

That is the whole program. The mouse works, Tab moves focus, the button
fades to green on hover, and the count updates when you click.

## What's in the box

https://github.com/user-attachments/assets/10d75224-eb4d-4e71-8d53-2e647028c923

**Layout, the way the web does it**
- Flexbox, with `gap`, `wrap`, `grow`/`shrink`/`basis` and alignment on both axes.
- Grid: `repeat(auto-fill, minmax(…))`, named lines and areas, `grid-auto-flow: dense`.
- `position: relative | absolute | fixed | sticky`, `translate`, `z-index`.
- Scroll containers with real scrollbars, `scroll-behavior: smooth`, nested scrolling.
- `@media (max-width: …)` to rearrange the interface as the terminal resizes.
- `calc()`, `min()`, `max()`, `clamp()`, custom properties with `var()`.
- Tables with `colspan`, `rowspan` and `border-collapse`.

**Motion**
- `transition` on colors, sizes, `opacity` and `translate`, with every easing curve.
- `@keyframes` animations: delays, `steps()`, `alternate`, `infinite`, `animation-play-state`.
- Frames that only change colors or positions reuse the last layout, so animations stay cheap.

**Interaction**
- Mouse: hover, click, drag, wheel, all hit-tested against the real layout.
- Keyboard: Tab order, `tabindex`, arrow-key spatial navigation, `:focus-visible`.
- Select text with the mouse, double-click for a word, triple-click for a line, then copy.
- Clickable `<a href>` links, through the terminal's OSC 8 hyperlinks.

**Batteries included**
- `<input>`, `<textarea>`, `<select>`, `<checkbox>`, `<radio>`, `<slider>`, `<progress>`.
- `<tabs>`, `<details>`, `<dialog>`, `<toast>`, `<tooltip>`, `<table>`, tree items.
- `<markdown>`, rendered and styled with CSS.
- `<for virtual="">`: lists of 100,000 rows that only lay out what is on screen.

**Plain C++**
- Bind members, methods and collections of structs. No code generator, no macros to learn.
- `<if>`/`<elif>`/`<else>`, `<for>`, slots and props for your own components.
- Timers and worker threads post back to the UI thread with `PostTask()`.
- Optional C++26 reflection binds struct fields for you, on compilers that have it.

**A fast development loop**
- Hot reload: save the C++ file, and the running app picks up the new HTML and CSS.
- Errors in templates and styles point at the line and column, like a compiler does.
- Inline mode draws below the prompt, like `cargo` or `apt`, instead of taking the whole screen.
- The same code builds to WebAssembly: every example in the docs runs live in the browser.

**Unicode done right**
- CJK and other wide characters, combining marks and emoji are laid out by display width.

## Where it runs

| Platform | Compilers tested in CI |
| :--- | :--- |
| Linux | GCC 14, GCC 16, Clang 20 |
| macOS | AppleClang |
| Windows | MSVC, clang-cl, MinGW-w64 GCC |
| Browser | Emscripten (WebAssembly) |

It needs C++23 and CMake 3.24 or newer, and depends on nothing beyond the
standard library.

## Built to be trusted

- 800+ tests, also run under AddressSanitizer and UndefinedBehaviorSanitizer.
- 8 fuzz targets: the template, CSS, selector, Markdown and terminal input
  parsers, layout and paint, input events, and a whole app over time. The app
  fuzzer checks every frame drawn from a cached layout against a fresh one, so
  a stale cache fails loudly instead of drawing a plausible wrong frame.
- A snapshot of every example's first screen, compared on every change.
- The shared library's ABI is checked on every change, and the public API is
  kept small: implementation details stay behind pimpl.

## Get started

Add it to your `CMakeLists.txt`:

```cmake
include(FetchContent)
FetchContent_Declare(
  rtxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/RTXUI.git
  GIT_TAG        main
)
FetchContent_MakeAvailable(rtxui)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE rtxui)
```

`find_package(rtxui)` and `pkg-config rtxui` work too, after
`cmake --install`. See the
[getting started guide](https://arthursonzogni.github.io/RTXUI/guide/getting-started).

Then browse the [examples](https://arthursonzogni.github.io/RTXUI/guide/examples):
66 of them, each running live in your browser next to its source.

## RTXUI or FTXUI?

Both are by the same author, and make different bets.

- **[FTXUI](https://github.com/ArthurSonzogni/FTXUI)** builds interfaces from
  C++ function calls (`hbox`, `vbox`, `border`). It is mature, small, and
  checked by the compiler. Pick it for a minimal footprint, or for C++17.
- **RTXUI** runs a browser-style pipeline: HTML templates, the CSS cascade,
  and reactive state diffed into a DOM. Pick it if you think in web layouts,
  want grid, animations and hot reload, or want the same app in a terminal and
  in a browser.

## Building from source

```bash
cmake -B build -G Ninja -DRTXUI_BUILD_EXAMPLES=ON -DRTXUI_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
./build/rtxui_example_showcase
```

## Contributing

Contributions are welcome. The [contributing guide](CONTRIBUTING.md) covers
the coding style (Chromium), the rules (no exceptions, a regression test with
every fix), and formatting.

## License

MIT. See [LICENSE](LICENSE).
