#!/usr/bin/env python3
import glob
import subprocess
import re
import json
import sys
import csv
from pathlib import Path
import matplotlib.pyplot as plt
import numpy as np

def extract_args(bril_file):
    """Extract ARGS from the bril file comments."""
    try:
        with open(bril_file, 'r') as f:
            for line in f:
                match = re.match(r'^\s*#\s*ARGS:\s*(.*)', line)
                if match:
                    return match.group(1).strip().split()
    except Exception as e:
        print(f"Error reading {bril_file}: {e}", file=sys.stderr)
    return []

def bril_to_json(bril_file):
    """Convert bril file to JSON."""
    try:
        result = subprocess.run(
            ['bril2json'],
            stdin=open(bril_file, 'r'),
            capture_output=True,
            text=True,
            check=True
        )
        return result.stdout
    except subprocess.CalledProcessError as e:
        print(f"Error converting {bril_file} to JSON: {e.stderr}", file=sys.stderr)
        return None

def run_brili(json_input, args):
    """Run brili with the given JSON and arguments, return output and total_dyn_inst."""
    try:
        cmd = ['brili', '-p'] + args
        result = subprocess.run(
            cmd,
            input=json_input,
            capture_output=True,
            text=True,
            check=True
        )

        # Extract total_dyn_inst from output
        match = re.search(r'total_dyn_inst:\s*(\d+)', result.stderr)
        total_dyn_inst = int(match.group(1)) if match else None

        return result.stdout, total_dyn_inst
    except subprocess.CalledProcessError as e:
        print(f"Error running brili: {e.stderr}", file=sys.stderr)
        return None, None

def main():
    benchmark_pattern = '../../../bril/benchmarks/core/*.bril'
    benchmark_files = glob.glob(benchmark_pattern)

    if not benchmark_files:
        print(f"No benchmark files found matching: {benchmark_pattern}", file=sys.stderr)
        return

    # Track statistics
    ok_count = 0
    error_count = 0
    skip_count = 0

    # Store results for CSV export
    csv_results = []

    print(f"{'Benchmark':<30} {'Baseline':<15} {'Optimized':<15} {'Status':<10}")
    print("=" * 75)

    for bril_file in sorted(benchmark_files):
        bench_name = Path(bril_file).stem

        # Get ARGS from the bril file
        args = extract_args(bril_file)

        # Convert baseline bril to JSON
        baseline_json = bril_to_json(bril_file)
        if baseline_json is None:
            print(f"{bench_name:<30} {'ERROR':<15} {'-':<15} {'SKIP':<10}")
            skip_count += 1
            continue

        # Run baseline
        baseline_output, baseline_inst = run_brili(baseline_json, args)
        if baseline_output is None:
            print(f"{bench_name:<30} {'ERROR':<15} {'-':<15} {'SKIP':<10}")
            skip_count += 1
            continue

        # Find the corresponding merged JSON file
        merged_json_file = f"merged/{bench_name}_merged.json"
        if not Path(merged_json_file).exists():
            print(f"{bench_name:<30} {baseline_inst if baseline_inst else 'N/A':<15} {'NOT FOUND':<15} {'SKIP':<10}")
            skip_count += 1
            continue

        # Read merged JSON
        try:
            with open(merged_json_file, 'r') as f:
                optimized_json = f.read()
        except Exception as e:
            print(f"{bench_name:<30} {baseline_inst if baseline_inst else 'N/A':<15} {'ERROR':<15} {'SKIP':<10}")
            skip_count += 1
            continue

        # Run optimized
        optimized_output, optimized_inst = run_brili(optimized_json, args)
        if optimized_output is None:
            print(f"{bench_name:<30} {baseline_inst if baseline_inst else 'N/A':<15} {'ERROR':<15} {'ERROR':<10}")
            error_count += 1
            continue

        # Compare outputs (ignore the last line which contains dynamic instruction count)
        baseline_lines = baseline_output.strip().split('\n')[:-1] if baseline_output.strip() else []
        optimized_lines = optimized_output.strip().split('\n')[:-1] if optimized_output.strip() else []

        if baseline_lines == optimized_lines:
            baseline_str = str(baseline_inst) if baseline_inst is not None else 'N/A'
            optimized_str = str(optimized_inst) if optimized_inst is not None else 'N/A'
            print(f"{bench_name:<30} {baseline_str:<15} {optimized_str:<15} {'OK':<10}")
            ok_count += 1

            # Calculate overhead (percentage increase)
            if baseline_inst is not None and optimized_inst is not None:
                overhead = ((optimized_inst - baseline_inst) / baseline_inst) * 100
                csv_results.append({
                    'benchmark': bench_name,
                    'baseline': baseline_inst,
                    'optimized': optimized_inst,
                    'overhead_percent': overhead
                })
        else:
            baseline_str = str(baseline_inst) if baseline_inst is not None else 'N/A'
            optimized_str = str(optimized_inst) if optimized_inst is not None else 'N/A'
            print(f"{bench_name:<30} {baseline_str:<15} {optimized_str:<15} {'ERROR':<10}")
            error_count += 1

    # Print summary
    print("=" * 75)
    total = ok_count + error_count + skip_count
    print(f"\nSummary:")
    print(f"  Total benchmarks: {total}")
    print(f"  OK: {ok_count}")
    print(f"  Errors: {error_count}")
    print(f"  Skipped: {skip_count}")

    # Export CSV for OK benchmarks
    if csv_results:
        csv_file = 'results.csv'
        with open(csv_file, 'w', newline='') as f:
            writer = csv.DictWriter(f, fieldnames=['benchmark', 'baseline', 'optimized', 'overhead_percent'])
            writer.writeheader()
            writer.writerows(csv_results)
        print(f"\nResults exported to {csv_file}")

        # Generate bar chart
        try:
            benchmarks = [r['benchmark'] for r in csv_results]
            baseline_counts = [r['baseline'] for r in csv_results]
            optimized_counts = [r['optimized'] for r in csv_results]

            x = np.arange(len(benchmarks))
            width = 0.35

            fig, ax = plt.subplots(figsize=(14, 8))
            bars1 = ax.bar(x - width/2, baseline_counts, width, label='Baseline', alpha=0.8)
            bars2 = ax.bar(x + width/2, optimized_counts, width, label='Optimized', alpha=0.8)

            ax.set_ylabel('Dynamic Instruction Count (log scale)')
            ax.set_xlabel('Benchmark')
            ax.set_title('Baseline vs Optimized Instruction Counts')
            ax.set_xticks(x)
            ax.set_xticklabels(benchmarks, rotation=45, ha='right')
            ax.legend()
            ax.set_yscale('log')
            ax.grid(True, which='both', alpha=0.3, axis='y')

            plt.tight_layout()
            chart_file = 'instruction_counts.png'
            plt.savefig(chart_file, dpi=300, bbox_inches='tight')
            print(f"Bar chart saved to {chart_file}")
            plt.close()
        except Exception as e:
            print(f"Warning: Could not generate chart: {e}", file=sys.stderr)

if __name__ == '__main__':
    main()
