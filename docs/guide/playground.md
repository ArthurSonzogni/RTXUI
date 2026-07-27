# Playground: A Live HTML/CSS Editor

`example/playground.cpp` splits the terminal in two: a `<textarea>` on the
left holds raw HTML + CSS template text, and the pane on the right renders
that exact text live, using RTXUI to render RTXUI. Every keystroke reparses
the template and repaints the preview — no recompilation, no restart.

```bash
./build/rtxui_example_playground
```

## How It Works

The app is two components:

*   `Playground` — the static shell. It owns a `std::string code` bound to
    the editor's `<textarea>`, and lays out the two panes with flexbox.
*   `LivePreview` — a child component mounted as `<LivePreview id="preview" />`
    inside the right-hand pane. Its initial template is just a sensible
    default; after that, its content is replaced entirely at runtime.

`Playground` overrides `Digest()` to notice when `code` has changed since the
last frame. When it has, it parses the new text with `xml::Parse()` to check
it's valid, then looks up the preview child through the DOM and reloads it:

```cpp
bool Digest() override {
  bool changed = Component<Playground>::Digest();
  if (code == last_code_) {
    return changed;
  }
  last_code_ = code;

  if (!xml::Parse(code)) {
    status = "Parse error: ...";
    return true;  // Keep showing the last valid preview.
  }

  Element* preview_root = Root()->QuerySelector("#preview");
  auto* preview = dynamic_cast<LivePreview*>(
      const_cast<ComponentBase*>(preview_root->component()));
  preview->HotReload(code);
  return true;
}
```

Two mechanisms from elsewhere in RTXUI make this possible:

*   **[`HotReload()`](/guide/hot-reload)** normally re-reads a component's own
    source file when it changes on disk. Called directly with an in-memory
    string, it reparses and re-renders *any* mounted component — which is
    exactly what a live editor needs, minus the file-watching.
*   **[`QuerySelector()`](/guide/cpp/dom)** finds the preview's root element by
    its `id`, and `Element::component()` recovers the owning `ComponentBase*`
    so `Playground` can call `HotReload()` on it directly.

If the typed markup fails to parse, `LivePreview` simply keeps rendering
whatever it last rendered successfully, and the status line beneath the
editor reports the error with a line number — the same recovery behavior
`EnableHotReload()` uses for file-based reloading.

## Limitations

*   `LivePreview` only understands the tags it has explicitly `Import`-ed
    (see the top of `example/playground.cpp` for the full list). A tag it
    hasn't imported is silently ignored by the XML-to-DOM step.
*   The preview is a *document*, not a program: `onclick` handlers and
    C++ member bindings only work for names already bound on `LivePreview`
    itself. Typed markup can restyle and rearrange freely, but it can't
    invent new application logic.
