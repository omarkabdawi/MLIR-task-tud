// RUN: standalone-opt %s --lower-stencil-sweep | FileCheck %s

module {
  func.func @seidel(%A: memref<?x?xf64>) {
    stencil.sweep %A
      offsets = [
        [-1, -1], [-1, 0], [-1, 1],
        [0, -1], [0, 0], [0, 1],
        [1, -1], [1, 0], [1, 1]
      ]
      divisor = 9.000000e+00
      : memref<?x?xf64>

    return
  }

  // CHECK-LABEL: func.func @seidel
  // CHECK: scf.for
  // CHECK: scf.for
  // CHECK: memref.load
  // CHECK: arith.addf
  // CHECK: arith.divf
  // CHECK: memref.store
  // CHECK-NOT: stencil.sweep
}
