
module {
  func.func @kernel_jacobi_2d(
      %tsteps: index,
      %A: memref<?x?xf64>,
      %B: memref<?x?xf64>
  ) {
    %c0 = arith.constant 0 : index
    %c1 = arith.constant 1 : index

    scf.for %t = %c0 to %tsteps step %c1 {
      stencil.apply %A, %B
        offsets = [[0, 0], [0, -1], [0, 1], [1, 0], [-1, 0]]
        scale = 2.000000e-01
        : memref<?x?xf64>, memref<?x?xf64>

      stencil.apply %B, %A
        offsets = [[0, 0], [0, -1], [0, 1], [1, 0], [-1, 0]]
        scale = 2.000000e-01
        : memref<?x?xf64>, memref<?x?xf64>
    }

    func.return
  }

  func.func @kernel_seidel_2d(
      %tsteps: index,
      %A: memref<?x?xf64>
  ) {
    %c0 = arith.constant 0 : index
    %c1 = arith.constant 1 : index

    scf.for %t = %c0 to %tsteps step %c1 {
      stencil.sweep %A
        offsets = [
          [-1, -1], [-1, 0], [-1, 1],
          [0, -1], [0, 0], [0, 1],
          [1, -1], [1, 0], [1, 1]
        ]
        divisor = 9.000000e+00
        : memref<?x?xf64>
    }

    func.return
  }
}
