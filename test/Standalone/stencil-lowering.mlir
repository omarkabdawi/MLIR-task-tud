// RUN: standalone-opt %s --lower-stencil-apply | FileCheck %s

module {
  func.func @jacobi(%A: memref<?x?xf64>, %B: memref<?x?xf64>) {
    stencil.apply %A, %B
      offsets = [[0, 0], [0, -1], [0, 1], [1, 0], [-1, 0]]
      scale = 2.000000e-01
      : memref<?x?xf64>, memref<?x?xf64>

    return
  }

  // CHECK-LABEL: func.func @jacobi
  // CHECK: memref.subview
  // CHECK: memref.subview
  // CHECK: linalg.generic
  // CHECK: iterator_types = ["parallel", "parallel"]
  // CHECK: arith.addf
  // CHECK: arith.mulf
  // CHECK: linalg.yield
  // CHECK-NOT: stencil.apply
}