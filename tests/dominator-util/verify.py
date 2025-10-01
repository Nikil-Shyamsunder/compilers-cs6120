import subprocess
import glob
import os
import sys

# Path to the directory with .bril files
bril_dir = "../../../bril/benchmarks/core/"

# Expand to all .bril files
bril_files = glob.glob(os.path.join(bril_dir, "*.bril"))
flag = True
for bril_file in bril_files:
    # print(f"Processing {bril_file}...")
    # Run the command: bril2json < file | build/dominator_util
    cmd = f"bril2json < {bril_file} | ../../src/build/dominator_util"
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True)

    # Print stdout
    # if result.stdout.strip():
    #     print(result.stdout)

    # Print stderr if present
    if result.stderr.strip():
        print(f"[stderr from {bril_file}]:\n{result.stderr}", file=sys.stderr)

    # Check for "ERROR" in output
    if "ERROR" in result.stdout or "ERROR" in result.stderr:
        print(f"Error found in output for {bril_file}")
        print(result.stdout)
        flag = False
    
if flag:
    print(f"Passed all checks.")
