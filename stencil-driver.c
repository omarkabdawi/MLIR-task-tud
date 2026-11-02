#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define JACOBI_N 30
#define SEIDEL_N 40
#define STEPS 20

typedef struct {
    double *allocated;
    double *aligned;
    int64_t offset;
    int64_t sizes[2];
    int64_t strides[2];
} MemRef2D;

void _mlir_ciface_kernel_jacobi_2d(
    int64_t tsteps, MemRef2D *A, MemRef2D *B);

void _mlir_ciface_kernel_seidel_2d(
    int64_t tsteps, MemRef2D *A);

void run_polybench_jacobi(
    int tsteps, int n, double A[n][n], double B[n][n]);

void run_polybench_seidel(
    int tsteps, int n, double A[n][n]);

static void initialize_jacobi(double A[JACOBI_N][JACOBI_N],
                              double B[JACOBI_N][JACOBI_N]) {
    for (int i = 0; i < JACOBI_N; ++i)
        for (int j = 0; j < JACOBI_N; ++j) {
            A[i][j] = ((double)i * (j + 2) + 2) / JACOBI_N;
            B[i][j] = ((double)i * (j + 3) + 3) / JACOBI_N;
        }
}

static void initialize_seidel(double A[SEIDEL_N][SEIDEL_N]) {
    for (int i = 0; i < SEIDEL_N; ++i)
        for (int j = 0; j < SEIDEL_N; ++j)
            A[i][j] = ((double)i * (j + 2) + 2) / SEIDEL_N;
}

static MemRef2D descriptor(double *data, int64_t n) {
    MemRef2D result = {
        data, data, 0, {n, n}, {n, 1}
    };
    return result;
}

static int compare(const char *name, const double *actual,
                   const double *expected, int n) {
    double max_difference = 0.0;
    int identical = 1;

    for (int i = 0; i < n * n; ++i) {
        double difference = fabs(actual[i] - expected[i]);

        if (difference > max_difference)
            max_difference = difference;

        if (actual[i] != expected[i])
            identical = 0;
    }

    printf("%s: %s\n", name,
           identical ? "bit-identical values" : "values differ");
    printf("  Maximum absolute difference: %.17g\n", max_difference);

    return max_difference <= 1e-12;
}

int main(void) {
    static double jacobi_A[JACOBI_N][JACOBI_N];
    static double jacobi_B[JACOBI_N][JACOBI_N];
    static double reference_A[JACOBI_N][JACOBI_N];
    static double reference_B[JACOBI_N][JACOBI_N];

    initialize_jacobi(jacobi_A, jacobi_B);
    initialize_jacobi(reference_A, reference_B);

    MemRef2D jacobi_A_desc = descriptor(&jacobi_A[0][0], JACOBI_N);
    MemRef2D jacobi_B_desc = descriptor(&jacobi_B[0][0], JACOBI_N);

    _mlir_ciface_kernel_jacobi_2d(
        STEPS, &jacobi_A_desc, &jacobi_B_desc);

    run_polybench_jacobi(
        STEPS, JACOBI_N, reference_A, reference_B);

    static double seidel_A[SEIDEL_N][SEIDEL_N];
    static double reference_S[SEIDEL_N][SEIDEL_N];

    initialize_seidel(seidel_A);
    initialize_seidel(reference_S);

    MemRef2D seidel_desc = descriptor(&seidel_A[0][0], SEIDEL_N);

    _mlir_ciface_kernel_seidel_2d(STEPS, &seidel_desc);

    run_polybench_seidel(STEPS, SEIDEL_N, reference_S);

    int jacobi_ok = compare(
        "Jacobi", &jacobi_A[0][0], &reference_A[0][0], JACOBI_N);

    int seidel_ok = compare(
        "Seidel", &seidel_A[0][0], &reference_S[0][0], SEIDEL_N);

    return (jacobi_ok && seidel_ok)
        ? EXIT_SUCCESS
        : EXIT_FAILURE;
}