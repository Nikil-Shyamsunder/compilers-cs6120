# LLVM Loop Fusion Pass

An LLVM optimization pass that performs loop fusion on adjacent loops with matching trip counts.
Built for LLVM 17.

## Overview

This pass detects and fuses consecutive loops that:
- Are adjacent in control flow (one loop's exit leads to the next loop's entry)
- Have identical trip counts (same number of iterations)
- Are top-level loops in a function

When these conditions are met, the pass merges the second loop's body into the first loop, reducing loop overhead and potentially improving cache locality.

## Build

```bash
cd loopFusion
mkdir build
cd build
cmake ..
make
cd ..
```

## Usage

Compile your C/C++ code with the loop fusion pass enabled:

```bash
clang -fpass-plugin=`echo build/skeleton/SkeletonPass.*` your_code.c -o output
```

For LLVM IR files:

```bash
opt -load-pass-plugin=build/skeleton/SkeletonPass.so -passes="skeleton-pass" input.ll -o output.ll
```

## How It Works

1. **Loop Adjacency Check**: Verifies that the first loop's exit block directly leads to the second loop's preheader
2. **Trip Count Analysis**: Uses LLVM's ScalarEvolution to compute and compare loop trip counts
3. **Fusion**: If conditions are met, clones the second loop's instructions into the first loop's body
4. **Induction Variable Mapping**: Maps the second loop's induction variable to the first loop's to maintain correctness

## Example

Before fusion:
```c
for (int i = 0; i < N; i++) {
    a[i] = b[i] + c[i];
}
for (int i = 0; i < N; i++) {
    d[i] = a[i] * 2;
}
```

After fusion:
```c
for (int i = 0; i < N; i++) {
    a[i] = b[i] + c[i];
    d[i] = a[i] * 2;
}
```
