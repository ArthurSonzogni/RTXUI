#!/usr/bin/env python3
# Copyright 2026 Arthur Sonzogni. All rights reserved.
# Use of this source code is governed by the MIT license that can be found in
# the LICENSE file.
"""Render a captured RTXUI frame to SVG, for embedding in the README/docs.

RTXUI paints whole frames, so a capture is a sequence of complete screens. This
takes the last one, replays its SGR colour state onto a character grid, and
emits an SVG of background rectangles plus text runs. SVG keeps the true-colour
output crisp at any zoom and stays diff-friendly in git.

Usage:
  script -qec "stty rows 24 cols 100; timeout 2 ./build/rtxui_example_foo" /dev/null \\
    | python3 tools/ansi_to_svg.py --out docs/public/foo.svg
"""
import argparse
import html
import re
import sys

CELL_W = 8.4
CELL_H = 17.0
FONT = ("ui-monospace, SFMono-Regular, 'SF Mono', Menlo, Consolas, "
        "'DejaVu Sans Mono', monospace")

SGR_RE = re.compile(r"\x1b\[([0-9;]*)m")
ANSI_OTHER_RE = re.compile(r"\x1b\[[0-9;?]*[A-Za-z]|\x1b\][^\x07]*\x07|\x1b[()][B0]")

DEFAULT_FG = (230, 237, 243)
DEFAULT_BG = (13, 17, 23)

# Block elements (U+2580..U+2595) tile edge to edge in a terminal, but as text
# glyphs in an SVG they leave hairline gaps and overshoot the cell -- which
# wrecks exactly the `tall` border style the examples prefer. Draw them as
# rectangles instead, as fractions of the cell: (x, y, width, height).
BLOCKS = {
    "▀": (0, 0, 1, 1 / 2),      # upper half
    "▁": (0, 7 / 8, 1, 1 / 8),  # lower one eighth
    "▂": (0, 3 / 4, 1, 1 / 4),
    "▃": (0, 5 / 8, 1, 3 / 8),
    "▄": (0, 1 / 2, 1, 1 / 2),  # lower half
    "▅": (0, 3 / 8, 1, 5 / 8),
    "▆": (0, 1 / 4, 1, 3 / 4),
    "▇": (0, 1 / 8, 1, 7 / 8),
    "█": (0, 0, 1, 1),          # full block
    "▉": (0, 0, 7 / 8, 1),
    "▊": (0, 0, 3 / 4, 1),      # left three quarters
    "▋": (0, 0, 5 / 8, 1),
    "▌": (0, 0, 1 / 2, 1),      # left half
    "▍": (0, 0, 3 / 8, 1),
    "▎": (0, 0, 1 / 4, 1),      # left one quarter
    "▏": (0, 0, 1 / 8, 1),
    "▐": (1 / 2, 0, 1 / 2, 1),  # right half
    "▔": (0, 0, 1, 1 / 8),      # upper one eighth
    "▕": (7 / 8, 0, 1 / 8, 1),  # right one eighth
}


class Cell:
    __slots__ = ("ch", "fg", "bg", "bold")

    def __init__(self):
        self.ch = " "
        self.fg = DEFAULT_FG
        self.bg = DEFAULT_BG
        self.bold = False


def last_frame(text):
    """Split on the clear-screen/home sequence and keep the fullest screen.

    Limitation: RTXUI diffs textures and emits only the cells that changed, so
    everything after the first full paint is a partial update addressed with
    cursor-positioning sequences that this script discards. That makes it a
    screenshot tool for an application's initial frame, not a way to capture
    state after interaction -- drive the component directly if you need that.
    """
    parts = re.split(r"\x1b\[2J(?:\x1b\[H)?|\x1b\[H", text)
    parts = [p for p in parts if p.strip()]
    if not parts:
        return text
    # The richest trailing chunk is the settled frame.
    return max(parts[-3:], key=len)


