# Terminal footage for the promo (tools/video/promo/). Short, fast segments of
# example/showcase.cpp, each marked by its caption so promo.html can cut to it:
#
#   python3 tools/video/record.py tools/video/promo_footage.py \
#       /tmp/footage.mp4 tools/video/promo/footage
#
# Coordinates are terminal cells at COLS x ROWS.
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
COLS, ROWS = 120, 36
SOURCE = str(ROOT / "example" / "showcase.cpp")
COMMAND = [str(ROOT / "build" / "rtxui_example_showcase")]

ACCENT_OLD = "--accent: rgb(88, 166, 255);"
ACCENT_NEW = "--accent: rgb(255, 123, 172);"


def run(t):
    try:
        segments(t)
    finally:
        with open(SOURCE) as f:
            text = f.read()
        with open(SOURCE, "w") as f:
            f.write(text.replace(ACCENT_NEW, ACCENT_OLD))


def segments(t):
    t.hide_pointer()
    t.wait(0.3)

    t.caption("palette")
    t.type("\x0b")
    t.wait(0.4)
    t.type("tab", cps=10)
    t.wait(0.3)
    t.key("enter")
    t.wait(0.4)

    t.caption("table")
    t.key("down", 3, gap=0.3)
    t.key("enter")
    t.wait(0.9)
    t.key("esc")
    t.wait(0.4)

    t.caption("resize")
    t.resize(84, 36, 0.9)
    t.wait(0.5)
    t.resize(120, 36, 0.8)
    t.wait(0.3)

    t.caption("hover")
    t.move(10, 8, 0.4)
    t.move(10, 4, 0.5)
    t.click(10, 4, 0.1)
    t.click(28, 7, 0.4)
    t.wait(0.3)
    t.click(57, 7, 0.4)
    t.wait(0.9)

    t.caption("dialog")
    t.click(10, 8, 0.4)
    t.wait(0.2)
    t.click(87, 9, 0.5)
    t.wait(0.6)
    t.hide_pointer()
    t.key("tab")
    t.wait(0.3)
    t.key("enter")
    t.wait(0.8)

    t.caption("reload")
    t.wait(0.4)
    t.edit(SOURCE, ACCENT_OLD, ACCENT_NEW)
    t.wait(1.6)
    t.caption("end")
    t.wait(0.2)
