# Diagnostics

A template or stylesheet can be valid HTML and CSS and still do nothing in
RTXUI: a property the engine does not implement, a typo in a bound name, an
attribute borrowed from another framework. Rather than ignore these silently,
RTXUI reports each one as a diagnostic.

## What Is Reported

| Mistake | Example | Diagnostic |
|---|---|---|
| Unsupported CSS property or value | `font-size: small`, `flex-direction: diagonal` | `unsupported CSS declaration 'font-size: small'` |
| Interpolating a name that is not bound | `{titel}` | `'{titel}' in <App> is not a bound name` |
| An expression inside `{}` | `{count == 1}` | `templates do not evaluate expressions; bind a const method computing the value and use its name` |
| A handler that is not bound | `onclick="Sumbit"`, when clicked | `handler 'Sumbit' is not bound in any enclosing component` |
| Another framework's attribute syntax | `v-if`, `*ngFor`, `className`, `onClick`, `for="x in xs"` | `attribute 'v-if' on <div> in <App> is not RTXUI syntax: use if="{name}" or <if condition="{name}">` |

Each distinct message is reported once per run, even though styles are
re-applied on every frame.

## Where They Go

By default each diagnostic is printed to stderr, prefixed with `rtxui:`.

Set the environment variable `RTXUI_STRICT=1` to abort on the first one
instead. This is what RTXUI's own example test runs with, and it is a good
setting for your tests and CI:

```bash
RTXUI_STRICT=1 ./my_app
```

An application that owns the terminal can route diagnostics through its own
interface, since printing to stderr disturbs a running frame:

```cpp
#include <rtxui/rtxui.hpp>

rtxui::SetDiagnosticHandler([](const rtxui::Diagnostic& d) {
  log_file << d.message << '\n';
});
```

Pass `nullptr` to restore the default.
