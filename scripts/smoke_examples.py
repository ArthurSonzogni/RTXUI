#!/usr/bin/env python3
# Copyright 2026 Arthur Sonzogni. All rights reserved.
# Use of this source code is governed by the MIT license that can be found in
# the LICENSE file.
"""Starts every built example and checks it draws a frame without dying.

The unit tests drive components through Screen and MockTerminalDevice, which
never links the examples or runs main(). Nothing else covered them, so an
example could stop starting -- a crash at construction, a template typo, an
assert on a real terminal size -- and the suite would stay green.

Each example is an interactive TUI that takes over the terminal, so it needs a
PTY rather than a pipe: on a pipe they see no terminal, and several size
themselves from TIOCGWINSZ. Stdlib only, no pyte -- this checks that a frame
was produced and the process survived, not what the frame contained.
"""
import os
import pty
import select
import signal
import struct
import sys
import termios
import fcntl
import time

# Long enough for a first frame on a loaded machine, short enough that the
# whole sweep stays under a minute.
STARTUP_SECONDS = 1.2
COLUMNS, ROWS = 100, 30
# A first frame is at minimum a few escape sequences plus content; anything
# this small means the app died before drawing.
MIN_FRAME_BYTES = 20


def run_one(program):
    """Returns None if the example started and drew, else a failure reason."""
    pid, fd = pty.fork()
    if pid == 0:
        os.environ["TERM"] = "xterm-256color"
        try:
            os.execv(program, [program])
        finally:
            os._exit(127)

    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", ROWS, COLUMNS, 0, 0))

    output = b""
    deadline = time.time() + STARTUP_SECONDS
    while time.time() < deadline:
        readable, _, _ = select.select([fd], [], [], 0.2)
        if not readable:
            continue
        try:
            chunk = os.read(fd, 65536)
        except OSError:  # The child exited and closed the PTY.
            break
        if not chunk:
            break
        output += chunk

    # Ctrl-C, not Escape: a lone ESC is held pending in case a CSI sequence
    # follows, so it does not quit.
    try:
        os.write(fd, b"\x03")
    except OSError:
        pass
    time.sleep(0.2)
    try:
        os.close(fd)
    except OSError:
        pass

    try:
        _, status = os.waitpid(pid, 0)
    except ChildProcessError:
        return None

    if os.WIFSIGNALED(status):
        received = os.WTERMSIG(status)
        if received != signal.SIGINT:
            return "killed by signal %d (%s)" % (
                received, signal.Signals(received).name)
    if len(output) < MIN_FRAME_BYTES:
        return "drew no frame (%d bytes before exit)" % len(output)
    return None


def expand(arguments):
    """Accepts example binaries, or a build directory to find them in."""
    programs = []
    for argument in arguments:
        if os.path.isdir(argument):
            programs += [
                os.path.join(argument, name)
                for name in os.listdir(argument)
                if name.startswith("rtxui_example_")
                and os.access(os.path.join(argument, name), os.X_OK)
                and os.path.isfile(os.path.join(argument, name))
            ]
        else:
            programs.append(argument)
    return programs


def main(arguments):
    programs = expand(arguments)
    if not programs:
        print("[SKIP] No examples built; nothing to smoke test.")
        return 0

    failures = []
    for program in sorted(programs):
        reason = run_one(program)
        name = os.path.basename(program)
        if reason:
            failures.append((name, reason))
            print("  FAIL %s: %s" % (name, reason))

    print("Checked %d examples, %d failed." % (len(programs), len(failures)))
    if failures:
        print("\n[ERROR] These examples no longer start:", file=sys.stderr)
        for name, reason in failures:
            print("  - %s: %s" % (name, reason), file=sys.stderr)
        return 1
    print("[SUCCESS] Every example starts and draws a frame.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
