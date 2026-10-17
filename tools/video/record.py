#!/usr/bin/env python3
# Copyright 2026 Arthur Sonzogni. All rights reserved.
# Use of this source code is governed by the MIT license that can be found in
# the LICENSE file.
"""Record a scripted RTXUI session to MP4.

usage: record.py <scene.py> <out.mp4>

Needs ffmpeg, the JetBrains Mono Nerd Font (FONT_DIR below), Noto Sans CJK,
and `pip install pyte pillow fonttools`.

The scene module defines COLS, ROWS, COMMAND (argv list) and run(t), where `t`
is a Driver. Phase 1 runs the app in a PTY in real time and logs every output
chunk, pointer position and resize with a timestamp. Phase 2 replays the log
into pyte at FPS and renders each frame with PIL, piped into ffmpeg.
"""
import fcntl
import importlib.util
import math
import os
import pty
import select
import struct
import subprocess
import sys
import termios
import threading
import time
import unicodedata

import pyte
from fontTools.ttLib import TTFont
from PIL import Image, ImageDraw, ImageFont

FPS = 30
FONT_SIZE = 20
FONT_DIR = "/usr/local/share/fonts/"
FONTS = {
    (False, False): FONT_DIR + "JetBrainsMonoNerdFontMono-Regular.ttf",
    (True, False): FONT_DIR + "JetBrainsMonoNerdFontMono-Bold.ttf",
    (False, True): FONT_DIR + "JetBrainsMonoNerdFontMono-Italic.ttf",
    (True, True): FONT_DIR + "JetBrainsMonoNerdFontMono-BoldItalic.ttf",
}
FALLBACK = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf"
CJK = "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"
BG_CANVAS = (24, 26, 37)
TERM_BG = (13, 17, 23)
TERM_FG = (230, 237, 243)
CHROME = (33, 38, 46)
ANSI = {
    "black": (0, 0, 0), "red": (205, 49, 49), "green": (13, 188, 121),
    "brown": (229, 229, 16), "blue": (36, 114, 200), "magenta": (188, 63, 188),
    "cyan": (17, 168, 205), "white": (229, 229, 229),
    "brightblack": (102, 102, 102), "brightred": (241, 76, 76),
    "brightgreen": (35, 209, 139), "brightbrown": (245, 245, 67),
    "brightblue": (59, 142, 234), "brightmagenta": (214, 112, 214),
    "brightcyan": (41, 184, 219), "brightwhite": (255, 255, 255),
}


