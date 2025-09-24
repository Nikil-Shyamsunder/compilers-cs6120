# compilers-cs6120

Compiler implementations for CS 6120 at Cornell.

Compiler passes for the [Bril IR](https://github.com/sampsyo/bril):
- Basic block construction
- Control flow graph construction
- Local dead code elimination
- Global dead code elimination
- Local value numbering
- Dataflow analysis (reaching definitions with worklist algorithm)

Primary implementation is in C++ in `src/`. Tests and benchmarking in `tests/`. Legacy Python implementations in `python/`.