# An out-of-tree dialect template for MLIR

This repository contains a template for an out-of-tree [MLIR](https://mlir.llvm.org/) dialect as well as a
standalone `opt`-like tool to operate on that dialect. It is based on
[mlir-standalone-template](https://github.com/jmgorius/mlir-standalone-template)
and is tested with LLVM/MLIR **`llvmorg-23.1.3`**.

## How to build

First build and install LLVM with MLIR enabled, for example:

```sh
git clone --depth 1 --branch llvmorg-23.1.3 https://github.com/llvm/llvm-project.git
cmake -S llvm-project/llvm -B llvm-project/build -G Ninja \
  -DLLVM_ENABLE_PROJECTS=mlir -DLLVM_INSTALL_UTILS=ON \
  -DCMAKE_INSTALL_PREFIX=$PREFIX   # plus the build options of your choice
cmake --build llvm-project/build --target install
```

This setup assumes that you have built LLVM and MLIR in `$BUILD_DIR` and installed them to `$PREFIX`. To build and launch the tests, run
```sh
mkdir build && cd build
cmake -G Ninja .. -DMLIR_DIR=$PREFIX/lib/cmake/mlir -DLLVM_EXTERNAL_LIT=$BUILD_DIR/bin/llvm-lit
cmake --build . --target check-standalone-opt
```
To build the documentation from the TableGen description of the dialect
operations, run
```sh
cmake --build . --target mlir-doc
```
**Note**: Make sure to pass `-DLLVM_INSTALL_UTILS=ON` when building LLVM with
CMake so that it installs `FileCheck` to the chosen installation prefix.

## License

This dialect template is made available under the Apache License 2.0 with LLVM Exceptions. See the `LICENSE.txt` file for more details.

## PolyBench reference implementations

The Part 2 stencil kernels are tested against the PolyBench/C reference
implementations of Jacobi-2D and Seidel-2D.

The PolyBench/C source code is included under `polybench-c/`:
- Upstream repository: https://github.com/hyunsungc/polybench-c
- Included revision: `9f375d85e123c999cb39dd54cbbe6e8c4917d7a8`

PolyBench/C is distributed under the Ohio State University Software
Distribution License. Its original license and copyright notices are
preserved in `polybench-c/LICENSE.txt`.

The license of the PolyBench/C sources is separate from the Apache
License 2.0 with LLVM Exceptions that applies to the dialect template.

## Reproducing the coursework results

The repository includes two scripts that automate the compilation and testing workflows.

### Requirements

- LLVM/MLIR 23.1.3, built with MLIR enabled and installed.
- `clang`, `cmake`, and `ninja`.
- The LLVM build directory containing `llvm-lit`.

The scripts use these default paths:

- LLVM/MLIR installation: `$HOME/mlir-work/llvm-install`
- LLVM build directory: `$HOME/mlir-work/build`

If your paths differ, set `LLVM_INSTALL_DIR` and, for Part 2,
`LLVM_BUILD_DIR` before running the scripts.

### Part 1: Matrix multiplication

Run `./run-part1.sh`.

### Part 2: Stencil dialect and PolyBench comparison

Run `./run-part2.sh`.

The Part 2 script builds `standalone-opt`, runs the dialect tests,
lowers the stencil kernels to LLVM IR, compiles them, and compares
their results against the PolyBench/C reference implementations.

Generated build outputs are written under `build/submission/`.