class Driver:
    """What a scene script calls. Every call blocks for its real duration."""

    def __init__(self, fd, log, cols, rows):
        self.fd = fd
        self.log = log
        self.t0 = time.monotonic()
        self.x, self.y = cols / 2, rows / 2
        self.cols, self.rows = cols, rows

    def now(self):
        return time.monotonic() - self.t0

    def _send(self, data):
        os.write(self.fd, data.encode() if isinstance(data, str) else data)

    def _pointer(self, visible=True):
        self.log.append((self.now(), "pointer", (self.x, self.y, visible)))

    def wait(self, seconds):
        end = time.monotonic() + seconds
        while time.monotonic() < end:
            time.sleep(min(0.02, max(0, end - time.monotonic())))

    def type(self, text, cps=14):
        for ch in text:
            self._send(ch)
            self.wait(1 / cps)

    def key(self, name, times=1, gap=0.25):
        seq = {
            "tab": "\t", "shift-tab": "\x1b[Z", "enter": "\r", "space": " ",
            "up": "\x1b[A", "down": "\x1b[B", "right": "\x1b[C",
            "left": "\x1b[D", "backspace": "\x7f", "esc": "\x1b",
        }[name]
        for _ in range(times):
            self._send(seq)
            self.wait(gap)

    def _mouse(self, button, suffix):
        col = int(self.x) + 1
        row = int(self.y) + 1
        self._send(f"\x1b[<{button};{col};{row}{suffix}")

    def move(self, x, y, duration=0.6, button=None):
        """Glide the pointer to cell (x, y) with an ease-in-out curve."""
        x0, y0 = self.x, self.y
        steps = max(1, int(duration * 60))
        last = None
        for i in range(1, steps + 1):
            s = i / steps
            e = 0.5 - 0.5 * math.cos(math.pi * s)
            self.x = x0 + (x - x0) * e
            self.y = y0 + (y - y0) * e
            self._pointer()
            cell = (int(self.x), int(self.y))
            if cell != last:
                self._mouse(35 if button is None else 32 + button, "M")
                last = cell
            self.wait(duration / steps)

    def click(self, x=None, y=None, duration=0.6, button=0):
        if x is not None:
            self.move(x, y, duration)
        self._mouse(button, "M")
        self.log.append((self.now(), "press", None))
        self.wait(0.12)
        self._mouse(button, "m")
        self.wait(0.2)

    def drag(self, x, y, duration=0.8):
        self._mouse(0, "M")
        self.log.append((self.now(), "press", None))
        self.move(x, y, duration, button=0)
        self._mouse(0, "m")
        self.wait(0.2)

    def wheel(self, direction, times=1, gap=0.08):
        for _ in range(times):
            self._mouse(64 if direction == "up" else 65, "M")
            self.wait(gap)

    def hide_pointer(self):
        self._pointer(visible=False)

    def resize(self, cols, rows, duration=1.0):
        """Animate the terminal window to cols x rows, one cell at a time."""
        c0, r0 = self.cols, self.rows
        steps = max(abs(cols - c0), abs(rows - r0), 1)
        for i in range(1, steps + 1):
            c = round(c0 + (cols - c0) * i / steps)
            r = round(r0 + (rows - r0) * i / steps)
            if (c, r) != (self.cols, self.rows):
                self.cols, self.rows = c, r
                self.log.append((self.now(), "resize", (c, r)))
                fcntl.ioctl(self.fd, termios.TIOCSWINSZ,
                            struct.pack("HHHH", r, c, 0, 0))
            self.wait(duration / steps)

    def dump(self, label=""):
        self.log.append((self.now(), "dump", label))

    def overlay(self, lines):
        """Show a code card under the terminal; None hides it."""
        self.log.append((self.now(), "overlay", lines))

    def edit(self, path, old, new):
        """Replace `old` by `new` in `path`, as an editor saving would."""
        with open(path) as f:
            text = f.read()
        assert old in text, old
        with open(path, "w") as f:
            f.write(text.replace(old, new, 1))

    def caption(self, text):
        self.log.append((self.now(), "caption", text))


def record(scene):
    pid, fd = pty.fork()
    if pid == 0:
        os.environ["TERM"] = "xterm-256color"
        os.environ["COLORTERM"] = "truecolor"
        os.execvp(scene.COMMAND[0], scene.COMMAND)
    fcntl.ioctl(fd, termios.TIOCSWINSZ,
                struct.pack("HHHH", scene.ROWS, scene.COLS, 0, 0))
    log = []
    driver = Driver(fd, log, scene.COLS, scene.ROWS)
    done = threading.Event()

    def reader():
        while not done.is_set():
            r, _, _ = select.select([fd], [], [], 0.05)
            if not r:
                continue
            try:
                data = os.read(fd, 65536)
            except OSError:
                return
            if not data:
                return
            log.append((driver.now(), "out", data))
            # Answer the colour queries (OSC 10/11) the way a dark terminal
            # would, so the app does not wait for a reply.
            if b"\x1b]11;?" in data:
                os.write(fd, b"\x1b]11;rgb:0d0d/1111/1717\x1b\\")
            if b"\x1b]10;?" in data:
                os.write(fd, b"\x1b]10;rgb:e6e6/eded/f3f3\x1b\\")

    thread = threading.Thread(target=reader, daemon=True)
    thread.start()
    driver.wait(0.8)  # Let the first frame land before anything happens.
    driver.log.append((driver.now(), "start", None))
    scene.run(driver)
    end = driver.now()
    os.write(fd, b"\x03")
    time.sleep(0.3)
    done.set()
    try:
        os.kill(pid, 9)
    except ProcessLookupError:
        pass
    log.sort(key=lambda e: e[0])
    start = next(t for t, kind, _ in log if kind == "start")
    return log, start, end


