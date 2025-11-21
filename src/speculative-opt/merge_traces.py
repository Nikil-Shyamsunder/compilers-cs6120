import json
import os
import subprocess
from pathlib import Path

BENCHMARKS_DIR = Path("../../../bril/benchmarks/core/")
TRACES_DIR = Path("processed_traces/")
OUTPUT_DIR = Path("merged/")

def main():
    bril_files = sorted(BENCHMARKS_DIR.glob("*.bril"))

    for bril_file in bril_files:
        base_name = bril_file.stem

        trc_file = TRACES_DIR / f"{base_name}.trc"

        if not trc_file.exists():
            print(f"Warning: No trace file found for {base_name}, skipping")
            continue

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

        trace_instructions = []
        last_line_num = None
        with open(trc_file, 'r') as f:
            lines = f.readlines()
            for line in lines:
                line = line.strip()
                if line: 
                    try:
                        last_line_num = int(line)
                    except ValueError:
                        try:
                            trace_instructions.append(json.loads(line))
                        except json.JSONDecodeError as e:
                            print(f"Warning: Could not parse line in {trc_file}: {line}")

        for func in bril_json.get("functions", []):
            if func.get("name") == "main":
                if "instrs" not in func or not func["instrs"]:
                    print(f"Warning: No instructions in main function of {bril_file}")
                    continue

                original_instrs = func["instrs"]

                success_label_idx = None
                if last_line_num is not None:
                    instr_count = 0
                    for idx, orig_instr in enumerate(original_instrs):
                        if "label" not in orig_instr:
                            if instr_count == last_line_num + 1:  # +1 because we want the next instruction
                                success_label_idx = idx
                                break
                            instr_count += 1

                # Build new instruction list
                new_instrs = []

                new_instrs.extend(trace_instructions)

                new_instrs.append({"label": "end_speculation"})

                if success_label_idx is not None:
                    original_instrs.insert(success_label_idx, {"label": "success"})
                    new_instrs.extend(original_instrs)
                else:
                    # if no match found, just add all original instructions
                    # and put success label at the beginning
                    new_instrs.append({"label": "success"})
                    new_instrs.extend(original_instrs)

                func["instrs"] = new_instrs
                break
        else:
            print(f"Warning: No 'main' function found in {bril_file}")
            continue

        # output merged
        output_file = OUTPUT_DIR / f"{base_name}_merged.json"
        with open(output_file, 'w') as f:
            json.dump(bril_json, f, indent=2)

        print(f"Processed {base_name}: {len(trace_instructions)} trace instructions added")

if __name__ == "__main__":
    main()
