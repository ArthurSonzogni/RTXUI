# The launch video: a tour of example/showcase.cpp.
#
#   cmake --build build --target rtxui_example_showcase
#   python3 tools/video/record.py tools/video/showcase_tour.py rtxui.mp4
#
# Coordinates are terminal cells at COLS x ROWS. Hot reload edits the example's
# source while recording and puts it back at the end.
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
COLS, ROWS = 120, 36
SOURCE = str(ROOT / "example" / "showcase.cpp")
COMMAND = [str(ROOT / "build" / "rtxui_example_showcase")]

MUTED = (139, 148, 158)
RED = (248, 81, 73)
GREEN = (63, 185, 80)

ACCENT_OLD = "--accent: rgb(88, 166, 255);"
ACCENT_NEW = "--accent: rgb(255, 123, 172);"
GRID_OLD = "grid-template-columns: repeat(auto-fill, minmax(24, 1fr));"
GRID_NEW = "grid-template-columns: repeat(auto-fill, minmax(40, 1fr));"


def run(t):
    try:
        tour(t)
    finally:
        # Put the source back however the recording ended.
        with open(SOURCE) as f:
            text = f.read()
        text = text.replace(ACCENT_NEW, ACCENT_OLD).replace(GRID_NEW, GRID_OLD)
        with open(SOURCE, "w") as f:
            f.write(text)


def tour(t):
    # 1. A live dashboard.
    t.caption("RTXUI: HTML and CSS for terminal apps, in C++")
    t.wait(2.0)
    t.move(40, 8, 0.8)
    t.wait(0.8)
    t.move(70, 8, 0.6)
    t.wait(0.8)
    t.move(102, 15, 0.6)
    t.wait(0.8)

    # 2. The grid reflows.
    t.caption("CSS grid + @media: the layout reflows with the terminal")
    t.move(110, 33, 0.5)
    t.hide_pointer()
    t.resize(92, 36, 1.0)
    t.wait(1.0)
    t.resize(70, 36, 1.0)
    t.wait(1.2)
    t.resize(120, 36, 1.2)
    t.wait(0.8)

    # 3. Motion.
    t.caption("Transitions and @keyframes, all declared in CSS")
    t.click(8, 4, 0.8)
    t.wait(1.5)
    t.move(36, 16, 0.6)
    t.wait(0.7)
    t.move(67, 16, 0.5)
    t.wait(0.7)
    t.move(36, 16, 0.5)
    t.wait(0.5)
    t.click(36, 22, 0.6)
    t.wait(2.0)

    # 4. Widgets.
    t.caption("Widgets bound to plain C++ members")
    t.click(8, 5, 0.6)
    t.wait(0.8)
    t.click(62, 6, 0.6)
    t.type(" Lovelace", cps=12)
    t.wait(0.5)
    t.move(52, 11, 0.6)
    t.drag(63, 11, 0.8)
    t.wait(0.3)
    t.drag(42, 11, 0.8)
    t.wait(0.4)
    t.click(54, 15, 0.6)
    t.wait(0.4)
    t.click(45, 17, 0.6)
    t.wait(1.0)

    # 5. Text.
    t.caption("Markdown, Unicode and mouse selection")
    t.click(8, 6, 0.6)
    t.wait(1.0)
    t.move(24, 8, 0.6)
    t.drag(87, 9, 1.0)
    t.wait(1.5)

    # 6. Hot reload.
    t.caption("Hot reload: save the file, no recompile")
    t.click(8, 3, 0.6)
    t.move(110, 33, 0.5)
    t.hide_pointer()
    t.wait(0.8)
    t.overlay([
        ("self {", MUTED),
        ("-  " + ACCENT_OLD, RED),
        ("+  " + ACCENT_NEW, GREEN),
    ])
    t.wait(1.0)
    t.edit(SOURCE, ACCENT_OLD, ACCENT_NEW)
    t.wait(2.0)
    t.overlay([
        (".grid {", MUTED),
        ("-  grid-template-columns: ... minmax(24, 1fr));", RED),
        ("+  grid-template-columns: ... minmax(40, 1fr));", GREEN),
    ])
    t.wait(1.0)
    t.edit(SOURCE, GRID_OLD, GRID_NEW)
    t.wait(2.2)
    t.overlay(None)

    # 7. End card.
    t.caption("github.com/ArthurSonzogni/RTXUI")
    t.wait(2.5)