def box_arms(ch):
    """{direction: weight} for a box-drawing character, read from its Unicode
    name ("BOX DRAWINGS DOWN SINGLE AND RIGHT DOUBLE"): 1 light, 2 heavy,
    3 double. Returns (arms, dashes, arc)."""
    name = unicodedata.name(ch, "")
    if not name.startswith("BOX DRAWINGS "):
        return None
    name = name[len("BOX DRAWINGS "):]
    dashes = 0
    for word, n in (("DOUBLE DASH", 2), ("TRIPLE DASH", 3),
                    ("QUADRUPLE DASH", 4)):
        if word in name:
            dashes = n
            name = name.replace(word, "")
    arc = "ARC" in name
    name = name.replace("ARC", "")
    weights = {"LIGHT": 1, "SINGLE": 1, "HEAVY": 2, "DOUBLE": 3}
    dirs = {"UP": ["up"], "DOWN": ["down"], "LEFT": ["left"],
            "RIGHT": ["right"], "HORIZONTAL": ["left", "right"],
            "VERTICAL": ["up", "down"]}
    arms = {}
    weight = 1
    for part in name.split(" AND "):
        words = part.split()
        for w in words:
            if w in weights:
                weight = weights[w]
        for w in words:
            for d in dirs.get(w, []):
                arms[d] = weight
    if not arms:
        return None
    return arms, dashes, arc


