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
| A tag that is not a built-in or an imported component | `<View>`, `<Card/>` without `Import<Card>()` | `unknown tag <Card> in <App>: if it is your component, call Import<Card>() in <App>'s constructor` |
| A `<style>` nested inside an element | `<div><style>...</style></div>` | `<style> inside <div> in <App> is ignored: put <style> at the top level of the view` |
| Another framework's attribute syntax | `v-if`, `*ngFor`, `className`, `onClick`, `for="x in xs"` | `attribute 'v-if' on <div> in <App> is not RTXUI syntax: use if="{name}" or <if condition="{name}">` |

Each distinct message is reported once per run, even though styles are
re-applied on every frame.

Markup or CSS that does not parse is reported too, with where the error is:

| Mistake | `kind` | `line`, `column` |
|---|---|---|
| A `<style>` block that does not parse, such as one built from bound state | `CssSyntax` | In that `<style>` block, 0-based |
| A template that does not parse, on hot reload, or markup built at run time such as a `<markdown>` component's | `XmlSyntax` | In that markup, 0-based |

A syntax error is reported each time the text fails to parse, not once, so
that an app showing live-edited markup hears about every attempt. Without a
handler it is printed with the lines around it, and a component's own
template that does not parse ends the process: it is usually a literal in
the source, and the message is then the last thing on the terminal.

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
  if (d.kind == rtxui::Diagnostic::Kind::CssSyntax) {
    log_file << "line " << d.line + 1 << ": ";
  }
  log_file << d.message << '\n';
});
```

Pass `nullptr` to restore the default.
