#define main polybench_jacobi_main
#include "polybench-c/stencils/jacobi-2d/jacobi-2d.c"
#undef main

void run_polybench_jacobi(int tsteps, int n, double A[n][n], double B[n][n]) {
    kernel_jacobi_2d(tsteps, n, A, B);
}
