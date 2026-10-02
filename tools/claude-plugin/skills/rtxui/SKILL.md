---
name: rtxui
description: Build and check terminal user interfaces with RTXUI, the C++ library whose components are HTML templates and CSS in a string member. Use when code includes <rtxui/rtxui.hpp>, when writing or fixing an RTXUI component, or when asked for a C++ TUI with RTXUI.
---

# RTXUI

RTXUI templates look like HTML and Vue, and its styles like browser CSS, but
it implements its own subset. Write only the constructs below, then check the
result headlessly before saying it works.

## A component

```cpp
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class Counter : public Component<Counter> {
 public:
  int count = 0;
  std::vector<std::string> items = {"a", "b"};

  int Doubled() const { return count * 2; }  // Computed: bind a const method.
  void Increment() { count++; }              // Handler: bind a method.
  void Remove(std::string index) { items.erase(items.begin() + std::stoi(index)); }

  Counter() {
    Bind(count);
    Bind(items);
    Bind(Doubled);
    Bind(Increment);
    Bind(Remove);
  }

  std::string_view view = R"html(
    <div class="card">
      <span>{count} doubled is {Doubled}</span>
      <button onclick="Increment">+1</button>
      <for each="{items}" as="item">
        <div onclick="Remove({$index})">{item}</div>
      </for>
    </div>
    <style>
      .card { border: rounded; padding: 1; }
    </style>
  )html";
};

int main() {
  auto app = Ref<Counter>::New();
  Screen screen(app);
  screen.Loop();
}
```

Link the CMake target `rtxui`, never `rtxui_lib`, and include only
`<rtxui/rtxui.hpp>`.

## Rules

- `{name}` interpolates a bound member or const method. There are no
  expressions: no `{a + b}`, `{x == 1}`, `{ok ? "y" : "n"}`. Compute the value
  in a const method and bind it.
- Every name a template uses must be passed to `Bind(...)`. `Bind` takes a
  member name only; `Bind("Name", lambda)` does not exist.
- Conditions: `if="{flag}"` on any element, or `<if condition="{flag}">` with
  `<elif condition="...">` and `<else>`. True means the value prints as `true`
  or `1`. Never `v-if`, `:if` or `*ngIf`.
- Loops: `<for each="{list}" as="item">` with `{item}` and `{$index}`. Never
  `v-for` or `for="x in xs"`.
- Handlers: `onclick="Method"` or `onclick="Method(arg)"`, one string argument;
  `@click`, `@change` are aliases. No braces, statements or camelCase
  (`onClick`). Use `class`, not `className`.
- Two-way binding: `<input value="{text}"/>`, `<textarea value="{text}"/>`,
  `<checkbox checked="{done}">Label</checkbox>`,
  `<radio name="group" checked="{picked}">Label</radio>`,
  `<select value="{choice}">`. Checkboxes and radios are their own tags, not
  `<input type="...">`.
- Built-in tags need no import. A custom component used in a template must be
  imported in the parent constructor: `Import<Header>();`, then `<Header/>`.
- Define the template as a `view` member or a `std::string_view Setup()`
  method (no `override`), never both.
- Put `<style>` at the top level of the view, next to the root element; a
  nested `<style>` is ignored.
- Use only built-in tags (`div`, `span`, `p`, `h1`-`h6`, `button`, `input`,
  `textarea`, `checkbox`, `radio`, `select`, `ul`/`ol`/`li`, `table`, ...) and
  imported components. There is no `<View>`, `<Text>` or `<Box>`.
- CSS lengths are terminal cells (`width: 20`). `font-size` and `font-family`
  do not exist; use `font-weight`, `font-style`, `text-decoration`, colors and
  `border` (`solid`, `rounded`, `double`, `tall`, ...).
- Only the UI thread may change component state. From a worker thread, take
  `auto post = rtxui::TaskPoster();` on the UI thread and call
  `post([...] { ... })` from the worker.

When unsure whether a property, tag or attribute exists, check the references
rather than guessing: https://arthursonzogni.github.io/RTXUI/llms-full.txt
(the whole manual as Markdown), or `docs/css_reference.md` and
`docs/html_reference.md` in the RTXUI repository.

## Check the result

Never claim an interface works from reading the code. Build it, then run it
headless. `RTXUI_STRICT=1` aborts on anything RTXUI would silently ignore (an
unknown CSS property, an unbound name, another framework's syntax) and prints
why; `RTXUI_HEADLESS=<width>x<height>` prints the screen as text instead of
taking over the terminal:

```bash
RTXUI_STRICT=1 RTXUI_HEADLESS=80x24 ./my_app < /dev/null
```

Drive it by piping terminal input. The screen printed is the one after all of
it is handled:

```bash
printf '\t\t\r' | RTXUI_STRICT=1 RTXUI_HEADLESS=80x24 ./my_app
```

| Input | Bytes |
|---|---|
| Typed text | the text |
| Tab, Shift+Tab, Enter, Backspace | `\t`, `\x1b[Z`, `\r`, `\x7f` |
| Up, Down, Right, Left | `\x1b[A`, `\x1b[B`, `\x1b[C`, `\x1b[D` |
| Click column x, row y (1-based) | `\x1b[<0;x;yM\x1b[<0;x;ym` |

Read the printed screen and compare it with what was asked. In C++ tests,
`rtxui::HeadlessScreen screen(app, 80, 24);` offers `Input(bytes)`,
`Click(x, y)` (0-based), `Resize(w, h)` and `Text()`.
