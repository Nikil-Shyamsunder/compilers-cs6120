#!/bin/bash

# Test SSA transformation on all Bril benchmark files
# Usage: ./test_all_benchmarks.sh

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SSA_BUILD="$SCRIPT_DIR/../../src/build/to_ssa"
IS_SSA_SCRIPT="$SCRIPT_DIR/is_ssa.py"
BENCHMARK_DIR="$SCRIPT_DIR/../../../bril/benchmarks/core"

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Counters
total=0
passed=0
failed=0

echo "Testing SSA transformation on benchmark files..."
echo "================================================"
echo ""

# Check if directories exist
if [ ! -f "$SSA_BUILD" ]; then
    echo -e "${RED}Error: SSA binary not found at $SSA_BUILD${NC}"
    echo "Please build it first with: cd src && make to_ssa"
    exit 1
fi

if [ ! -d "$BENCHMARK_DIR" ]; then
    echo -e "${RED}Error: Benchmark directory not found at $BENCHMARK_DIR${NC}"
    exit 1
fi

# Process each .bril file in the benchmark directory
for bril_file in "$BENCHMARK_DIR"/*.bril; do
    if [ ! -f "$bril_file" ]; then
        continue
    fi

    filename=$(basename "$bril_file")
    total=$((total + 1))

    # Run the SSA transformation and check if output is valid SSA
    result=$(bril2json < "$bril_file" 2>/dev/null | "$SSA_BUILD" 2>/dev/null | python3 "$IS_SSA_SCRIPT" 2>/dev/null)

    if [ $? -eq 0 ] && [ "$result" = "yes" ]; then
        echo -e "${GREEN}✓${NC} $filename"
        passed=$((passed + 1))
    else
        echo -e "${RED}✗${NC} $filename (result: $result)"
        failed=$((failed + 1))
    fi
done

echo ""
echo "================================================"
echo -e "Results: ${GREEN}$passed passed${NC}, ${RED}$failed failed${NC} out of $total tests"

if [ $failed -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed.${NC}"
    exit 1
fi
