#!/bin/bash

set -e

PASS="build/skeleton/SkeletonPass.dylib"
COUNTER_SRC="counter.c"
SDK_PATH=$(xcrun --show-sdk-path)

clang -isysroot "$SDK_PATH" -c "$COUNTER_SRC" -o counter.o

echo "==> fib_iterative"
echo "----------------------------------------"
clang -isysroot "$SDK_PATH" -fpass-plugin="$PASS" -O1 -c "fib_iterative.c" -o test.o 2>&1
clang -isysroot "$SDK_PATH" counter.o test.o -o instrumented_program
./instrumented_program
echo ""

echo "==> fib_recursive"
echo "----------------------------------------"
clang -isysroot "$SDK_PATH" -fpass-plugin="$PASS" -O1 -c "fib_recursive.c" -o test.o 2>&1
clang -isysroot "$SDK_PATH" counter.o test.o -o instrumented_program
./instrumented_program
echo ""

rm -f counter.o test.o instrumented_program
