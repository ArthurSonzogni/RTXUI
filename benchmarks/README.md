# Benchmarks

Micro-benchmarks for the RTXUI rendering pipeline. Each binary emits JSON on
stdout consumed by [`tools/benchmark.py`](../tools/benchmark.py).

| Binary                          | What it measures                                             |
| ------------------------------- | ----------------------------------------------------------- |
| `rtxui_benchmark`               | Full demo app: Digest + Draw with live state changes.       |
| `rtxui_steady_state_benchmark`  | 500-item grid, nothing mutating (early-out / idle cost).    |
| `rtxui_layout_benchmark`        | 500-item grid, one item mutated per frame (reconciliation). |
| `rtxui_xml_benchmark`           | XML template parse throughput.                              |
| `rtxui_nesting_benchmark`       | Fixed leaf under N levels of block/flex/grid: nesting cost. |

Shared code (timing, stats, JSON format, the stress component) lives in
[`benchmark_common.hpp`](benchmark_common.hpp).

## Running

Build with benchmarks enabled (on by default), then from the repo root:

```bash
python3 tools/benchmark.py                 # main benchmark, pretty table
python3 tools/benchmark.py --layout        # --layout / --steady / --xml / --nesting select a binary
python3 tools/benchmark.py --profile       # run under `perf` and show hot spots
```

## Comparing against a baseline

```bash
python3 tools/benchmark.py --save benchmarks/baseline/main.json          # capture
python3 tools/benchmark.py --compare benchmarks/baseline/main.json       # compare
python3 tools/benchmark.py --steady --compare benchmarks/baseline/steady_state.json --threshold 10
```

Comparison uses the **median** (robust to both scheduler-outlier highs and
trivial early-out frames), and only flags changes beyond `--threshold` percent
(default 5%). `--compare` exits non-zero when any metric regresses past the
threshold.

The nesting benchmark also reports **layout runs** — how many times a layout
algorithm executed for one frame. That number is deterministic, so unlike the
timings it is a portable regression signal: `--compare` flags any growth in it
regardless of the threshold.

## Getting a number you can trust

The **first run of a benchmark process after the machine has been idle reads
about 40% high**, and stays high for that whole process. Measured on the
layout benchmark: 11.4ms, then 8.9ms, then 8.3ms for three back-to-back runs
after an idle gap, with nothing changed in between. The same shape repeats
after every idle period, so a single run is not a measurement.

In-process warmup does not fix it. Each benchmark already runs warmup frames
before measuring, and raising the layout benchmark's from 10 to 100 (roughly
80ms to 800ms of work) still produced 11.6ms on the first run after an idle
gap. Whatever the CPU is ramping, it outlasts a warmup long enough to be
worth paying for.

So, to compare two builds:

1. Build **both** binaries first and keep them side by side; do not rebuild
   between measurements.
2. Throw the first run of each away.
3. Alternate them -- `A B A B A B` -- rather than running all of A then all of
   B, so any drift during the session hits both equally.
4. Compare medians, and treat anything under a few percent as noise.

A difference that only shows up when the two builds are measured in separate
batches is drift, not a change in the code.

## About the committed baselines

`benchmarks/baseline/*.json` are **reference numbers captured on the maintainer's
development machine** — they are hardware-specific and were recorded under
thermal constraints, so absolute values are noisy. Treat them as a local
starting point, not a portable ground truth: regenerate with `--save` on your
own machine before relying on `--compare`.

This is also why CI only *builds and smoke-runs* the benchmarks (to catch
compile/runtime breakage) rather than gating on timing: wall-clock numbers vary
too much across GitHub runners — and even between back-to-back local runs as the
CPU throttles — to serve as a reliable regression gate.
