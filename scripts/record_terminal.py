#!/usr/bin/env python3
# Copyright 2026 Arthur Sonzogni. All rights reserved.
# Use of this source code is governed by the MIT license that can be found in
# the LICENSE file.
"""Records the bytes a real terminal sends for a fixed set of gestures.

    scripts/record_terminal.py urxvt [kitty ...]

Terminals disagree on how they encode input: urxvt reports a motion with no
button held as a drag of the last button released, and keys like Home or F1
have several encodings in the wild. A unit test written from the xterm spec
can't catch those; bytes recorded from the terminal itself can. Each capture
lands in src/rtxui/terminal/captures/<terminal>-<version>.txt, and
terminal_capture_test.cpp replays every one through TerminalInputParser.

It starts its own X server (Xvfb, or Xephyr in a window when only that is
installed), so the gestures never reach the desktop, opens the terminal in it
running this script with --inside, and sends the gestures with xdotool. The
inside half turns on the modes RTXUI's Screen turns on, and files the bytes it
reads under the name of the gesture the outside half announced last through a
FIFO. Needs xdotool, setxkbmap, libXtst, and Xvfb or Xephyr.
"""
import argparse
import ctypes
import os
import re
import select
import shutil
import signal
import struct
import subprocess
import sys
import tempfile
import termios
import time
import tty
import fcntl

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CAPTURES = os.path.join(ROOT, "src", "rtxui", "terminal", "captures")
TITLE = "rtxui-terminal-capture"

# How to run `command` in each terminal. Wayland-only terminals (foot) can't
# be driven through X.
TERMINALS = {
    "xterm": lambda c: ["xterm", "-e", *c],
    "urxvt": lambda c: ["urxvt", "-e", *c],
    "alacritty": lambda c: ["alacritty", "-e", *c],
    "kitty": lambda c: ["kitty", "-o", "linux_display_server=x11", *c],
    "ghostty": lambda c: ["ghostty", "-e", *c],
    "konsole": lambda c: ["konsole", "--separate", "-e", *c],
    "xfce4-terminal": lambda c: [
        "xfce4-terminal", "--disable-server", "--hide-menubar", "-x", *c],
    "tilix": lambda c: ["tilix", "-e", " ".join(c)],
    # On a bus of its own: else the desktop's gnome-terminal-server opens the
    # window, on the desktop.
    "gnome-terminal": lambda c: ["dbus-run-session", "--", "gnome-terminal",
                                 "--wait", "--", *c],
    # Multiplexers re-encode what the terminal around them sends, in xterm.
    "tmux": lambda c: ["xterm", "-e", "tmux", "-L", "rtxui-capture", "-f",
                       "/dev/null", "new-session", *c],
    # screen's command key moved off Ctrl+A, which it would keep, along with
    # the key after it.
    "screen": lambda c: ["xterm", "-e", "screen", "-U", "-e", "^Gg", "-c",
                         "/dev/null", *c],
}

# How each terminal tells its version, when not --version.
VERSION_FLAGS = {"xterm": "-version", "tmux": "-V", "screen": "-v"}

# Where the pointer goes, as a fraction of the window, away from the edges a
# terminal keeps for padding or scrollbars.
A, B, C, D = (0.3, 0.3), (0.4, 0.35), (0.5, 0.4), (0.6, 0.45)

