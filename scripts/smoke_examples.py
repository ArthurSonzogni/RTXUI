#!/usr/bin/env python3
# Copyright 2026 Arthur Sonzogni. All rights reserved.
# Use of this source code is governed by the MIT license that can be found in
# the LICENSE file.
"""Starts every built example, drives input at it, and checks it survives.

The unit tests drive components through Screen and MockTerminalDevice, which
never links the examples or runs main(). Nothing else covered them, so an
example could stop starting -- a crash at construction, a template typo, an
assert on a real terminal size -- and the suite would stay green.

Drawing a frame is only half of it. Every example is a real interface built
from the components an app would use, so replaying keyboard and mouse input at
all of them covers far more event handling than any single test app can. That
matters most under the sanitizer build, where a stale pointer aborts instead of
being read quietly: the one memory bug this engine has had reached hit-testing
only after an event, and only when several arrived together, which is why the
burst below is written to one PTY write rather than paced out.

Each example is an interactive TUI that takes over the terminal, so it needs a
PTY rather than a pipe: on a pipe they see no terminal, and several size
themselves from TIOCGWINSZ. Stdlib only, no pyte -- this checks that the
process survived, not what the frame contained.
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
# After the input burst, keep reading until the example has been silent this
# long. Waiting for quiet rather than for a fixed duration is what makes this
# work under the sanitizer build: the heaviest examples take over ten seconds
# there to work through the burst (a twentieth of a second without it), and
# they redraw the whole time, so silence is a far better signal that they are
# done than any timeout picked in advance.
QUIET_SECONDS = 0.4
# Ceiling on that wait, so an example that genuinely never settles still fails
# rather than stalling the sweep.
INPUT_SECONDS_MAX = 25.0
# How long to wait for an example to exit after Ctrl-C before killing it.
#
# Exceeding this is NOT a failure. Under the sanitizer build the heaviest
# examples take over ten seconds to work through the input burst, against a
# twentieth of a second without it, and the same binaries drain it well inside
# two seconds in a release build -- so a slow exit measures how fast the
# machine is, not whether the example is healthy. Failing on it would make the
# sweep flaky on a loaded CI runner. The kill is only here so a genuinely wedged
# example cannot stall the sweep until ctest's timeout; what this test judges is
# sanitizer reports and abnormal signals.
EXIT_SECONDS = 4.0


def _mouse(button, column, row, pressed=True):
    """One SGR mouse report. Coordinates are 1-based, as the protocol has them."""
    return b"\x1b[<%d;%d;%d%s" % (button, column, row,
                                  b"M" if pressed else b"m")


def _input_burst():
    """Keyboard and mouse aimed at whatever the example happens to put on screen.

    Deliberately one bytestring: Screen::Step drains everything buffered in a
    single read and handles the events back to back, batching away the redraws
    in between. That is the path where a handler that reconciles the tree can
    leave the fragment tree stale for the events behind it, so pacing these out
    would test a strictly easier case.
    """
    parts = [
        b"\t\t",                          # Move focus, twice.
        b"\x1b[Z",                         # And back.
        b"\x1b[A\x1b[B\x1b[C\x1b[D",       # Arrows.
        b"ab",                             # Typing, into whatever has focus.
        b"\x7f",                           # Backspace.
        b" ",                              # Activate.
        b"\r",
    ]
    # Click down a column, hitting whatever controls are stacked there.
    for row in range(2, ROWS, 4):
        parts.append(_mouse(0, 10, row))
        parts.append(_mouse(0, 10, row, pressed=False))
    # A drag, which is the path that holds an element pointer across frames.
    parts.append(_mouse(0, 12, 5))
    for row in range(6, 16, 2):
        parts.append(_mouse(35, 12, row))
    parts.append(_mouse(0, 12, 15, pressed=False))
    # Wheel both ways over a likely scroll container.
    for _ in range(3):
        parts.append(_mouse(65, 20, 10))
    for _ in range(3):
        parts.append(_mouse(64, 20, 10))
    return b"".join(parts)


# Sanitizer reports go to the child's stderr, which is the PTY, so they land in
# the captured output. Naming the diagnostic beats reporting only the signal.
SANITIZER_MARKERS = (b"AddressSanitizer", b"UndefinedBehaviorSanitizer",
                     b"runtime error:", b"LeakSanitizer")


def _sanitizer_report(output):
    for marker in SANITIZER_MARKERS:
        index = output.find(marker)
        if index != -1:
            line = output[index:].split(b"\n", 1)[0]
            return line.decode("utf-8", "replace").strip()
    return None


def _reap(pid, fd):
    """Ctrl-C the example, then make sure it is gone. Returns its wait status."""
    deadline = time.time() + EXIT_SECONDS
    next_interrupt = 0.0
    while time.time() < deadline:
        # Ctrl-C, not Escape: a lone ESC is held pending in case a CSI sequence
        # follows, so it does not quit. Repeated because an example still
        # working through the burst reads it late, or not at all if its input
        # buffer was full when it arrived.
        if time.time() >= next_interrupt:
            try:
                os.write(fd, b"\x03")
            except OSError:
                pass
            next_interrupt = time.time() + 0.5

        try:
            waited, status = os.waitpid(pid, os.WNOHANG)
        except ChildProcessError:
            return None
        if waited:
            return status
        # Keep draining, or the example can block writing into a full PTY and
        # never reach its own exit.
        readable, _, _ = select.select([fd], [], [], 0.05)
        if readable:
            try:
                os.read(fd, 65536)
            except OSError:
                break

    os.kill(pid, signal.SIGKILL)
    try:
        _, status = os.waitpid(pid, 0)
        return status
    except ChildProcessError:
        return None


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

    def drain(seconds):
        collected = b""
        deadline = time.time() + seconds
        while time.time() < deadline:
            readable, _, _ = select.select([fd], [], [], 0.05)
            if not readable:
                continue
            try:
                chunk = os.read(fd, 65536)
            except OSError:  # The child exited and closed the PTY.
                break
            if not chunk:
                break
            collected += chunk
        return collected

    def drain_until_quiet():
        collected = b""
        give_up = time.time() + INPUT_SECONDS_MAX
        quiet_until = time.time() + QUIET_SECONDS
        while time.time() < give_up and time.time() < quiet_until:
            readable, _, _ = select.select([fd], [], [], 0.05)
            if not readable:
                continue
            try:
                chunk = os.read(fd, 65536)
            except OSError:
                break
            if not chunk:
                break
            collected += chunk
            quiet_until = time.time() + QUIET_SECONDS
        return collected

    first_frame = drain(STARTUP_SECONDS)

    # An example that quits on its own input (or on a click) is not a failure,
    # so a write to a closed PTY is not either.
    try:
        os.write(fd, _input_burst())
    except OSError:
        pass
    output = first_frame + drain_until_quiet()

    status = _reap(pid, fd)
    try:
        os.close(fd)
    except OSError:
        pass

    if status is None:
        return None

    report = _sanitizer_report(output)
    if report:
        return report
    if os.WIFSIGNALED(status):
        received = os.WTERMSIG(status)
        if received == signal.SIGKILL:
            # Ours, from _reap. See EXIT_SECONDS.
            return None
        if received != signal.SIGINT:
            return "killed by signal %d (%s)" % (
                received, signal.Signals(received).name)
    if len(first_frame) < MIN_FRAME_BYTES:
        return "drew no frame (%d bytes before exit)" % len(first_frame)
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
        print("\n[ERROR] These examples failed the sweep:", file=sys.stderr)
        for name, reason in failures:
            print("  - %s: %s" % (name, reason), file=sys.stderr)
        return 1
    print("[SUCCESS] Every example starts, draws, and survives input.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
