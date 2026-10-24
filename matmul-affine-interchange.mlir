
module {
  func.func @matmul_named(%arg0: memref<?x?xf64>, %arg1: memref<?x?xf64>, %arg2: memref<?x?xf64>) {
    %c1 = arith.constant 1 : index
    %c0 = arith.constant 0 : index
    %dim = memref.dim %arg0, %c0 : memref<?x?xf64>
    %dim_0 = memref.dim %arg0, %c1 : memref<?x?xf64>
    %dim_1 = memref.dim %arg1, %c1 : memref<?x?xf64>

    // New loop order: i -> k -> j
    affine.for %arg3 = 0 to %dim {       // i loop: iterate over rows of C.
      affine.for %arg5 = 0 to %dim_0 {   // k loop: iterate over the reduction dimension.
        affine.for %arg4 = 0 to %dim_1 { // j loop: iterate over columns of C.
          %0 = affine.load %arg0[%arg3, %arg5] : memref<?x?xf64> // A[i,k]
          %1 = affine.load %arg1[%arg5, %arg4] : memref<?x?xf64> // B[k,j]
          %2 = affine.load %arg2[%arg3, %arg4] : memref<?x?xf64> // Current C[i,j]
          %3 = arith.mulf %0, %1 : f64
          %4 = arith.addf %2, %3 : f64
          affine.store %4, %arg2[%arg3, %arg4] : memref<?x?xf64> // Update C[i,j]
        }
      }
    }
    return
  }

  func.func @matmul_generic(%arg0: memref<?x?xf64>, %arg1: memref<?x?xf64>, %arg2: memref<?x?xf64>) {
    %c1 = arith.constant 1 : index
    %c0 = arith.constant 0 : index
    %dim = memref.dim %arg0, %c0 : memref<?x?xf64>
    %dim_0 = memref.dim %arg0, %c1 : memref<?x?xf64>
    %dim_1 = memref.dim %arg1, %c1 : memref<?x?xf64>

    // New loop order: i -> k -> j
    affine.for %arg3 = 0 to %dim {       // i loop: iterate over rows of C.
      affine.for %arg5 = 0 to %dim_0 {   // k loop: iterate over the reduction dimension.
        affine.for %arg4 = 0 to %dim_1 { // j loop: iterate over columns of C.
          %0 = affine.load %arg0[%arg3, %arg5] : memref<?x?xf64> // A[i,k]
          %1 = affine.load %arg1[%arg5, %arg4] : memref<?x?xf64> // B[k,j]
          %2 = affine.load %arg2[%arg3, %arg4] : memref<?x?xf64> // Current C[i,j]
          %3 = arith.mulf %0, %1 : f64
          %4 = arith.addf %2, %3 : f64
          affine.store %4, %arg2[%arg3, %arg4] : memref<?x?xf64> // Update C[i,j]
        }
      }
    }
    return
  }
}
