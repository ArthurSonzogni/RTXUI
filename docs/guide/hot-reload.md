# HTML/CSS Hot-Reloading

RTXUI can re-read a component's HTML template and `<style>` block from its C++
source file while the application is running, updating the live interface
without recompiling.

## How It Works

`EnableHotReload()` records which source file defines the component, using
`std::source_location`, and which member variable holds its template. While
the application runs, the event loop polls the file's modification time; when
the file is saved, the
raw string literal is re-extracted from the C++ source, re-parsed, and the
component's DOM subtree is rebuilt and re-bound.

Only the template text is reloaded — the compiled C++ stays as it was. Changes
to methods, bindings, event handlers, or member variables still require a
rebuild.

## Usage

Call `EnableHotReload()` in the component's constructor:

```cpp
#include <rtxui/rtxui.hpp>

class MyPanel : public rtxui::Component<MyPanel> {
 public:
  std::string_view view = R"html(
    <div class="panel">
      <h1>Live Panel</h1>
      <p>Modify this text, save the file, and watch the terminal update.</p>
    </div>
    <style>
      .panel {
        border: heavy;
        border-color: rgb(59, 130, 246);
        padding: 1;
      }
      h1 { color: rgb(234, 179, 8); }
    </style>
  )html";

  MyPanel() {
    EnableHotReload();
  }
};
```

By default the template is assumed to live in a member named `view`. If yours
is named differently, pass the name:

```cpp
EnableHotReload("my_custom_view");
```

An overload also accepts an explicit file path, for cases where
`std::source_location` does not point at the file you want watched:

```cpp
EnableHotReload("view", "src/panels/my_panel.cpp");
```

## Limitations

- Only HTML markup and `<style>` blocks inside the watched raw string literal
  are reloaded.
- The extractor locates the template by the member variable's name in the
  source text, so the declaration must keep the form
  `std::string_view view = R"html(...)html";`.
- If the edited template fails to parse, the error is printed to stderr and
  the previous template stays active.