def draw_special(draw, ch, x, y, w, h, fg):
    """Draws block elements, box drawing and braille edge to edge, which a
    font only does when its line height matches the cell. True if drawn."""
    o = ord(ch)
    if 0x2580 <= o <= 0x259F:
        r = lambda x0, y0, x1, y1: draw.rectangle(
            [x + round(x0 * w), y + round(y0 * h),
             x + round(x1 * w) - 1, y + round(y1 * h) - 1], fill=fg)
        if o == 0x2580:
            r(0, 0, 1, 0.5)
        elif 0x2581 <= o <= 0x2588:
            r(0, 1 - (o - 0x2580) / 8, 1, 1)
        elif 0x2589 <= o <= 0x258F:
            r(0, 0, (0x2590 - o) / 8, 1)
        elif o == 0x2590:
            r(0.5, 0, 1, 1)
        elif o in (0x2591, 0x2592, 0x2593):
            shade = {0x2591: 0.25, 0x2592: 0.5, 0x2593: 0.75}[o]
            draw.rectangle([x, y, x + w - 1, y + h - 1],
                           fill=tuple(int(c * shade) for c in fg))
        elif o == 0x2594:
            r(0, 0, 1, 1 / 8)
        elif o == 0x2595:
            r(7 / 8, 0, 1, 1)
        else:
            quads = {0x2596: "3", 0x2597: "4", 0x2598: "1", 0x2599: "134",
                     0x259A: "14", 0x259B: "123", 0x259C: "124", 0x259D: "2",
                     0x259E: "23", 0x259F: "234"}[o]
            for q in quads:
                qx = 0 if q in "13" else 0.5
                qy = 0 if q in "12" else 0.5
                r(qx, qy, qx + 0.5, qy + 0.5)
        return True
    if 0x2800 <= o <= 0x28FF:
        bits = o - 0x2800
        order = [(0, 0), (0, 1), (0, 2), (1, 0), (1, 1), (1, 2), (0, 3), (1, 3)]
        rad = max(1.5, w / 7)
        for i, (dx, dy) in enumerate(order):
            if bits & (1 << i):
                cx = x + w * (0.3 + 0.4 * dx)
                cy = y + h * (0.14 + 0.24 * dy)
                draw.ellipse([cx - rad, cy - rad, cx + rad, cy + rad], fill=fg)
        return True
    box = box_arms(ch) if 0x2500 <= o <= 0x257F else None
    if box is None:
        return False
    arms, dashes, arc = box
    cx, cy = x + w // 2, y + h // 2
    thin = max(1, round(w / 9))
    thick = thin * 2 + 1
    gap = thin + 2
    ends = {"up": (cx, y), "down": (cx, y + h), "left": (x, cy),
            "right": (x + w, cy)}
    if arc:
        # A rounded corner: a quarter circle joining the two arms.
        (a, _), (b, _) = sorted(arms.items())
        rad = min(w, h) // 2
        horiz = "left" if "left" in arms else "right"
        vert = "up" if "up" in arms else "down"
        sx = -1 if horiz == "left" else 1
        sy = -1 if vert == "up" else 1
        ox, oy = cx + sx * rad, cy + sy * rad
        box_ = [ox - rad, oy - rad, ox + rad, oy + rad]
        start = {(-1, -1): 0, (1, -1): 90, (1, 1): 180, (-1, 1): 270}[(sx, sy)]
        draw.arc(box_, start, start + 90, fill=fg, width=thin)
        draw.line([ox, cy, ends[horiz][0], cy], fill=fg, width=thin)
        draw.line([cx, oy, cx, ends[vert][1]], fill=fg, width=thin)
        return True
    if len(arms) == 2 and set(arms.values()) == {3} and \
            not {"up", "down"} <= arms.keys() and \
            not {"left", "right"} <= arms.keys():
        # A double corner: two nested L shapes.
        sx = -1 if "left" in arms else 1
        sy = -1 if "up" in arms else 1
        ex = ends["left" if sx < 0 else "right"][0]
        ey = ends["up" if sy < 0 else "down"][1]
        for k in (-1, 1):
            jx, jy = cx + k * sx * gap, cy + k * sy * gap
            draw.line([(jx, ey), (jx, jy), (ex, jy)], fill=fg, width=thin)
        return True
    for d, weight in arms.items():
        ex, ey = ends[d]
        offsets = [-gap, gap] if weight == 3 else [0]
        width = thick if weight == 2 else thin
        for off in offsets:
            if d in ("up", "down"):
                p0, p1 = (cx + off, cy), (ex + off, ey)
            else:
                p0, p1 = (cx, cy + off), (ex, ey + off)
            if dashes:
                # A dashed line spans the whole cell, one dash per segment.
                if d in ("left", "up"):
                    continue
                full = [(x, cy), (x + w, cy)] if d == "right" else [(cx, y), (cx, y + h)]
                (ax, ay), (bx, by) = full
                for k in range(dashes):
                    t0, t1 = k / dashes, (k + 0.6) / dashes
                    draw.line([ax + (bx - ax) * t0, ay + (by - ay) * t0,
                               ax + (bx - ax) * t1, ay + (by - ay) * t1],
                              fill=fg, width=width)
                continue
            draw.line([p0, p1], fill=fg, width=width)
        # Fill the joint so arms of different weights meet squarely.
    if len(arms) > 1 and max(arms.values()) != 3:
        half = (thick if 2 in arms.values() else thin) // 2
        draw.rectangle([cx - half, cy - half, cx + half, cy + half], fill=fg)
    return True


