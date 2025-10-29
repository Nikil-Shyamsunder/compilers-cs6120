# compilers-cs6120

Compiler implementations for CS 6120 at Cornell using the [Bril IR](https://github.com/sampsyo/bril) and the LLVM IR.
- Basic block construction
- Control flow graph construction
- Local dead code elimination
- Global dead code elimination
- Local value numbering
- Dataflow analysis (reaching definitions with worklist algorithm)
- Dominance relation, Dominator tree, and dominance frontier computation on CFGs
- Into SSA and out of SSA transformations
- LLVM Passes (e.g. simple loop fusion pass)

Primary implementations for Bril in C++ in `src/`. LLVM passes in `llvm/`. Tests and benchmarking in `tests/`. Legacy Python implementations in `python/`.
