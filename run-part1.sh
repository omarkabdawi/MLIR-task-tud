#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LLVM_INSTALL_DIR="${LLVM_INSTALL_DIR:-$HOME/mlir-work/llvm-install}"

MLIR_OPT="$LLVM_INSTALL_DIR/bin/mlir-opt"
MLIR_TRANSLATE="$LLVM_INSTALL_DIR/bin/mlir-translate"
LLC="$LLVM_INSTALL_DIR/bin/llc"

for tool in "$MLIR_OPT" "$MLIR_TRANSLATE" "$LLC"; do
    if [[ ! -x "$tool" ]]; then
        echo "Missing tool: $tool" >&2
        exit 1
    fi
done

OUT="$ROOT_DIR/build/submission/part1"
mkdir -p "$OUT"

echo "=== Lowering original matrix multiplication ==="

"$MLIR_OPT" "$ROOT_DIR/matmul.mlir" \
    --convert-linalg-to-affine-loops \
    --verify-each \
    -o "$OUT/matmul-affine.mlir"

echo "=== Lowering original version to LLVM ==="

"$MLIR_OPT" "$OUT/matmul-affine.mlir" \
    --lower-affine \
    --convert-scf-to-cf \
    --convert-arith-to-llvm \
    --finalize-memref-to-llvm \
    --llvm-request-c-wrappers \
    --convert-func-to-llvm \
    --convert-cf-to-llvm \
    --reconcile-unrealized-casts \
    --verify-each \
    -o "$OUT/matmul-llvm.mlir"

echo "=== Lowering interchanged version to LLVM ==="

"$MLIR_OPT" "$ROOT_DIR/matmul-affine-interchange.mlir" \
    --lower-affine \
    --convert-scf-to-cf \
    --convert-arith-to-llvm \
    --finalize-memref-to-llvm \
    --llvm-request-c-wrappers \
    --convert-func-to-llvm \
    --convert-cf-to-llvm \
    --reconcile-unrealized-casts \
    --verify-each \
    -o "$OUT/matmul-interchange-llvm.mlir"

echo "=== Compiling to object files ==="

"$MLIR_TRANSLATE" --mlir-to-llvmir \
    "$OUT/matmul-llvm.mlir" -o "$OUT/matmul.ll"

"$MLIR_TRANSLATE" --mlir-to-llvmir \
    "$OUT/matmul-interchange-llvm.mlir" -o "$OUT/matmul-interchange.ll"

"$LLC" -filetype=obj "$OUT/matmul.ll" -o "$OUT/matmul.o"
"$LLC" -filetype=obj "$OUT/matmul-interchange.ll" \
    -o "$OUT/matmul-interchange.o"

echo "=== Linking and running original version ==="

clang "$ROOT_DIR/driver.c" "$OUT/matmul.o" -o "$OUT/matmul_test"
"$OUT/matmul_test"

echo "=== Linking and running interchanged version ==="

clang "$ROOT_DIR/driver.c" "$OUT/matmul-interchange.o" \
    -o "$OUT/matmul_interchange_test"
"$OUT/matmul_interchange_test"

echo "=== Part 1 completed successfully ==="
