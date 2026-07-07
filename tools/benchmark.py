#!/usr/bin/env python3
# Copyright 2026 Arthur Sonzogni. All rights reserved.
# Use of this source code is governed by the MIT license that can be found in
# the LICENSE file.

import argparse
import json
import os
import subprocess
import sys

BENCHMARK_BIN = "./build/rtxui_benchmark"

def run_benchmark():
    """Runs the C++ benchmark executable and parses its JSON output."""
    if not os.path.exists(BENCHMARK_BIN):
        print(f"Error: Benchmark executable '{BENCHMARK_BIN}' not found.")
        print("Please build it first: cmake --build build --target rtxui_benchmark")
        sys.exit(1)

    result = subprocess.run([BENCHMARK_BIN], capture_output=True, text=True)
    if result.returncode != 0:
        print("Error running benchmark binary:")
        print(result.stderr)
        sys.exit(result.returncode)

    try:
        # Find JSON boundaries in output (in case of other stdout prints)
        output = result.stdout
        start = output.find('{')
        end = output.rfind('}') + 1
        if start == -1 or end == 0:
            raise ValueError("No JSON object found in output")
        return json.loads(output[start:end])
    except Exception as e:
        print("Failed to parse benchmark JSON output:")
        print(result.stdout)
        print(e)
        sys.exit(1)

def print_metrics(data, title="Benchmark Results"):
    """Prints benchmark metrics in a clean readable layout."""
    if "avg_parse_us" in data:
        print("=" * 55)
        print(f" {title.upper()} (XML PARSE)")
        print("=" * 55)
        print(f"Iterations: {data.get('iterations')}")
        print("-" * 55)
        print(f"Total time: {data.get('total_time_ms'):.4f} ms")
        print(f"Avg parse:  {data.get('avg_parse_us'):.4f} us")
        print("=" * 55)
        return

    print("=" * 68)
    print(f" {title.upper()}")
    print("=" * 68)
    print(f"Frames evaluated: {data.get('frames')}")
    print("-" * 68)
    print(f"{'Metric':<25} | {'Median (ms)':<12} | {'Avg (ms)':<10} | {'Min (ms)':<10}")
    print("-" * 68)
    for stage in ["digest", "draw", "frame"]:
        name = f"DOM {stage.capitalize()}" if stage == "digest" else (
               f"Layout/Paint ({stage})" if stage == "draw" else f"Total {stage.capitalize()}")
        median_val = data.get(f"median_{stage}_ms", 0.0)
        avg_val = data.get(f"avg_{stage}_ms", 0.0)
        min_val = data.get(f"min_{stage}_ms", 0.0)
        print(f"{name:<25} | {median_val:<12.4f} | {avg_val:<10.4f} | {min_val:<10.4f}")
    print("=" * 68)

def format_diff(baseline_val, current_val, threshold):
    """Returns (diff_str, regressed) comparing current vs baseline.

    Wall-clock benchmarks are noisy, so only changes beyond `threshold`
    percent are colored / flagged. A positive change (slower) beyond the
    threshold counts as a regression.
    """
    if baseline_val <= 0:
        return "N/A", False
    diff_percent = ((current_val - baseline_val) / baseline_val) * 100.0
    diff_str = f"{diff_percent:+.2f}%"
    regressed = False
    if diff_percent < -threshold:
        diff_str = f"\033[92m{diff_str}\033[0m"  # Green: speedup
    elif diff_percent > threshold:
        diff_str = f"\033[91m{diff_str}\033[0m"  # Red: slowdown
        regressed = True
    return diff_str, regressed

