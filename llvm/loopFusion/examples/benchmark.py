#!/usr/bin/env python3
"""
Benchmark script to compare performance between optimized and baseline compilations.
Runs both executables multiple times and reports statistics.
"""

import subprocess
import re
import statistics
import sys
from pathlib import Path

# Configuration
NUM_RUNS = 1000  # Number of times to run each executable
OPTIMIZED_BIN = "./feedForward_optimized"
BASELINE_BIN = "./feedForward_baseline"


def run_executable(executable_path, run_number=None):
    """
    Run an executable and extract the elapsed time from its output.

    Args:
        executable_path: Path to the executable
        run_number: Optional run number for progress display

    Returns:
        Elapsed time in microseconds, or None if parsing failed
    """
    try:
        result = subprocess.run(
            [executable_path],
            capture_output=True,
            text=True,
            timeout=30
        )

        # Parse the elapsed time from output
        # Expected format: "Elapsed time: 123456 microseconds"
        match = re.search(r'Elapsed time: (\d+) microseconds', result.stdout)
        if match:
            elapsed_us = int(match.group(1))
            # if run_number is not None:
            #     print(f"  Run {run_number + 1}: {elapsed_us} μs")
            return elapsed_us
        else:
            print(f"Warning: Could not parse timing from {executable_path}")
            print(f"Output: {result.stdout}")
            return None

    except subprocess.TimeoutExpired:
        print(f"Error: {executable_path} timed out")
        return None
    except Exception as e:
        print(f"Error running {executable_path}: {e}")
        return None


def benchmark_executable(executable_path, name):
    """
    Benchmark an executable by running it multiple times.

    Args:
        executable_path: Path to the executable
        name: Display name for the executable

    Returns:
        Dictionary with timing statistics
    """
    print(f"\n{'='*60}")
    print(f"Benchmarking: {name}")
    print(f"{'='*60}")

    # Check if executable exists
    if not Path(executable_path).exists():
        print(f"Error: {executable_path} not found!")
        return None

    times = []
    for i in range(NUM_RUNS):
        elapsed = run_executable(executable_path, i)
        if elapsed is not None:
            times.append(elapsed)

    if not times:
        print(f"No successful runs for {name}")
        return None

    # Calculate statistics
    stats = {
        'mean': statistics.mean(times),
        'median': statistics.median(times),
        'stdev': statistics.stdev(times) if len(times) > 1 else 0,
        'min': min(times),
        'max': max(times),
        'runs': len(times)
    }

    print(f"\nStatistics:")
    print(f"  Mean:   {stats['mean']:.2f} μs")
    print(f"  Median: {stats['median']:.2f} μs")
    print(f"  StdDev: {stats['stdev']:.2f} μs")
    print(f"  Min:    {stats['min']} μs")
    print(f"  Max:    {stats['max']} μs")

    return stats


def main():
    """Main benchmarking routine."""
    print("Matrix-Vector Multiply Benchmark")
    print("Comparing optimized vs baseline compilation")

    # Benchmark baseline version
    baseline_stats = benchmark_executable(BASELINE_BIN, "Baseline (no optimization pass)")

    # Benchmark optimized version
    optimized_stats = benchmark_executable(OPTIMIZED_BIN, "Optimized (with skeleton pass)")

    # Compare results
    if baseline_stats and optimized_stats:
        print(f"\n{'='*60}")
        print("COMPARISON SUMMARY")
        print(f"{'='*60}")

        baseline_mean = baseline_stats['mean']
        optimized_mean = optimized_stats['mean']

        speedup = baseline_mean / optimized_mean
        improvement_pct = ((baseline_mean - optimized_mean) / baseline_mean) * 100

        print(f"\nBaseline mean:  {baseline_mean:.2f} μs")
        print(f"Optimized mean: {optimized_mean:.2f} μs")
        print(f"\nSpeedup:        {speedup:.3f}x")

        if improvement_pct > 0:
            print(f"Improvement:    {improvement_pct:.2f}% faster")
        else:
            print(f"Regression:     {abs(improvement_pct):.2f}% slower")

        # Statistical significance (simple check)
        baseline_ci = 1.96 * baseline_stats['stdev'] / (baseline_stats['runs'] ** 0.5)
        optimized_ci = 1.96 * optimized_stats['stdev'] / (optimized_stats['runs'] ** 0.5)

        print(f"\n95% Confidence Intervals:")
        print(f"  Baseline:  {baseline_mean:.2f} ± {baseline_ci:.2f} μs")
        print(f"  Optimized: {optimized_mean:.2f} ± {optimized_ci:.2f} μs")

        # Check if difference is significant
        if abs(baseline_mean - optimized_mean) > (baseline_ci + optimized_ci):
            print(f"\nResult: Difference appears statistically significant")
        else:
            print(f"\nResult: Difference may not be statistically significant")

        print(f"{'='*60}\n")

        return 0
    else:
        print("\nError: Could not complete benchmark")
        return 1


if __name__ == "__main__":
    sys.exit(main())