class Renderer:
    def __init__(self, max_cols, max_rows):
        self.fonts = {k: ImageFont.truetype(v, FONT_SIZE)
                      for k, v in FONTS.items()}
        self.fallback = ImageFont.truetype(FALLBACK, FONT_SIZE)
        self.cjk = ImageFont.truetype(CJK, FONT_SIZE)
        self.cmap = set(TTFont(FONTS[(False, False)]).getBestCmap())
        regular = self.fonts[(False, False)]
        ascent, descent = regular.getmetrics()
        self.cw = round(regular.getlength("M"))
        self.ch = ascent + descent
        self.cache = {}
        pad = 28
        self.margin = 48
        self.bar = 34
        self.width = self.margin * 2 + pad * 2 + max_cols * self.cw
        self.top = 110  # Room for the caption above the window.
        self.caption_font = ImageFont.truetype(FONTS[(True, False)], 34)
        self.height = (self.margin * 2 + self.top + self.bar + pad +
                       max_rows * self.ch + pad)
        self.width += self.width % 2
        self.height += self.height % 2
        self.pad = pad

    def color(self, name, default):
        if name == "default":
            return default
        if name in ANSI:
            return ANSI[name]
        if len(name) == 6:
            return tuple(int(name[i:i + 2], 16) for i in (0, 2, 4))
        return default

    def glyph(self, ch, fg, bold, italic):
        key = (ch, fg, bold, italic)
        img = self.cache.get(key)
        if img is None:
            font = self.fonts[(bold, italic)]
            wide = 2 if pyte.screens.wcwidth(ch[0]) == 2 else 1
            if ch and ord(ch[0]) not in self.cmap:
                font = self.cjk if wide == 2 else self.fallback
            img = Image.new("RGBA", (self.cw * wide, self.ch), (0, 0, 0, 0))
            ImageDraw.Draw(img).text((0, 0), ch, font=font, fill=fg)
            self.cache[key] = img
        return img

    def terminal(self, screen):
        cols, rows = screen.columns, screen.lines
        img = Image.new("RGB", (cols * self.cw, rows * self.ch), TERM_BG)
        draw = ImageDraw.Draw(img)
        for y in range(rows):
            line = screen.buffer[y]
            for x in range(cols):
                cell = line[x]
                fg = self.color(cell.fg, TERM_FG)
                bg = self.color(cell.bg, TERM_BG)
                if cell.reverse:
                    fg, bg = bg, fg
                px, py = x * self.cw, y * self.ch
                if bg != TERM_BG:
                    draw.rectangle([px, py, px + self.cw - 1, py + self.ch - 1],
                                   fill=bg)
                if cell.data and cell.data != " " and fg != bg:
                    if len(cell.data) == 1 and draw_special(
                            draw, cell.data, px, py, self.cw, self.ch, fg):
                        continue
                    g = self.glyph(cell.data, fg, cell.bold, cell.italics)
                    img.paste(g, (px, py), g)
                if cell.underscore:
                    draw.line([px, py + self.ch - 2, px + self.cw, py + self.ch - 2],
                              fill=fg)
                if cell.strikethrough:
                    draw.line([px, py + self.ch // 2, px + self.cw, py + self.ch // 2],
                              fill=fg)
        return img

    def frame(self, term_img, cols, rows, pointer, caption, overlay=None):
        canvas = Image.new("RGB", (self.width, self.height), BG_CANVAS)
        draw = ImageDraw.Draw(canvas)
        w = cols * self.cw + self.pad * 2
        h = rows * self.ch + self.pad + self.bar + self.pad // 2
        x0 = (self.width - w) // 2
        y0 = self.top + (self.height - self.top - h) // 2
        draw.rounded_rectangle([x0, y0, x0 + w, y0 + h], radius=12, fill=TERM_BG,
                               outline=CHROME, width=2)
        draw.rounded_rectangle([x0, y0, x0 + w, y0 + self.bar], radius=12,
                               fill=CHROME)
        draw.rectangle([x0, y0 + self.bar - 12, x0 + w, y0 + self.bar], fill=CHROME)
        for i, c in enumerate([(255, 95, 86), (255, 189, 46), (39, 201, 63)]):
            cx = x0 + 20 + i * 22
            cy = y0 + self.bar // 2
            draw.ellipse([cx - 7, cy - 7, cx + 7, cy + 7], fill=c)
        if caption:
            font = self.caption_font
            tw = font.getlength(caption)
            draw.text(((self.width - tw) / 2, self.margin + 20), caption,
                      font=font, fill=(230, 237, 243))
        tx = x0 + self.pad
        ty = y0 + self.bar + self.pad // 2
        canvas.paste(term_img, (tx, ty))
        if pointer and pointer[2]:
            px = tx + pointer[0] * self.cw + self.cw / 2
            py = ty + pointer[1] * self.ch + self.ch / 2
            s = 1.3
            arrow = [(0, 0), (0, 17), (4, 13), (7, 20), (10, 19), (7, 12), (12, 12)]
            pts = [(px + ax * s, py + ay * s) for ax, ay in arrow]
            draw.polygon(pts, fill=(255, 255, 255), outline=(0, 0, 0), width=2)
        if overlay:
            self.code_card(draw, overlay)
        return canvas

    def code_card(self, draw, lines):
        """A small editor-like card, bottom right, over the terminal."""
        font = self.fonts[(False, False)]
        lh = self.ch + 2
        w = max(font.getlength(text) for text, _ in lines) + 48
        h = lh * len(lines) + 56
        x0 = self.width - w - self.margin - 24
        y0 = self.height - h - self.margin - 16
        draw.rounded_rectangle([x0 + 6, y0 + 8, x0 + w + 6, y0 + h + 8],
                               radius=12, fill=(5, 7, 10))
        draw.rounded_rectangle([x0, y0, x0 + w, y0 + h], radius=12,
                               fill=(30, 34, 44), outline=(88, 166, 255),
                               width=2)
        draw.text((x0 + 24, y0 + 12), "showcase.cpp", font=font,
                  fill=(139, 148, 158))
        for i, (text, color) in enumerate(lines):
            draw.text((x0 + 24, y0 + 40 + i * lh), text, font=font, fill=color)


def render(log, start, end, out, max_cols, max_rows, cols, rows):
    r = Renderer(max_cols, max_rows)
    screen = pyte.Screen(cols, rows)
    stream = pyte.ByteStream(screen)
    ffmpeg = subprocess.Popen(
        ["ffmpeg", "-loglevel", "error", "-y", "-f", "rawvideo",
         "-pix_fmt", "rgb24", "-s", f"{r.width}x{r.height}", "-r", str(FPS),
         "-i", "-", "-c:v", "libx264", "-pix_fmt", "yuv420p", "-crf", "18",
         "-preset", "slow", "-movflags", "+faststart", out],
        stdin=subprocess.PIPE)
    i = 0
    pointer = None
    caption = None
    overlay = None
    term_img = None
    dirty = True
    frames = int((end - start) * FPS)
    for f in range(frames + 1):
        t = start + f / FPS
        while i < len(log) and log[i][0] <= t:
            _, kind, data = log[i]
            if kind == "out":
                stream.feed(data)
                dirty = True
            elif kind == "resize":
                screen.resize(data[1], data[0])
                dirty = True
            elif kind == "pointer":
                pointer = data
            elif kind == "caption":
                caption = data
            elif kind == "overlay":
                overlay = data
            elif kind == "dump":
                print(f"--- {data} ({screen.columns}x{screen.lines})")
                for row, line in enumerate(screen.display):
                    print(f"{row:2d} {line.rstrip()}")
            i += 1
        if dirty:
            term_img = r.terminal(screen)
            dirty = False
        img = r.frame(term_img, screen.columns, screen.lines, pointer, caption,
                      overlay)
        ffmpeg.stdin.write(img.tobytes())
    ffmpeg.stdin.close()
    ffmpeg.wait()


def main():
    spec = importlib.util.spec_from_file_location("scene", sys.argv[1])
    scene = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(scene)
    log, start, end = record(scene)
    max_cols = max([scene.COLS] + [d[0] for _, k, d in log if k == "resize"])
    max_rows = max([scene.ROWS] + [d[1] for _, k, d in log if k == "resize"])
    # Everything before "start" replays instantly, so the video opens on the
    # first drawn frame.
    render(log, start, end, sys.argv[2], max_cols, max_rows, scene.COLS,
           scene.ROWS)


if __name__ == "__main__":
    main()