# The gestures, in order: a name terminal_capture_test.cpp knows, and what to
# do. ("move", point) moves the pointer, ["key", keys] presses keys through
# XTest, ["keydown" | "keyup", key] holds or lets go of one, and anything else
# is xdotool arguments.
GESTURES = [
    ("move", [("move", A), ("move", B)]),
    ("left_press", [["mousedown", "1"]]),
    ("left_drag", [("move", C)]),
    ("left_release", [["mouseup", "1"]]),
    ("move_after_left", [("move", D), ("move", A)]),
    ("right_press", [["mousedown", "3"]]),
    ("right_drag", [("move", B)]),
    ("right_release", [["mouseup", "3"]]),
    ("move_after_right", [("move", C)]),
    ("middle_press", [["mousedown", "2"]]),
    ("middle_release", [["mouseup", "2"]]),
    ("wheel_up", [["click", "4"]]),
    ("wheel_down", [["click", "5"]]),
    # A motion with a modifier held and no button: urxvt sends it as code
    # 32 - 1 + the modifier, which reads like a button.
    ("shift_move", [["keydown", "shift"], ("move", B), ("move", C),
                    ["keyup", "shift"]]),
    ("ctrl_move", [["keydown", "ctrl"], ("move", B), ("move", C),
                   ["keyup", "ctrl"]]),
    ("alt_move", [["keydown", "alt"], ("move", B), ("move", C),
                  ["keyup", "alt"]]),
    ("a", [["key", "a"]]),
    ("shift_a", [["key", "shift+a"]]),
    ("ctrl_a", [["key", "ctrl+a"]]),
    ("alt_a", [["key", "alt+a"]]),
    # No key types it on a US layout; xdotool maps one for the time of it.
    ("e_acute", [["key", "--clearmodifiers", "eacute"]]),
    ("up", [["key", "Up"]]),
    ("down", [["key", "Down"]]),
    ("right", [["key", "Right"]]),
    ("left", [["key", "Left"]]),
    ("ctrl_left", [["key", "ctrl+Left"]]),
    ("ctrl_right", [["key", "ctrl+Right"]]),
    ("alt_up", [["key", "alt+Up"]]),
    ("home", [["key", "Home"]]),
    ("end", [["key", "End"]]),
    ("page_up", [["key", "Prior"]]),
    ("page_down", [["key", "Next"]]),
    ("insert", [["key", "Insert"]]),
    ("delete", [["key", "Delete"]]),
    ("backspace", [["key", "BackSpace"]]),
    ("return", [["key", "Return"]]),
    ("tab", [["key", "Tab"]]),
    ("shift_tab", [["key", "shift+Tab"]]),
    ("escape", [["key", "Escape"]]),
    ("f2", [["key", "F2"]]),
    ("f3", [["key", "F3"]]),
    ("f4", [["key", "F4"]]),
    ("f5", [["key", "F5"]]),
    ("f12", [["key", "F12"]]),
    # Last: xfce4-terminal opens its help on F1, which takes the focus.
    ("f1", [["key", "F1"]]),
]


def escape(data):
    """Bytes as one line of text: printable ASCII as is, the rest as \\xHH."""
    return "".join(
        chr(b) if 0x20 <= b < 0x7F and b != ord("\\") else f"\\x{b:02x}"
        for b in data)


def inside(fifo, out):
    """Runs in the terminal: records what it reads under each gesture name."""
    fd = sys.stdin.fileno()
    old = termios.tcgetattr(fd)
    tty.setraw(fd)
    # The modes Screen turns on (terminal_device.hpp), pixel reporting only
    # when the terminal says how big a cell is.
    rows, cols, xpixel, ypixel = struct.unpack(
        "HHHH", fcntl.ioctl(fd, termios.TIOCGWINSZ, b"\0" * 8))
    pixels = xpixel > 0 and ypixel > 0
    modes = "\x1b[?1003h\x1b[?1006h" + ("\x1b[?1016h" if pixels else "")
    os.write(1, f"\x1b]2;{TITLE}\x07\x1b[?2004h{modes}".encode())

    sections = []
    name, data = None, b""

    def read_for(seconds):
        nonlocal data
        end = time.time() + seconds
        while time.time() < end:
            if select.select([fd], [], [], 0.05)[0]:
                data += os.read(fd, 4096)

    # The replies to the queries Screen makes, before any gesture.
    for query_name, query in [
        ("reply_colors", "\x1b]11;?\x1b\\\x1b]10;?\x1b\\"),
        ("reply_cursor_position", "\x1b[6n"),
        ("reply_pixel_mode", "\x1b[?1016$p"),
    ]:
        read_for(0.2)  # Drops what arrived before.
        data = b""
        os.write(1, query.encode())
        read_for(1.0)
        sections.append((query_name, data))
        data = b""
    open(fifo + ".ready", "w").close()

    control = os.open(fifo, os.O_RDONLY | os.O_NONBLOCK)
    pending = b""
    done = False
    while not done:
        ready = select.select([fd, control], [], [], 1)[0]
        if fd in ready:
            data += os.read(fd, 4096)
        if control in ready:
            pending += os.read(control, 4096)
            while b"\n" in pending:
                line, pending = pending.split(b"\n", 1)
                if name is not None:
                    sections.append((name, data))
                name, data = line.decode(), b""
                done = name == "--done--"

    os.write(1, ("\x1b[?1003l\x1b[?1006l\x1b[?1016l\x1b[?2004l").encode())
    termios.tcsetattr(fd, termios.TCSADRAIN, old)
    with open(out, "w") as f:
        f.write(f"# pixels: {'yes' if pixels else 'no'}\n")
        for section, data in sections:
            f.write(f"{section}\t{escape(data)}\n")


