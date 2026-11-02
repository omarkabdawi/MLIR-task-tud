#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LLVM_INSTALL_DIR="${LLVM_INSTALL_DIR:-$HOME/mlir-work/llvm-install}"
LLVM_BUILD_DIR="${LLVM_BUILD_DIR:-$HOME/mlir-work/build}"
BUILD_DIR="$ROOT_DIR/build/submission/part2"

MLIR_OPT="$LLVM_INSTALL_DIR/bin/mlir-opt"
MLIR_TRANSLATE="$LLVM_INSTALL_DIR/bin/mlir-translate"
LLC="$LLVM_INSTALL_DIR/bin/llc"

for tool in "$MLIR_OPT" "$MLIR_TRANSLATE" "$LLC"; do
    if [[ ! -x "$tool" ]]; then
        echo "Missing tool: $tool" >&2
        exit 1
    fi
done

if [[ ! -f "$LLVM_INSTALL_DIR/lib/cmake/mlir/MLIRConfig.cmake" ]]; then
    echo "MLIRConfig.cmake not found in $LLVM_INSTALL_DIR" >&2
    exit 1
fi

if [[ ! -x "$LLVM_BUILD_DIR/bin/llvm-lit" ]]; then
    echo "llvm-lit not found in $LLVM_BUILD_DIR/bin" >&2
    exit 1
fi

mkdir -p "$BUILD_DIR"

echo "=== Configuring custom dialect ==="

cmake -S "$ROOT_DIR" -B "$BUILD_DIR/cmake" -G Ninja \
    -DMLIR_DIR="$LLVM_INSTALL_DIR/lib/cmake/mlir" \
    -DLLVM_EXTERNAL_LIT="$LLVM_BUILD_DIR/bin/llvm-lit"

echo "=== Building standalone-opt and running tests ==="

cmake --build "$BUILD_DIR/cmake" --target standalone-opt -j2
cmake --build "$BUILD_DIR/cmake" --target check-standalone-opt

OPT="$BUILD_DIR/cmake/bin/standalone-opt"

echo "=== Lowering stencil operations ==="

"$OPT" "$ROOT_DIR/stencil-kernels.mlir" \
    --lower-stencil-apply \
    --lower-stencil-sweep \
    --verify-each \
    -o "$BUILD_DIR/stencil-lowered.mlir"

echo "=== Lowering Linalg to loops ==="

"$MLIR_OPT" "$BUILD_DIR/stencil-lowered.mlir" \
    --convert-linalg-to-loops \
    --verify-each \
    -o "$BUILD_DIR/stencil-loops.mlir"

echo "=== Lowering to LLVM dialect ==="

"$MLIR_OPT" "$BUILD_DIR/stencil-loops.mlir" \
    --expand-strided-metadata \
    --lower-affine \
    --convert-scf-to-cf \
    --convert-arith-to-llvm \
    --finalize-memref-to-llvm \
    --llvm-request-c-wrappers \
    --convert-func-to-llvm \
    --convert-cf-to-llvm \
    --reconcile-unrealized-casts \
    --verify-each \
    -o "$BUILD_DIR/stencil-llvm.mlir"

echo "=== Translating and compiling ==="

"$MLIR_TRANSLATE" --mlir-to-llvmir \
    "$BUILD_DIR/stencil-llvm.mlir" \
    -o "$BUILD_DIR/stencil.ll"

"$LLC" -filetype=obj \
    "$BUILD_DIR/stencil.ll" \
    -o "$BUILD_DIR/stencil.o"

echo "=== Checking PolyBench sources ==="

POLYBENCH="$ROOT_DIR/polybench-c"

if [[ ! -f "$POLYBENCH/utilities/polybench.c" ]]; then
    echo "PolyBench sources not found in $POLYBENCH" >&2
    exit 1
fi

echo "=== Linking comparison driver ==="

clang -std=c99 \
    -D_POSIX_C_SOURCE=200112L \
    -DPOLYBENCH_USE_C99_PROTO \
    -I"$POLYBENCH/utilities" \
    "$ROOT_DIR/stencil-driver.c" \
    "$ROOT_DIR/polybench-jacobi.c" \
    "$ROOT_DIR/polybench-seidel.c" \
    "$POLYBENCH/utilities/polybench.c" \
    "$BUILD_DIR/stencil.o" \
    -lm \
    -o "$BUILD_DIR/stencil-test"

echo "=== Running comparisons ==="

"$BUILD_DIR/stencil-test"

echo "=== Part 2 completed successfully ==="
