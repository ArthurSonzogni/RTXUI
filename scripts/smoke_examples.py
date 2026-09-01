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
import random
import select
import signal
import struct
import sys
import termios
import tempfile
import fcntl
import time
import zlib

# Long enough for a first frame on a loaded machine, short enough that the
# whole sweep stays under a minute.
STARTUP_SECONDS = 1.2
COLUMNS, ROWS = 100, 30
# Sizes to resize to part way through, in order, ending back at the size the
# example started with. A terminal resize is the one input that invalidates the
# whole layout rather than a corner of it, and it arrives asynchronously as
# SIGWINCH, so it is worth doing while the example is mid-burst rather than
# while it is idle. Degenerate first: the arithmetic that clamps and divides
# for scrollbars, flex and grid tracks is where a viewport far too small to
# hold the interface shows up. Kept to two steps because each one costs the
# whole sweep a second per example.
RESIZE_STEPS = ((1, 1), (100, 30))
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
# How long to read between resize steps. Short on purpose -- the next step
# should land while the example is still busy -- and kept to a minimum because
# it is paid 53 times over, twice per resize step.
STEP_SECONDS = 0.2
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


def _set_size(fd, columns, rows):
    """Resizes the PTY, which sends SIGWINCH to the example."""
    fcntl.ioctl(fd, termios.TIOCSWINSZ,
                struct.pack("HHHH", rows, columns, 0, 0))


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


def _spawn(program, error_path):
    """Starts `program` on a PTY, with its stderr going to `error_path`.

    pty.fork() would put stderr on the PTY too, which loses the one thing worth
    reading: an assertion or sanitizer report written after this side has
    stopped draining is simply gone, and what arrives instead is a bare signal
    number. A file always has it.
    """
    master, slave = pty.openpty()
    pid = os.fork()
    if pid == 0:
        os.setsid()
        fcntl.ioctl(slave, termios.TIOCSCTTY, 0)
        os.dup2(slave, 0)
        os.dup2(slave, 1)
        handle = os.open(error_path, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o644)
        os.dup2(handle, 2)
        os.close(master)
        os.close(slave)
        os.environ["TERM"] = "xterm-256color"
        try:
            os.execv(program, [program])
        finally:
            os._exit(127)
    os.close(slave)
    return pid, master


def _read_errors(error_path):
    try:
        with open(error_path, "rb") as handle:
            return handle.read()
    except OSError:
        return b""


def run_one(program, error_path):
    """Returns None if the example started and drew, else a failure reason."""
    pid, fd = _spawn(program, error_path)

    _set_size(fd, COLUMNS, ROWS)

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
    output = first_frame
    try:
        os.write(fd, _input_burst())
        # Resize and re-drive without letting the example settle in between.
        # Landing a resize on an interface that is still working through a
        # burst is the whole point: it throws the layout away mid-interaction,
        # with a drag in flight, which is what holds an element across frames.
        # Waiting for quiet between steps hides exactly that -- it was tried,
        # and the sweep stopped reproducing a DOM assertion that this ordering
        # trips reliably.
        for columns, rows in RESIZE_STEPS:
            _set_size(fd, columns, rows)
            output += drain(STEP_SECONDS)
            os.write(fd, _input_burst())
            output += drain(STEP_SECONDS)
    except OSError:
        pass
    # Settle only at the end, so a crash the sequence provoked has happened
    # before the example is asked to quit.
    output += drain_until_quiet()

    status = _reap(pid, fd)
    try:
        os.close(fd)
    except OSError:
        pass

    if status is None:
        return None

    errors = _read_errors(error_path)
    report = _sanitizer_report(output + errors)
    if report:
        return report
    if os.WIFSIGNALED(status):
        received = os.WTERMSIG(status)
        if received == signal.SIGKILL:
            # Ours, from _reap. See EXIT_SECONDS.
            return None
        if received != signal.SIGINT:
            detail = errors.decode("utf-8", "replace").strip().splitlines()
            return "killed by signal %d (%s)%s" % (
                received, signal.Signals(received).name,
                ": " + detail[-1] if detail else "")
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


def _chaos_size(rng):
    """A terminal size, weighted hard toward the degenerate end.

    That bias is the whole point. A roomy terminal mostly re-walks the path the
    fixed sweep already covers; it is the sizes too small to hold the interface
    that reach the clamping and division in scrollbars, flex and grid tracks.
    A 1x1 resize landing mid-interaction is what turned up the last DOM bug.
    """
    if rng.random() < 0.6:
        return rng.randint(1, 6), rng.randint(1, 5)
    return rng.randint(1, 200), rng.randint(1, 80)


