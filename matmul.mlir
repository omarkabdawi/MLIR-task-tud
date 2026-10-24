
module {
  func.func @matmul_named(
      %A: memref<?x?xf64>,
      %B: memref<?x?xf64>,
      %C: memref<?x?xf64>) {
    linalg.matmul
      ins(%A, %B :
          memref<?x?xf64>, memref<?x?xf64>)
      outs(%C : memref<?x?xf64>)
    return
  }

  func.func @matmul_generic(
      %A: memref<?x?xf64>,
      %B: memref<?x?xf64>,
      %C: memref<?x?xf64>) {
    linalg.generic {
      indexing_maps = [
        affine_map<(d0, d1, d2) -> (d0, d2)>,
        affine_map<(d0, d1, d2) -> (d2, d1)>,
        affine_map<(d0, d1, d2) -> (d0, d1)>
      ],
      iterator_types = [
        "parallel", "parallel", "reduction"
      ]
    }
    ins(%A, %B :
        memref<?x?xf64>, memref<?x?xf64>)
    outs(%C : memref<?x?xf64>) {
    ^bb0(%a: f64, %b: f64, %c: f64):
      %product = arith.mulf %a, %b : f64
      %sum = arith.addf %c, %product : f64
      linalg.yield %sum : f64
    }
    return
  }
}