def parse(text, cols, rows):
    grid = [[Cell() for _ in range(cols)] for _ in range(rows)]
    fg, bg, bold = DEFAULT_FG, DEFAULT_BG, False
    row = col = 0
    i = 0
    while i < len(text):
        match = SGR_RE.match(text, i)
        if match:
            codes = [int(c) for c in match.group(1).split(";") if c != ""] or [0]
            j = 0
            while j < len(codes):
                code = codes[j]
                if code == 0:
                    fg, bg, bold = DEFAULT_FG, DEFAULT_BG, False
                elif code == 1:
                    bold = True
                elif code == 22:
                    bold = False
                elif code == 39:
                    fg = DEFAULT_FG
                elif code == 49:
                    bg = DEFAULT_BG
                elif code == 38 and j + 4 < len(codes) and codes[j + 1] == 2:
                    fg = (codes[j + 2], codes[j + 3], codes[j + 4])
                    j += 4
                elif code == 48 and j + 4 < len(codes) and codes[j + 1] == 2:
                    bg = (codes[j + 2], codes[j + 3], codes[j + 4])
                    j += 4
                j += 1
            i = match.end()
            continue

        other = ANSI_OTHER_RE.match(text, i)
        if other:
            i = other.end()
            continue

        ch = text[i]
        i += 1
        if ch == "\n":
            row += 1
            col = 0
            continue
        if ch == "\r":
            col = 0
            continue
        if ch in "\x00\x07\x0f\x0e":
            continue
        if 0 <= row < rows and 0 <= col < cols:
            cell = grid[row][col]
            cell.ch, cell.fg, cell.bg, cell.bold = ch, fg, bg, bold
        col += 1
    return grid


def to_svg(grid, cols, rows):
    width = cols * CELL_W
    height = rows * CELL_H
    out = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width:.0f}" '
        f'height="{height:.0f}" viewBox="0 0 {width:.2f} {height:.2f}" '
        f'font-family="{FONT}" font-size="13.5">',
        f'<rect width="100%" height="100%" fill="rgb{DEFAULT_BG}" rx="6"/>',
    ]

    # Background runs.
    for r, line in enumerate(grid):
        c = 0
        while c < cols:
            bg = line[c].bg
            if bg == DEFAULT_BG:
                c += 1
                continue
            start = c
            while c < cols and line[c].bg == bg:
                c += 1
            out.append(
                f'<rect x="{start * CELL_W:.2f}" y="{r * CELL_H:.2f}" '
                f'width="{(c - start) * CELL_W:.2f}" height="{CELL_H:.2f}" '
                f'fill="rgb{bg}"/>'
            )

    # Block elements, drawn as geometry so they tile seamlessly. Runs of the
    # same glyph and colour merge into one rectangle: adjacent rectangles that
    # merely share an edge still show an antialiasing hairline when the SVG is
    # rasterised, which would streak every full-width bar and border.
    for r, line in enumerate(grid):
        c = 0
        while c < cols:
            block = BLOCKS.get(line[c].ch)
            if not block:
                c += 1
                continue
            ch, fg, start = line[c].ch, line[c].fg, c
            while c < cols and line[c].ch == ch and line[c].fg == fg:
                c += 1
            bx, by, bw, bh = block
            span = c - start
            # Only full-width blocks may merge horizontally; a partial-width
            # block sits at its own offset inside each cell.
            if bx == 0 and bw == 1:
                x, width = start * CELL_W, span * CELL_W
            else:
                x, width = (start + bx) * CELL_W, bw * CELL_W
                c = start + 1  # re-walk the rest of the run individually
                span = 1
            out.append(
                f'<rect x="{x:.2f}" y="{(r + by) * CELL_H:.2f}" '
                f'width="{width:.2f}" height="{bh * CELL_H:.2f}" '
                f'fill="rgb{fg}"/>'
            )

    # Text runs sharing colour and weight.
    for r, line in enumerate(grid):
        c = 0
        y = r * CELL_H + CELL_H * 0.75
        while c < cols:
            if line[c].ch == " " or line[c].ch in BLOCKS:
                c += 1
                continue
            fg, bold, start = line[c].fg, line[c].bold, c
            run = []
            while c < cols and line[c].fg == fg and line[c].bold == bold \
                    and line[c].ch != " " and line[c].ch not in BLOCKS:
                run.append(line[c].ch)
                c += 1
            weight = ' font-weight="bold"' if bold else ""
            text = html.escape("".join(run))
            out.append(
                f'<text x="{start * CELL_W:.2f}" y="{y:.2f}" '
                f'fill="rgb{fg}"{weight} xml:space="preserve">{text}</text>'
            )

    out.append("</svg>")
    return "\n".join(out)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--cols", type=int, default=100)
    parser.add_argument("--rows", type=int, default=24)
    parser.add_argument("--out", required=True)
    args = parser.parse_args()

    raw = sys.stdin.buffer.read().decode("utf-8", errors="replace")
    grid = parse(last_frame(raw), args.cols, args.rows)
    with open(args.out, "w", encoding="utf-8") as handle:
        handle.write(to_svg(grid, args.cols, args.rows))
    print(f"wrote {args.out} ({args.cols}x{args.rows})")


if __name__ == "__main__":
    main()