class Keyboard:
    """Presses keys through XTest.

    xdotool 3.20160805 holds Alt while it presses F1..F12, because XKB lists
    F1 on levels that need it, so it can't be trusted with keys.
    """

    MODIFIERS = {"shift": "Shift_L", "ctrl": "Control_L", "alt": "Alt_L"}

    def __init__(self, display):
        self.x11 = ctypes.CDLL("libX11.so.6")
        self.xtst = ctypes.CDLL("libXtst.so.6")
        self.x11.XOpenDisplay.restype = ctypes.c_void_p
        self.x11.XOpenDisplay.argtypes = [ctypes.c_char_p]
        self.x11.XStringToKeysym.restype = ctypes.c_ulong
        self.x11.XStringToKeysym.argtypes = [ctypes.c_char_p]
        self.x11.XKeysymToKeycode.argtypes = [ctypes.c_void_p, ctypes.c_ulong]
        self.x11.XCloseDisplay.argtypes = [ctypes.c_void_p]
        self.xtst.XTestFakeKeyEvent.argtypes = [
            ctypes.c_void_p, ctypes.c_uint, ctypes.c_int, ctypes.c_ulong]
        self.display_name = display.encode()

    def press(self, keys):
        """Presses `keys`, like "ctrl+Left": the modifiers, then the key."""
        self.send([(name, True) for name in keys.split("+")] +
                  [(name, False) for name in reversed(keys.split("+"))])

    def send(self, events):
        """Sends (key, pressed) events, in order."""
        # A connection reads the keymap once. Xephyr can take the desktop's
        # layout over when its window gets the focus, so read it every time.
        display = self.x11.XOpenDisplay(self.display_name)
        for name, pressed in events:
            keysym = self.x11.XStringToKeysym(
                self.MODIFIERS.get(name, name).encode())
            code = self.x11.XKeysymToKeycode(display, keysym)
            self.xtst.XTestFakeKeyEvent(display, code, pressed, 0)
        self.x11.XCloseDisplay(display)


def start_x_server():
    """Starts a private X server; returns it and its DISPLAY."""
    number = 90
    while os.path.exists(f"/tmp/.X11-unix/X{number}"):
        number += 1
    display = f":{number}"
    if shutil.which("Xvfb"):
        command = ["Xvfb", display, "-screen", "0", "1280x1024x24"]
    elif shutil.which("Xephyr") and os.environ.get("DISPLAY"):
        command = ["Xephyr", display, "-screen", "1280x1024", "-ac", "-keybd",
                   "ephyr,,,xkbmodel=pc105,xkblayout=us"]
    else:
        sys.exit("record_terminal.py: needs Xvfb, or Xephyr and a display")
    server = subprocess.Popen(command, stdout=subprocess.DEVNULL,
                              stderr=subprocess.DEVNULL)
    for _ in range(100):
        if os.path.exists(f"/tmp/.X11-unix/X{number}"):
            time.sleep(1)  # Xephyr's window settles, and its layout with it.
            return server, display
        time.sleep(0.05)
    server.kill()
    sys.exit("record_terminal.py: the X server did not start")


def version_of(terminal):
    flag = VERSION_FLAGS.get(terminal, "--version")
    result = subprocess.run([terminal, flag], capture_output=True, text=True)
    # urxvt has no --version; its usage line carries it.
    if not re.search(r"\d", result.stdout + result.stderr):
        result = subprocess.run([terminal, "-help"], capture_output=True,
                                text=True)
    output = result.stdout + result.stderr
    # A dotted version first: "xfce4-terminal 1.1.5 (Xfce 4.20)".
    match = (re.search(r"\d+(\.\d+)+", output) or
             re.search(r"\d+", output))
    return match.group(0) if match else "unknown"


