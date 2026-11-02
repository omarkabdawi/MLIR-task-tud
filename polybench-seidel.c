#define main polybench_seidel_main
#include "polybench-c/stencils/seidel-2d/seidel-2d.c"
#undef main

void run_polybench_seidel(int tsteps, int n, double A[n][n]) {
    kernel_seidel_2d(tsteps, n, A);
}
