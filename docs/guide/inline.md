# Inline Mode

By default, `Screen::Loop()` switches the terminal to its alternate screen and
draws over all of it, as a full-screen editor does. In inline mode, the
application is drawn where the cursor is instead, below the command that
started it, as a progress display or a prompt is. The shell's lines stay above
it, and the application is only as tall as its content.

```cpp
#include <rtxui/rtxui.hpp>
#include <string>
#include <string_view>

class Prompt : public rtxui::Component<Prompt> {
 public:
  std::string name;
  std::string_view view = R"html(
    <div>Your name: <input value="{name}" autofocus=""/></div>
  )html";
  Prompt() { Bind(name); }
};

int main() {
  auto app = rtxui::Ref<Prompt>::New();
  rtxui::Screen screen(app);
  screen.SetInline(true);  // Before Loop().
  screen.Loop();
}
```

## Height

An inline application is laid out as a web page is: as tall as its content,
which grows and shrinks as the content does. The terminal's height is the
most it is shown at. Popups that hang below the last row, like an open
`<select>`, count too.

- `height: 100%` on the root or one of its children still makes it fill the
  terminal.
- Content taller than the terminal is cut at its bottom. Give the part that
  can grow `max-height` and `overflow-y: auto` to let it scroll instead.

## Writing above it: `Print()`

`Screen::Print()` writes lines above the application, where they stay as
part of the terminal's history, scrolling away as any output does. The
application is drawn again below them.

```cpp
screen.Print("Downloaded libc6");
```

`Print()` also takes a component. It draws it once, as wide as the terminal
and as tall as it is, with its own styles:

<!-- snippet: fragment -->
```cpp
screen.Print(rtxui::Ref<InstalledLine>::New(package));
```

Call `Print()` from the UI thread; from a worker thread, post it with
`rtxui::PostTask()`. On the alternate screen, which has no history, what
`Print()` writes appears once `Loop()` returns.

## Leaving

When `Loop()` returns, the last frame drawn stays in the terminal, and the
shell goes on below it. Make that frame the outcome: a summary line, a
chosen value. An application that should leave nothing behind draws nothing
before it exits, for instance its content inside an `<if>` turned off.

## Colors

An inline application sits on the terminal's own background, light or dark.
Text no style gives a color to is drawn in the terminal's own text color, and
what no style paints is left to the terminal's background, so plain text reads
on either. For highlights, prefer translucent backgrounds: they blend with
whatever the terminal's background is.

```css
.selected { background-color: rgba(88, 166, 255, 0.3); }
```

## Mouse and cursor

The mouse works as in full screen. Clicks and wheel turns on the shell's
lines, above or below the application, are left to the terminal. With the
mouse captured, the wheel scrolls the application's scroll containers, not
the terminal's history.

A focused `<input>` or `<textarea>` keeps the terminal's cursor on its
caret, where input methods (IME) show the text being composed.

## Resizing

When the terminal is resized, the application is drawn again, whole, at the
new size. Most terminals rewrap their lines when they get narrower: the rows
the application had drawn are rewrapped too, and RTXUI erases what is left of
them on the screen.

Two terminal behaviors are out of reach of any program:

- tmux and alacritty push the rows rewrapping adds into their history.
  Narrowing the terminal can leave pieces of the old frame there, above the
  screen.
- xterm keeps the cells past its new width when it gets narrower, and shows
  them again when it gets wider. Rows written in between can then show pieces
  of an older frame on their right.

## Examples

- [inline_install.cpp](/guide/examples/inline_install) is an installer in the
  style of `apt install`: a live view of the packages being downloaded and
  unpacked, a line printed above it for each one installed, and a summary
  left behind.
- [inline_prompt.cpp](/guide/examples/inline_prompt) is a prompt that stays
  until Escape: a search input with a list to pick from, each pick printed
  above it.