def record(terminal, env, keyboard):
    """Returns what `terminal` sent for each gesture, as capture lines."""
    work = tempfile.mkdtemp(prefix="rtxui-capture-")
    fifo = os.path.join(work, "control")
    out = os.path.join(work, "capture.txt")
    os.mkfifo(fifo)
    command = [sys.executable, "-I", os.path.abspath(__file__), "--inside",
               fifo, out]
    def xdotool(*args):
        return subprocess.run(["xdotool", *args], env=env, capture_output=True,
                              text=True).stdout

    def windows():
        return set(xdotool("search", "--onlyvisible", "--name", ".").split())

    # The gestures assume a US layout.
    subprocess.run(["setxkbmap", "us"], env=env)
    # Out of where the window opens, so that it sees no motion until asked.
    xdotool("mousemove", "10000", "10000")
    before = windows()
    process = subprocess.Popen(TERMINALS[terminal](command), env=env,
                               stdout=subprocess.DEVNULL,
                               stderr=subprocess.DEVNULL,
                               start_new_session=True)
    # Held open so the inside half never reads an end of file.
    control = os.open(fifo, os.O_RDWR)

    # The inside half sends the queries first.
    for _ in range(200):
        if os.path.exists(fifo + ".ready"):
            break
        time.sleep(0.1)
    def size(window):
        geometry = dict(
            line.split("=") for line in xdotool(
                "getwindowgeometry", "--shell", window).split())
        return int(geometry["WIDTH"]), int(geometry["HEIGHT"])

    # The window the inside half named, or else the largest that appeared:
    # konsole keeps its own title, and opens small helper windows too.
    window = xdotool("search", "--name", TITLE).split("\n")[0]
    if not window and windows() - before:
        window = max(windows() - before,
                     key=lambda w: size(w)[0] * size(w)[1])
    if not window:
        os.killpg(process.pid, signal.SIGKILL)
        shutil.rmtree(work)
        print(f"{terminal}: its window was not found")
        return None
    # With no window manager, nothing else gives it the keyboard, nor a size:
    # konsole opens at 249x100, mostly toolbar.
    xdotool("windowfocus", window)
    xdotool("windowsize", "--sync", window, "800", "500")
    time.sleep(0.5)
    width, height = size(window)

    def point(p):
        return [str(int(p[0] * width)), str(int(p[1] * height))]

    # The pointer in the window gives it the focus, with no window manager.
    xdotool("mousemove", "--window", window, *point(A))
    time.sleep(0.3)
    for name, actions in GESTURES:
        os.write(control, (name + "\n").encode())
        time.sleep(0.15)
        for action in actions:
            if action[0] == "move":
                xdotool("mousemove", "--window", window, *point(action[1]))
            elif action[0] == "key" and len(action) == 2:
                keyboard.press(action[1])
            elif action[0] in ("keydown", "keyup"):
                keyboard.send([(action[1], action[0] == "keydown")])
            else:
                xdotool(*action)
            time.sleep(0.15)
        time.sleep(0.3)
    os.write(control, b"--done--\n")

    for _ in range(50):
        if os.path.exists(out) and process.poll() is not None:
            break
        time.sleep(0.1)
    if process.poll() is None:
        os.killpg(process.pid, signal.SIGTERM)
    lines = None
    if os.path.exists(out):
        with open(out) as f:
            lines = f.read()
    else:
        print(f"{terminal}: the capture was not written")
    shutil.rmtree(work)
    return lines


def record_checked(terminal, env, keyboard):
    """Records `terminal` until its capture can be trusted, and saves it."""
    # The plain keys check the recording itself: anything else means the
    # layout changed under it, the keys went to another window, or the
    # desktop's pointer crossed Xephyr's window.
    expected = ["a\ta", "shift_a\tA", "ctrl_a\t\\x01"]
    for _ in range(3):
        lines = record(terminal, env, keyboard)
        if lines is None:
            return
        found = lines.split("\n")
        if all(line in found for line in expected):
            break
        print(f"{terminal}: the plain keys were not typed, recording again")
    else:
        print(f"{terminal}: gave up")
        return
    version = version_of(terminal)
    os.makedirs(CAPTURES, exist_ok=True)
    path = os.path.join(CAPTURES, f"{terminal}-{version}.txt")
    with open(path, "w") as capture:
        capture.write(f"# {terminal} {version}, recorded by "
                      "scripts/record_terminal.py\n")
        capture.write(lines)
    print(f"{terminal}: {os.path.relpath(path, ROOT)}")


def main():
    if len(sys.argv) == 4 and sys.argv[1] == "--inside":
        inside(sys.argv[2], sys.argv[3])
        return
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("terminals", nargs="+", choices=sorted(TERMINALS))
    args = parser.parse_args()
    if not shutil.which("xdotool"):
        sys.exit("record_terminal.py: needs xdotool")
    server, display = start_x_server()
    env = dict(os.environ, DISPLAY=display)
    env.pop("WAYLAND_DISPLAY", None)
    keyboard = Keyboard(display)
    try:
        for terminal in args.terminals:
            record_checked(terminal, env, keyboard)
    finally:
        server.terminate()


if __name__ == "__main__":
    main()
