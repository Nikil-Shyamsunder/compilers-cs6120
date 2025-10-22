# llvm-pass-swapSub

For all integer and floating point subtraction operations, this LLVM pass swaps the LHS and RHS operands of the operation.
It's for LLVM 17.

Build:

    $ mkdir build
    $ cd build
    $ cmake ..
    $ make
    $ cd ..

Run:

    $ clang -fpass-plugin=`echo build/swapSub/SwapSubPass.*` something.c
