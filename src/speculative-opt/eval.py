#!/usr/bin/env python3
import glob
import subprocess
import re
import json
import sys
from pathlib import Path

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
            continue

        # Run baseline
        baseline_output, baseline_inst = run_brili(baseline_json, args)
        if baseline_output is None:
            print(f"{bench_name:<30} {'ERROR':<15} {'-':<15} {'SKIP':<10}")
            continue

        # Find the corresponding merged JSON file
        merged_json_file = f"merged/{bench_name}_merged.json"
        if not Path(merged_json_file).exists():
            print(f"{bench_name:<30} {baseline_inst if baseline_inst else 'N/A':<15} {'NOT FOUND':<15} {'SKIP':<10}")
            continue

        # Read merged JSON
        try:
            with open(merged_json_file, 'r') as f:
                optimized_json = f.read()
        except Exception as e:
            print(f"{bench_name:<30} {baseline_inst if baseline_inst else 'N/A':<15} {'ERROR':<15} {'SKIP':<10}")
            continue

        # Run optimized
        optimized_output, optimized_inst = run_brili(optimized_json, args)
        if optimized_output is None:
            print(f"{bench_name:<30} {baseline_inst if baseline_inst else 'N/A':<15} {'ERROR':<15} {'ERROR':<10}")
            continue

        # Compare outputs (ignore the last line which contains dynamic instruction count)
        baseline_lines = baseline_output.strip().split('\n')[:-1] if baseline_output.strip() else []
        optimized_lines = optimized_output.strip().split('\n')[:-1] if optimized_output.strip() else []
        # print(f"  Baseline output:  {baseline_output.strip()}")
        # print(f"  Optimized output: {optimized_output.strip()}")

        if baseline_lines == optimized_lines:
            baseline_str = str(baseline_inst) if baseline_inst is not None else 'N/A'
            optimized_str = str(optimized_inst) if optimized_inst is not None else 'N/A'
            print(f"{bench_name:<30} {baseline_str:<15} {optimized_str:<15} {'OK':<10}")
        else:
            baseline_str = str(baseline_inst) if baseline_inst is not None else 'N/A'
            optimized_str = str(optimized_inst) if optimized_inst is not None else 'N/A'
            print(f"{bench_name:<30} {baseline_str:<15} {optimized_str:<15} {'ERROR':<10}")

if __name__ == '__main__':
    main()
