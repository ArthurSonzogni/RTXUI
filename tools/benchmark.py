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
    print("=" * 55)
    print(f" {title.upper()}")
    print("=" * 55)
    print(f"Frames evaluated: {data.get('frames')}")
    print("-" * 55)
    print(f"{'Metric':<25} | {'Avg (ms)':<10} | {'Min (ms)':<10} | {'Max (ms)':<10}")
    print("-" * 55)
    for stage in ["digest", "draw", "frame"]:
        name = f"DOM {stage.capitalize()}" if stage == "digest" else (
               f"Layout/Paint ({stage})" if stage == "draw" else f"Total {stage.capitalize()}")
        avg_val = data.get(f"avg_{stage}_ms", 0.0)
        min_val = data.get(f"min_{stage}_ms", 0.0)
        max_val = data.get(f"max_{stage}_ms", 0.0)
        print(f"{name:<25} | {avg_val:<10.4f} | {min_val:<10.4f} | {max_val:<10.4f}")
    print("=" * 55)

def compare_results(baseline_path, current_data):
    """Compares current benchmark run against a baseline JSON file."""
    try:
        with open(baseline_path, "r") as f:
            baseline = json.load(f)
    except Exception as e:
        print(f"Failed to read baseline file '{baseline_path}': {e}")
        sys.exit(1)

    print("=" * 65)
    print(f" PERFORMANCE COMPARISON VS: {os.path.basename(baseline_path)}")
    print("=" * 65)
    print(f"{'Metric':<20} | {'Baseline (ms)':<15} | {'Current (ms)':<15} | {'Change':<10}")
    print("-" * 65)

    for stage in ["digest", "draw", "frame"]:
        name = f"DOM {stage.capitalize()}" if stage == "digest" else (
               f"Layout/Paint ({stage})" if stage == "draw" else f"Total {stage.capitalize()}")
        b_val = baseline.get(f"avg_{stage}_ms", 0.0)
        c_val = current_data.get(f"avg_{stage}_ms", 0.0)

        if b_val > 0:
            diff_percent = ((c_val - b_val) / b_val) * 100.0
            diff_str = f"{diff_percent:+.2f}%"
            # Color code: green for speedup (negative difference), red for slowdown
            if diff_percent < -1.0:
                diff_str = f"\033[92m{diff_str}\033[0m" # Green
            elif diff_percent > 1.0:
                diff_str = f"\033[91m{diff_str}\033[0m" # Red
        else:
            diff_str = "N/A"

        print(f"{name:<20} | {b_val:<15.4f} | {c_val:<15.4f} | {diff_str:<10}")

    print("=" * 65)

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
    parser = argparse.ArgumentParser(description="RTXUI Benchmark & Profiler Automator")
    parser.add_argument("--save", type=str, metavar="FILE", help="Run benchmark and save results to JSON file")
    parser.add_argument("--compare", type=str, metavar="FILE", help="Run benchmark and compare with saved baseline JSON file")
    parser.add_argument("--profile", action="store_true", help="Run under CPU perf profiler and show top symbols")

    args = parser.parse_args()

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
        compare_results(args.compare, current_results)
    else:
        print_metrics(current_results)

if __name__ == "__main__":
    main()
