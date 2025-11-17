# Project Proposal: Allo -> ASIC Compiler Backend

**What will you do?**

Build an ASIC-oriented backend for the **Allo** compiler that targets one (we haven't figured out which) of:

1. **Siemens Catapult HLS** (extend the existing Allo -> Catapult path to support kernels currently unsupported), and/or
2. **Google XLS** (create a new Allo -> XLS codegen path from scratch).

We’ll pick a few (like 1-3) tractable Allo kernels (e.g., 2D conv or other examples in https://github.com/cornell-zhang/allo/tree/main/examples) that are either (1) unsupported by the current Catapult backend or (2) clearly unsupported due to the absence of any XLS backend.

We want to build the following:
- A working backend capable of compiling the chosen kernels from Allo IR to the ASIC tool’s input (Catapult-friendly C++ with directives **or** XLS DSLX).
- Golden tests

**How will you do it?**
- Select 2–3 kernels that define a tractable subset of Allo to write a backend for
- Define each kernel’s Allo features used
  - For Catapult HLS: Generate Catapult-friendly C++/SystemC from Allo IR; emit pragmas (e.g., pipeline, unroll, memory/array partitioning) inferred from Allo’s scheduling metadata.
  - For Google XLS: Lower Allo IR to XLS IR/DSLX constructs
- Add a lowering pass for the target
- Write equivalence test benches that compare Allo reference outputs vs. target-sim outputs.

**How will you empirically measure success?**
- That our system builds/runs: The chosen nontrivial kernels compile and run end-to-end on the target.
- That our system is correct: Exact output equivalence against Allo golden references across randomized inputs and edge cases.
- That our system is somewhat fast/quality: To test quality, maybe we synthesize to RTL and measure area proxies and timing?

## Team members
- Cynthia Shao (@CynyuS)
- This is joint work with 2 other students in ECE 6775. However, we will clearly delineate which components were built by us.
