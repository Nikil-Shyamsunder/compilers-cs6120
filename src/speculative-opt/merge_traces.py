#!/usr/bin/env python3
import json
import os
import subprocess
from pathlib import Path

# Directories
BENCHMARKS_DIR = Path("../../../bril/benchmarks/core/")
TRACES_DIR = Path("traces/")
OUTPUT_DIR = Path("merged/")

def main():
    # Get all .bril files in the benchmarks directory
    bril_files = sorted(BENCHMARKS_DIR.glob("*.bril"))

    for bril_file in bril_files:
        # Get the base name (without extension)
        base_name = bril_file.stem

        # Find corresponding .trc file
        trc_file = TRACES_DIR / f"{base_name}.trc"

        if not trc_file.exists():
            print(f"Warning: No trace file found for {base_name}, skipping")
            continue

        # Convert .bril to JSON using bril2json
        try:
            result = subprocess.run(
                ["bril2json"],
                stdin=open(bril_file, 'r'),
                capture_output=True,
                text=True,
                check=True
            )
            bril_json = json.loads(result.stdout)
        except subprocess.CalledProcessError as e:
            print(f"Error converting {bril_file} to JSON: {e}")
            continue
        except json.JSONDecodeError as e:
            print(f"Error parsing JSON for {bril_file}: {e}")
            continue

        # Read the trace file (each line is a JSON object)
        trace_instructions = []
        with open(trc_file, 'r') as f:
            for line in f:
                line = line.strip()
                if line:  # Skip empty lines
                    try:
                        trace_instructions.append(json.loads(line))
                    except json.JSONDecodeError as e:
                        print(f"Warning: Could not parse line in {trc_file}: {line}")

        # Find the main function and insert trace instructions
        for func in bril_json.get("functions", []):
            if func.get("name") == "main":
                # Insert trace instructions at the beginning of the main function's instructions
                if "instrs" in func:
                    func["instrs"] = trace_instructions + func["instrs"]
                else:
                    func["instrs"] = trace_instructions
                break
        else:
            print(f"Warning: No 'main' function found in {bril_file}")
            continue

        # Output the merged JSON
        output_file = OUTPUT_DIR / f"{base_name}_merged.json"
        with open(output_file, 'w') as f:
            json.dump(bril_json, f, indent=2)

        print(f"Processed {base_name}: {len(trace_instructions)} trace instructions added")

if __name__ == "__main__":
    main()