def compare_results(baseline_path, current_data, threshold=5.0):
    """Compares current run against a baseline JSON file.

    Uses the median (robust to scheduler-outlier highs and trivial early-out
    frames), and only flags changes beyond `threshold` percent since
    wall-clock timing is inherently noisy and hardware-specific.

    Returns True if any metric regressed beyond the threshold.
    """
    try:
        with open(baseline_path, "r") as f:
            baseline = json.load(f)
    except Exception as e:
        print(f"Failed to read baseline file '{baseline_path}': {e}")
        sys.exit(1)

    any_regression = False

    if "avg_parse_us" in current_data:
        print("=" * 65)
        print(f" PERFORMANCE COMPARISON VS: {os.path.basename(baseline_path)}")
        print(f" (threshold: {threshold:.1f}%)")
        print("=" * 65)
        print(f"{'Metric':<20} | {'Baseline (us)':<15} | {'Current (us)':<15} | {'Change':<10}")
        print("-" * 65)
        b_val = baseline.get("avg_parse_us", 0.0)
        c_val = current_data.get("avg_parse_us", 0.0)
        diff_str, regressed = format_diff(b_val, c_val, threshold)
        any_regression = any_regression or regressed
        print(f"{'XML Parse':<20} | {b_val:<15.4f} | {c_val:<15.4f} | {diff_str:<10}")
        print("=" * 65)
        return any_regression

    print("=" * 65)
    print(f" PERFORMANCE COMPARISON VS: {os.path.basename(baseline_path)}")
    print(f" (metric: median, threshold: {threshold:.1f}%)")
    print("=" * 65)
    print(f"{'Metric':<20} | {'Baseline (ms)':<15} | {'Current (ms)':<15} | {'Change':<10}")
    print("-" * 65)

    for stage in ["digest", "draw", "frame"]:
        name = f"DOM {stage.capitalize()}" if stage == "digest" else (
               f"Layout/Paint ({stage})" if stage == "draw" else f"Total {stage.capitalize()}")
        b_val = baseline.get(f"median_{stage}_ms", 0.0)
        c_val = current_data.get(f"median_{stage}_ms", 0.0)
        diff_str, regressed = format_diff(b_val, c_val, threshold)
        any_regression = any_regression or regressed
        print(f"{name:<20} | {b_val:<15.4f} | {c_val:<15.4f} | {diff_str:<10}")

    print("=" * 65)
    return any_regression

def run_profiler():
    """Runs the benchmark under perf record and displays top hot spots."""
    print("Running under perf profiler...")
    perf_data = "build/perf_benchmark.data"

    # 1. Capture profile
    cmd_record = ["perf", "record", "-g", "-o", perf_data, BENCHMARK_BIN]
    result_record = subprocess.run(cmd_record, capture_output=True, text=True)
    if result_record.returncode != 0:
        print("Failed to run perf record:")
        print(result_record.stderr)
        sys.exit(result_record.returncode)

    # 2. Generate report
    cmd_report = ["perf", "report", "-i", perf_data, "--stdio", "--no-children", "-n"]
    result_report = subprocess.run(cmd_report, capture_output=True, text=True)
    if result_report.returncode != 0:
        print("Failed to generate perf report:")
        print(result_report.stderr)
        sys.exit(result_report.returncode)

    print("\n" + "=" * 80)
    print(" TOP CPU HOT SPOTS (PERF REPORT)")
    print("=" * 80)
    lines = result_report.stdout.splitlines()
    printed_lines = 0
    in_symbol_table = False
    for line in lines:
        if line.startswith("#"):
            if "Overhead" in line:
                in_symbol_table = True
            continue
        if in_symbol_table and line.strip():
            print(line)
            printed_lines += 1
            if printed_lines >= 25:
                break
    print("=" * 80)

def main():
    global BENCHMARK_BIN
    parser = argparse.ArgumentParser(description="RTXUI Benchmark & Profiler Automator")
    parser.add_argument("--save", type=str, metavar="FILE", help="Run benchmark and save results to JSON file")
    parser.add_argument("--compare", type=str, metavar="FILE", help="Run benchmark and compare with saved baseline JSON file")
    parser.add_argument("--threshold", type=float, default=5.0, metavar="PCT", help="Regression threshold in percent for --compare (default: 5.0)")
    parser.add_argument("--profile", action="store_true", help="Run under CPU perf profiler and show top symbols")
    parser.add_argument("--layout", action="store_true", help="Use rtxui_layout_benchmark instead of rtxui_benchmark")
    parser.add_argument("--steady", action="store_true", help="Use rtxui_steady_state_benchmark instead of rtxui_benchmark")
    parser.add_argument("--xml", action="store_true", help="Use rtxui_xml_benchmark instead of rtxui_benchmark")

    args = parser.parse_args()

    if args.layout:
        BENCHMARK_BIN = "./build/rtxui_layout_benchmark"
    elif args.steady:
        BENCHMARK_BIN = "./build/rtxui_steady_state_benchmark"
    elif args.xml:
        BENCHMARK_BIN = "./build/rtxui_xml_benchmark"

    if args.profile:
        run_profiler()
        return

    # Run the benchmark
    current_results = run_benchmark()

    if args.save:
        try:
            with open(args.save, "w") as f:
                json.dump(current_results, f, indent=2)
            print(f"Results successfully saved to '{args.save}'.")
        except Exception as e:
            print(f"Failed to save results: {e}")
            sys.exit(1)
    elif args.compare:
        regressed = compare_results(args.compare, current_results, args.threshold)
        if regressed:
            sys.exit(1)
    else:
        print_metrics(current_results)

if __name__ == "__main__":
    main()
