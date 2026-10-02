# Common Mistakes

RTXUI templates look like HTML and Vue, and its styles look like browser CSS,
so code written from habit — or by a language model — often borrows syntax
RTXUI does not have. Most of these are reported at runtime as
[diagnostics](/guide/diagnostics); this page lists the right form for each.

## Templates

**`{}` holds a bound name, not an expression.** There is no arithmetic,
comparison or ternary in a template. Compute the value in a `const` method and
bind it:

```cpp
// Wrong: <span>{count * 2}</span>   <span>{done ? "yes" : "no"}</span>
class Counter : public rtxui::Component<Counter> {
 public:
  int count = 0;
  int Doubled() const { return count * 2; }

  Counter() {
    Bind(count);
    Bind(Doubled);
  }

  std::string_view view = R"html(<span>{Doubled}</span>)html";
};
```

**Conditions and loops use RTXUI's own tags.**

| Wrong | Right |
|---|---|
| `v-if="shown"`, `*ngIf`, `x-if` | `if="{shown}"` on the element, or `<if condition="{shown}">` with `<elif>` / `<else>` |
| `v-for="item in items"`, `for="item in items"` | `<for each="{items}" as="item">`, with `{$index}` for the index |

A condition is true when the bound value prints as `true` or `1`.

**Handlers name a bound method.** Write `onclick="Name"`, or
`onclick="Name(arg)"` to pass one string argument (`{$index}` works inside the
parentheses). `@click="Name"` is the same. There are no inline statements,
no braces, and no camelCase: `onClick` and `onclick="{() => ...}"` do nothing.

**`class`, not `className`.**

## Binding

**`Bind` takes a member name.** `Bind(count)` binds a variable, `Bind(Doubled)`
a `const` method's result, `Bind(Increment)` a method callable from a
handler. `Bind("Increment", [this] { ... })` does not exist: make the lambda a
method.

**Every name a template uses must be bound**, or `{name}` prints the word
`name`.

## Components

**Define a `view` member or a `Setup()` method, not both**, and write
`Setup()` without `override`: it is found at compile time.

**Built-in tags need no import.** `<div>`, `<button>`, `<input>` and the other
built-ins are always available. Your own components must be imported before a
template can use them: `Import<Header>()` in the constructor.

**Put `<style>` at the top level of the view**, next to the root element. A
`<style>` nested inside an element is ignored.

**There is no `<View>` or `<Text>`.** A tag that is neither a built-in nor an
imported component renders as an unstyled box.

## Styling

**Lengths are terminal cells.** `width: 20` is twenty columns. Fonts do not
exist in a terminal, so `font-size` and `font-family` do nothing; use
`font-weight`, `font-style`, `text-decoration` and colors for emphasis. The
[CSS reference](/css_reference) lists every supported property.

## C++ and Build

**Include only `<rtxui/rtxui.hpp>`.** Headers under `src/` are internal and
not installed.

**Link `rtxui`, not `rtxui_lib`.** `rtxui` keeps the built-in components that
a static link would otherwise drop.

**Only the UI thread touches component state.** From a worker thread, hand the
result back with `rtxui::TaskPoster()` (see the [cookbook](/guide/cookbook)).