def _chaos_chunk(rng):
    """A slice of input: the fixed burst, typing, mouse, paste or navigation."""
    pick = rng.random()
    if pick < 0.35:
        return _input_burst()
    if pick < 0.5:
        return bytes(rng.choice(b"\t\r ab\x7f") for _ in range(rng.randint(1, 8)))
    if pick < 0.65:
        return b"".join(
            _mouse(rng.choice([0, 2, 32, 35, 64, 65]),
                   rng.randint(1, 60), rng.randint(1, 40),
                   pressed=rng.random() < 0.7)
            for _ in range(rng.randint(1, 12)))
    if pick < 0.8:
        return b"\x1b[200~" + b"paste" * rng.randint(1, 20) + b"\x1b[201~"
    return b"".join(
        b"\x1b[" + rng.choice([b"A", b"B", b"C", b"D", b"Z", b"5~", b"6~"])
        for _ in range(rng.randint(1, 6)))


def chaos_one(program, error_path, seed, budget):
    """Randomly resizes and drives one example. Returns a failure reason or None."""
    rng = random.Random(seed)
    pid, fd = _spawn(program, error_path)
    _set_size(fd, COLUMNS, ROWS)

    def drain(seconds):
        deadline = time.time() + seconds
        while time.time() < deadline:
            readable, _, _ = select.select([fd], [], [], 0.02)
            if not readable:
                continue
            try:
                if not os.read(fd, 65536):
                    return False
            except OSError:
                return False
        return True

    alive = drain(0.8)
    deadline = time.time() + budget
    while alive and time.time() < deadline:
        try:
            if rng.random() < 0.6:
                _set_size(fd, *_chaos_size(rng))
            os.write(fd, _chaos_chunk(rng))
        except OSError:
            break
        # Deliberately short and uneven, so the next resize usually lands while
        # the example is still working through the last chunk.
        alive = drain(rng.uniform(0.02, 0.15))

    status = _reap(pid, fd)
    try:
        os.close(fd)
    except OSError:
        pass
    if status is None:
        return None
    errors = _read_errors(error_path)
    report = _sanitizer_report(errors)
    if report:
        return report
    if os.WIFSIGNALED(status):
        received = os.WTERMSIG(status)
        if received not in (signal.SIGINT, signal.SIGKILL):
            detail = errors.decode("utf-8", "replace").strip().splitlines()
            return "signal %d (%s)%s" % (
                received, signal.Signals(received).name,
                ": " + detail[-1] if detail else "")
    return None


USAGE = """usage: smoke_examples.py [--chaos] [--seed N] [--budget SECONDS] <dir|binary>...

Default: start each example, drive a fixed burst of input and a couple of
resizes at it, and check it survives. This is what CI runs.

--chaos: instead, randomly resize and drive each example for --budget seconds.
Nondeterministic by design, so it is not wired into CI -- run it by hand, and
run it against a sanitizer build, which is the only way most of what it
provokes becomes visible.

A failure prints the seed that produced it. Re-running with --seed replays the
same input, but not the same interleaving: the timing between what is written
and what the example has processed is up to the scheduler, and the bugs worth
finding here live in exactly that gap. Expect to re-run a failing seed several
times, and treat a clean run as inconclusive rather than as a fix.
"""


def main(arguments):
    chaos = False
    seed = int(time.time())
    budget = 2.5
    rest = []
    index = 0
    while index < len(arguments):
        argument = arguments[index]
        if argument == "--chaos":
            chaos = True
        elif argument == "--seed":
            index += 1
            seed = int(arguments[index])
        elif argument == "--budget":
            index += 1
            budget = float(arguments[index])
        elif argument in ("-h", "--help"):
            print(USAGE)
            return 0
        else:
            rest.append(argument)
        index += 1

    programs = expand(rest)
    if not programs:
        print("[SKIP] No examples built; nothing to smoke test.")
        return 0

    error_path = os.path.join(
        tempfile.gettempdir(), "rtxui_smoke_stderr_%d.log" % os.getpid())
    failures = []
    for program in sorted(programs):
        name = os.path.basename(program)
        if chaos:
            # Per-example seed, so one example's failure replays on its own.
            example_seed = seed + (zlib.crc32(name.encode()) & 0xFFFF)
            reason = chaos_one(program, error_path, example_seed, budget)
            if reason:
                reason = "%s (--seed %d)" % (reason, example_seed - (
                    zlib.crc32(name.encode()) & 0xFFFF))
        else:
            reason = run_one(program, error_path)
        if reason:
            failures.append((name, reason))
            print("  FAIL %s: %s" % (name, reason))
    try:
        os.remove(error_path)
    except OSError:
        pass

    print("Checked %d examples, %d failed." % (len(programs), len(failures)))
    if failures:
        print("\n[ERROR] These examples failed the sweep:", file=sys.stderr)
        for name, reason in failures:
            print("  - %s: %s" % (name, reason), file=sys.stderr)
        return 1
    if chaos:
        print("[SUCCESS] Every example survived %.1fs of random input and "
              "resizing (seed %d)." % (budget, seed))
    else:
        print("[SUCCESS] Every example starts, draws, and survives input.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
