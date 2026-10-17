# Terminal footage for the promo (tools/video/promo/). Short, fast segments of
# example/showcase.cpp, each marked by its caption so promo.html can cut to it:
#
#   python3 tools/video/record.py tools/video/promo_footage.py \
#       /tmp/footage.mp4 tools/video/promo/footage
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
COLS, ROWS = 120, 36
SOURCE = str(ROOT / "example" / "showcase.cpp")
COMMAND = [str(ROOT / "build" / "rtxui_example_showcase")]

ACCENT_OLD = "--accent: rgb(88, 166, 255);"
ACCENT_NEW = "--accent: rgb(255, 123, 172);"
GRID_OLD = "grid-template-columns: repeat(auto-fill, minmax(24, 1fr));"
GRID_NEW = "grid-template-columns: repeat(auto-fill, minmax(40, 1fr));"


def run(t):
    try:
        segments(t)
    finally:
        with open(SOURCE) as f:
            text = f.read()
        text = text.replace(ACCENT_NEW, ACCENT_OLD).replace(GRID_NEW, GRID_OLD)
        with open(SOURCE, "w") as f:
            f.write(text)


def segments(t):
    t.caption("dash")
    t.move(40, 8, 0.5)
    t.wait(0.4)
    t.move(70, 8, 0.4)
    t.wait(0.4)
    t.move(102, 15, 0.4)
    t.wait(0.6)

    t.caption("resize")
    t.hide_pointer()
    t.resize(70, 36, 1.2)
    t.wait(0.5)
    t.resize(120, 36, 1.0)
    t.wait(0.3)

    t.caption("motion")
    t.click(8, 4, 0.4)
    t.wait(0.6)
    t.move(36, 16, 0.4)
    t.wait(0.4)
    t.move(67, 16, 0.4)
    t.wait(0.3)
    t.click(36, 22, 0.4)
    t.wait(1.2)

    t.caption("widgets")
    t.click(8, 5, 0.4)
    t.wait(0.3)
    t.click(62, 6, 0.4)
    t.type(" Lovelace", cps=20)
    t.move(52, 11, 0.3)
    t.drag(63, 11, 0.5)
    t.drag(42, 11, 0.5)
    t.click(45, 17, 0.4)
    t.wait(0.4)

    t.caption("select")
    t.click(8, 6, 0.4)
    t.wait(0.3)
    t.move(24, 8, 0.4)
    t.drag(87, 9, 0.7)
    t.wait(0.6)

    t.caption("reload")
    t.click(8, 3, 0.4)
    t.hide_pointer()
    t.wait(0.6)
    t.edit(SOURCE, ACCENT_OLD, ACCENT_NEW)
    t.wait(1.2)
    t.edit(SOURCE, GRID_OLD, GRID_NEW)
    t.wait(1.4)
    t.caption("end")
    t.wait(0.2)
