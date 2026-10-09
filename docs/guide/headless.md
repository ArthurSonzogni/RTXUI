# Headless Rendering

An RTXUI application can run without a terminal: at a fixed size, fed input as
bytes, with the screen read back as plain text. Use it to test an interface,
or to check from a script what a change looks like.

## From the Command Line

Any RTXUI program runs headless when the environment variable
`RTXUI_HEADLESS` gives a size. `Screen::Loop()` then feeds whatever is piped
on stdin to the application as input, prints the final screen to stdout and
returns. No code change is needed.

```bash
RTXUI_HEADLESS=80x24 ./my_app < /dev/null    # The first screen.
printf '\t\t\r' | RTXUI_HEADLESS=80x24 ./my_app  # After Tab, Tab, Enter.
```

Input is raw terminal bytes, exactly as a terminal sends them:

| Input | Bytes |
|---|---|
| Typed text | the text itself |
| Tab, Shift+Tab | `\t`, `\x1b[Z` |
| Enter, Backspace | `\r`, `\x7f` |
| Arrows | `\x1b[A` up, `\x1b[B` down, `\x1b[C` right, `\x1b[D` left |
| Left click at column x, row y (1-based) | `\x1b[<0;x;yM\x1b[<0;x;ym` |

The output has one line per row, without colors or styles, with trailing
spaces removed. Combine it with `RTXUI_STRICT=1` (see
[Diagnostics](/guide/diagnostics)) to also fail on any construct RTXUI ignores.

## From C++

`HeadlessScreen` does the same inside a program, which suits unit tests:

```cpp
#include <rtxui/rtxui.hpp>

auto app = rtxui::Ref<Counter>::New();
rtxui::HeadlessScreen screen(app, 40, 10);

screen.Click(5, 3);       // 0-based column and row.
screen.Input("\t\r");     // Tab, then Enter.
screen.Resize(30, 8);

std::string text = screen.Text();
```

Each call returns once the input is handled and any transition it started has
finished, so `Text()` does not depend on timing.

For a single frame, `RenderToString(app, width, height)` returns the text
directly.

## Over Time

`HeadlessScreen` waits for animations to end, so it shows where they land, not
how they get there. `TimelineScreen` runs the application on a clock that
moves only when told to. Animations, transitions, smooth scrolling and delayed
tasks all follow it, so each frame can be checked at an exact time:

```cpp
#include <rtxui/rtxui.hpp>

auto app = rtxui::Ref<Notifier>::New();
rtxui::TimelineScreen screen(app, 40, 10);

screen.Click(5, 3);                        // Starts a 300ms transition.
std::string halfway = screen.Advance(150);  // The frame drawn at 150ms.
screen.Advance(2000);                      // A PostDelayedTask of 2s has run.

// Every distinct frame over the next half second, one per 16ms step:
std::vector<std::string> frames = screen.Record(500);

// Colors, which the text cannot show: a highlight, a scrollbar thumb.
rtxui::Color row = screen.BackgroundAt(0, 2);
```

Nothing waits in real time, so such a test runs instantly and gives the same
result on every machine. A `TimelineScreen` replaces the process-wide clock
while it lives: use one at a time.

## Snapshot Tests

RTXUI checks every example this way: `scripts/snapshot_examples.py` runs each
one headless at 80x24 and compares the output with `example/snapshots/`. A
change that alters what an example shows fails the test with a diff of the
text. When the change is intended, accept it with:

```bash
python3 scripts/snapshot_examples.py build --update
```
