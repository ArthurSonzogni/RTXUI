# Modules

> [!WARNING]
> Experimental. The module's contents may change between releases. If
> something you need is missing or does not compile, please open an issue.

RTXUI can be used as a
[C++ module](https://en.cppreference.com/w/cpp/language/modules):
`import rtxui;` instead of `#include <rtxui/rtxui.hpp>`.

## Building the module

Turn on `RTXUI_BUILD_MODULE`. It needs CMake 3.28 or newer and the Ninja (or
Visual Studio) generator: Makefile generators cannot build modules.

```sh
cmake -B build -G Ninja -DRTXUI_BUILD_MODULE=ON
cmake --build build
```

Link your target against `rtxui::module`:

```cmake
cmake_minimum_required(VERSION 3.28)  # Scans your sources for `import`.
# ...
target_link_libraries(my_app PRIVATE rtxui::module)
```

If your project has to declare an older minimum version, CMake does not look
for `import` in your sources. Turn it on for the target instead:
`set_target_properties(my_app PROPERTIES CXX_SCAN_FOR_MODULES ON)`.

## Using it

Three things differ from including the headers:

1. **`Bind()` comes from `<rtxui/bind.hpp>`.** It is a macro, and a module
   cannot export macros.
2. **Include the standard headers you use.** `<rtxui/rtxui.hpp>` brings in
   `<string>`, `<vector>` and others, and code often counts on that without
   noticing. The module makes none of them visible.
3. **Put every `#include` before `import rtxui;`.** GCC requires a header to
   be included before importing a module that also includes it.

<!-- snippet: fragment -->
```cpp
#include <string>
#include <string_view>
#include <rtxui/bind.hpp>
import rtxui;

using namespace rtxui;

class Counter : public Component<Counter> {
 public:
  int count = 0;
  void Increment() { count++; }

  Counter() {
    Bind(count);
    Bind(Increment);
  }

  std::string_view view = R"html(
    <button onclick="Increment">Clicked {count} times</button>
  )html";
};

int main() {
  auto app = Ref<Counter>::New();
  Screen screen(app);
  screen.Loop();
}
```

Every example in `example/` is also built this way and checked against its
snapshot: see `cmake/modules.cmake`.

## What the module exports

`Component`, `Ref`, `Screen`, `Event`, `FieldSink`, `PostTask`,
`PostDelayedTask`, `Diagnostic` and `SetDiagnosticHandler`. That is what the
examples use, and engine internals stay out. `HeadlessScreen` and the other
testing helpers are not exported yet: test with the headers for now.

## Known issues

- **GCC 15 crashes** ("internal compiler error: in nonnull_arg_p") at `-O2`
  and above on a file that imports `rtxui` but includes neither `<string>`
  nor `<memory>`. Include `<memory>` before the import.

- **GCC 16 with C++26 reflection**: the 2026-03-22 snapshot of GCC 16 rejects
  an importer that includes `<string>` when `-freflection` is on ("conflicting
  imported declaration"), even with a module that has nothing to do with
  RTXUI. RTXUI turns reflection on when the compiler has it, so on that
  compiler, use the headers.
